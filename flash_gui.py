#!/usr/bin/env python3
"""GUI for RiffMIDI: assign chords to the five strum buttons and program
the board step by step -- flash the atmega16u2 to Arduino/USB-Serial mode,
compile and upload the sketch, then flash the atmega16u2 back to MIDI mode.
Each step is a separate button so you can retry a step (e.g. picking the
serial port again) without redoing the others.
"""

import colorsys
import json
import os
import re
import shlex
import shutil
import signal
import subprocess
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, simpledialog, ttk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
INO_PATH = os.path.join(SCRIPT_DIR, "RiffMIDI.ino")
CHORD_SETS_PATH = os.path.join(SCRIPT_DIR, "chord_sets.h")
NOTE_SETS_PATH = os.path.join(SCRIPT_DIR, "note_sets.h")
CUSTOM_CHORDS_PATH = os.path.join(SCRIPT_DIR, "custom_chords.h")
CONTROL_SURFACE_LIB = os.path.join(SCRIPT_DIR, "libraries", "Control-Surface")
BUILD_CACHE_DIR = os.path.join(SCRIPT_DIR, ".build-cache")
FQBN = "arduino:avr:mega"

ARDUINO_HEX = os.path.join(SCRIPT_DIR, "Arduino-usbserial-atmega16u2-Mega2560-Rev3.hex")
MIDI_HEX = os.path.join(SCRIPT_DIR, "arduino_midi.hex")

# (button label, ChordSet struct field name)
BUTTON_SLOTS = [
    ("Green", "green"),
    ("Red", "red"),
    ("Yellow", "yellow"),
    ("Blue", "blue"),
    ("Orange", "orange"),
]

# Standard tuning, low string to high string (left to right, matching a
# guitar-tech fretboard chart): (open-string label, open-string MIDI note).
GUITAR_STRINGS = [
    ("E", 40),  # 6th string, E2
    ("A", 45),  # 5th string, A2
    ("D", 50),  # 4th string, D3
    ("G", 55),  # 3rd string, G3
    ("B", 59),  # 2nd string, B3
    ("E", 64),  # 1st string, E4
]
MAX_FRET = 24
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
TABS_DIR = os.path.join(SCRIPT_DIR, "tabs")

try:
    import pymupdf as fitz
    PDF_SUPPORT = True
except ImportError:
    PDF_SUPPORT = False

# --- Look & feel -----------------------------------------------------------
# One accent color per numbered step, used consistently for that step's
# LabelFrame header, its buttons, and its inner frames/labels so each section
# reads as one colored block instead of flat gray-on-gray.

APP_BG = "#F6F3EE"
TEXT = "#2B2B2B"
MUTED_TEXT = "#6B6B6B"
FONT_HEADER = ("Helvetica", 12, "bold")
FONT_BODY = ("Helvetica", 10)
FONT_MONO = ("Menlo", 10)

SECTIONS = {
    "custom": {"tint": "#E8F1FB", "accent": "#2E6DA4", "button": "#3B82C4"},
    "sets": {"tint": "#E9F7EF", "accent": "#1E8449", "button": "#2FAE66"},
    "notes": {"tint": "#E4F6F6", "accent": "#1C7A7A", "button": "#2FA6A6"},
    "arduino": {"tint": "#FDF1E3", "accent": "#B9770E", "button": "#E08E2B"},
    "upload": {"tint": "#F1E9FB", "accent": "#6C3483", "button": "#8E5CB5"},
    "midi": {"tint": "#FBEAEA", "accent": "#A93226", "button": "#D1574B"},
    "neutral": {"tint": APP_BG, "accent": TEXT, "button": "#5A5A5A"},
}

# The actual physical button colors on the controller -- used to color the
# Green/Red/Yellow/Blue/Orange rows in the chord-set editor so it reads as
# "this is the button you press," not just an arbitrary label.
BUTTON_COLORS = {
    "Green": "#2FAE4A",
    "Red": "#E0423B",
    "Yellow": "#E8B91E",
    "Blue": "#2E7FE0",
    "Orange": "#E08A2E",
}


def _shade(hex_color, factor):
    """Darken (factor<1) or lighten (factor>1) a #rrggbb color."""
    hex_color = hex_color.lstrip("#")
    r, g, b = (int(hex_color[i:i + 2], 16) for i in (0, 2, 4))
    r, g, b = (min(255, max(0, int(c * factor))) for c in (r, g, b))
    return f"#{r:02x}{g:02x}{b:02x}"


def mk_section(parent, text, section):
    s = SECTIONS[section]
    frame = tk.LabelFrame(
        parent, text=text, padx=10, pady=10,
        bg=s["tint"], fg=s["accent"], font=FONT_HEADER,
    )
    return frame


def mk_frame(parent, section):
    return tk.Frame(parent, bg=SECTIONS[section]["tint"])


def mk_label(parent, text, section, **kw):
    s = SECTIONS[section]
    kw.setdefault("bg", s["tint"])
    kw.setdefault("fg", TEXT)
    kw.setdefault("font", FONT_BODY)
    return tk.Label(parent, text=text, **kw)


class ColorButton(tk.Label):
    """A Label made to act as a colored push button.

    macOS's native Aqua rendering ignores the -background/-foreground
    options on plain tk.Button (it always draws the system gray button
    face), so a colorful button palette has to be faked with a Label plus
    click/hover bindings instead -- Label's colors ARE respected."""

    def __init__(self, parent, text, command, bg, fg="white", font=None, padx=12, pady=5, **kw):
        super().__init__(
            parent, text=text, bg=bg, fg=fg, font=font or FONT_BODY,
            padx=padx, pady=pady, relief="flat", cursor="pointinghand", **kw
        )
        self._command = command
        self._normal_bg = bg
        self._normal_fg = fg
        self._hover_bg = _shade(bg, 0.85)
        self._enabled = True
        self.bind("<Button-1>", self._on_click)
        self.bind("<Enter>", self._on_enter)
        self.bind("<Leave>", self._on_leave)

    def _on_click(self, _event):
        if self._enabled and self._command:
            self._command()

    def _on_enter(self, _event):
        if self._enabled:
            tk.Label.configure(self, bg=self._hover_bg)

    def _on_leave(self, _event):
        if self._enabled:
            tk.Label.configure(self, bg=self._normal_bg)

    def config(self, **kw):
        if "state" in kw:
            state = kw.pop("state")
            self._enabled = (state == "normal")
            if self._enabled:
                tk.Label.configure(self, bg=self._normal_bg, fg=self._normal_fg, cursor="pointinghand")
            else:
                tk.Label.configure(self, bg="#D4D4D4", fg="#8A8A8A", cursor="arrow")
        if kw:
            tk.Label.configure(self, **kw)

    configure = config


def mk_button(parent, text, command, section, **kw):
    s = SECTIONS[section]
    bg = s["button"]
    return ColorButton(parent, text, command, bg=bg, **kw)


def bind_mousewheel_scroll(canvas):
    """Scroll `canvas` on the mouse wheel, but only while the pointer is over
    it. A plain canvas.bind_all("<MouseWheel>") binds globally for the whole
    app, so with more than one scrollable canvas open at once (e.g. this
    window's own scroll plus a chord/note builder dialog's fretboard), the
    most-recently-opened one silently steals every other canvas's scroll."""
    def _on_wheel(event):
        canvas.yview_scroll(int(-1 * (event.delta)), "units")
    def _bind(_event):
        canvas.bind_all("<MouseWheel>", _on_wheel)
    def _unbind(_event):
        canvas.unbind_all("<MouseWheel>")
    canvas.bind("<Enter>", _bind)
    canvas.bind("<Leave>", _unbind)


def note_name_at(open_midi, fret):
    return NOTE_NAMES[(open_midi + fret) % 12]


def note_color(name):
    """A distinct pastel color per pitch class, consistent everywhere it's used."""
    hue = NOTE_NAMES.index(name) / 12.0
    r, g, b = colorsys.hsv_to_rgb(hue, 0.38, 0.95)
    return f"#{int(r * 255):02x}{int(g * 255):02x}{int(b * 255):02x}"


def find_tool(name, extra_candidates=()):
    path = shutil.which(name)
    if path:
        return path
    for candidate in extra_candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


def find_dfu_programmer():
    return find_tool(
        "dfu-programmer",
        ("/opt/homebrew/bin/dfu-programmer", "/usr/local/bin/dfu-programmer"),
    )


def find_arduino_cli():
    return find_tool(
        "arduino-cli",
        ("/opt/homebrew/bin/arduino-cli", "/usr/local/bin/arduino-cli"),
    )


TAB_LINE_LABEL_RE = re.compile(r"^\s*([eEaAdDgGbB])\b")


def parse_ascii_tab(text):
    """Parse a standard 6-line ASCII guitar tab block (one STATIC chord
    shape, not a riff) into fret numbers, low string to high string --
    matching GUITAR_STRINGS order. Each element is an int fret, or None for
    a muted/unplayed string.

    Assumes the conventional top-to-bottom = high-string-to-low-string line
    order (e.g. e/B/G/D/A/E from top to bottom), which is how the vast
    majority of pasted tabs are written. Raises ValueError with a
    human-readable reason if the text isn't a single recognizable 6-line
    tab block, or if it looks like a multi-note riff instead of one chord
    (more than one fret number on some string's line)."""
    lines = [line for line in text.splitlines() if line.strip()]
    tab_line_contents = []
    for line in lines:
        m = TAB_LINE_LABEL_RE.match(line)
        if m:
            tab_line_contents.append(line[m.end():])
    if len(tab_line_contents) != 6:
        raise ValueError(
            f"Expected 6 tab lines (one per string, each starting with E/A/D/G/B/e), "
            f"found {len(tab_line_contents)}."
        )

    frets_top_to_bottom = []
    multi_note_lines = 0
    for content in tab_line_contents:
        numbers = re.findall(r"\d+", content)
        if not numbers:
            frets_top_to_bottom.append(None)
            continue
        if len(numbers) > 1:
            multi_note_lines += 1
        frets_top_to_bottom.append(int(numbers[0]))

    if multi_note_lines > 0:
        raise ValueError(
            "This looks like a multi-note riff (some strings have more than one "
            "fret number), not a single chord shape. Only single-chord tab blocks "
            "are supported right now -- paste just one column/moment."
        )

    out_of_range = [f for f in frets_top_to_bottom if f is not None and f > MAX_FRET]
    if out_of_range:
        raise ValueError(f"Fret {out_of_range[0]} is beyond the supported range (0-{MAX_FRET}).")

    return list(reversed(frets_top_to_bottom))  # top-to-bottom (high-to-low) -> low-to-high


def render_pdf_page(pdf_path, page_number, zoom):
    """Render one PDF page to PNG bytes at the given zoom factor, for display
    in a Tkinter PhotoImage. Returns (num_pages, png_bytes, width, height)."""
    doc = fitz.open(pdf_path)
    try:
        num_pages = len(doc)
        page = doc[page_number]
        pix = page.get_pixmap(matrix=fitz.Matrix(zoom, zoom))
        return num_pages, pix.tobytes("png"), pix.width, pix.height
    finally:
        doc.close()


def parse_chord_names(ino_path):
    """Pull the chordNames[] string list out of RiffMIDI.ino, in order."""
    with open(ino_path, "r") as f:
        text = f.read()
    match = re.search(r"chordNames\[\]\s*=\s*\{(.*?)\};", text, re.DOTALL)
    if not match:
        return []
    return re.findall(r'"([^"]*)"', match.group(1))


def read_chord_sets(path):
    """Parse the chordSets[] initializer rows out of chord_sets.h, in order.
    Returns a list of dicts with "name" plus the BUTTON_SLOTS field names."""
    if not os.path.isfile(path):
        return []
    with open(path, "r") as f:
        text = f.read()
    match = re.search(r"chordSets\[\]\s*=\s*\{(.*?)\};", text, re.DOTALL)
    if not match:
        return []
    rows = re.findall(
        r'\{\s*"([^"]*)"\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}',
        match.group(1),
    )
    fields = [f for _, f in BUTTON_SLOTS]
    result = []
    for row in rows:
        name, *indices = row
        s = dict(zip(fields, (int(v) for v in indices)))
        s["name"] = name
        result.append(s)
    return result


def write_chord_sets(path, sets, chord_names):
    lines = [
        '// Chord sets ("banks"): each row assigns a chord to all five buttons at',
        "// once, as indices into the `chords[]` / `chordNames[]` arrays in",
        "// RiffMIDI.ino. Up/Down select cycles through these sets on the device.",
        "// Generated by flash_gui.py's chord set editor -- edit by hand only if",
        "// you're not using the GUI.",
        "#ifndef CHORD_SETS_H",
        "#define CHORD_SETS_H",
        "",
        "struct ChordSet {",
        "  const char* name;",
        "  uint8_t green;",
        "  uint8_t red;",
        "  uint8_t yellow;",
        "  uint8_t blue;",
        "  uint8_t orange;",
        "};",
        "",
        "const ChordSet chordSets[] = {",
    ]
    for i, s in enumerate(sets):
        name = s.get("name") or f"Set {i + 1}"
        values = ", ".join(str(s[f]) for _, f in BUTTON_SLOTS)
        names = ", ".join(chord_names[s[f]] if 0 <= s[f] < len(chord_names) else "?" for _, f in BUTTON_SLOTS)
        lines.append(f'  {{"{name}", {values}}},  // {names}')
    lines += [
        "};",
        "",
        "const uint8_t numChordSets = sizeof(chordSets) / sizeof(ChordSet);",
        "",
        "#endif",
        "",
    ]
    with open(path, "w") as f:
        f.write("\n".join(lines))


def parse_custom_chords(path):
    """Parse the customChordN[] arrays out of custom_chords.h, in order.
    Returns a list of {"name": str, "notes": [int, ...]}."""
    if not os.path.isfile(path):
        return []
    with open(path, "r") as f:
        text = f.read()
    rows = re.findall(
        r"int\s+customChord\d+\[\]\s*=\s*\{([^}]*)\};\s*//\s*(.*)", text
    )
    result = []
    for notes_str, name in rows:
        notes = [int(n.strip()) for n in notes_str.split(",") if n.strip()]
        result.append({"name": name.strip(), "notes": notes})
    return result


def write_custom_chords(path, custom_chords):
    lines = [
        "// Custom chords built from flash_gui.py's fretboard chord builder. Each",
        "// array lists the actual MIDI note numbers for the strings that are",
        "// played (low to high; muted strings are simply left out). These get",
        "// appended onto the built-in chords[]/chordSizes[]/chordNames[] arrays",
        "// in RiffMIDI.ino. Edit by hand only if you're not using the GUI.",
        "#ifndef CUSTOM_CHORDS_H",
        "#define CUSTOM_CHORDS_H",
        "",
    ]
    for i, c in enumerate(custom_chords):
        notes_str = ", ".join(str(n) for n in c["notes"])
        lines.append(f"int customChord{i}[] = {{{notes_str}}};  // {c['name']}")
    lines.append("")
    if custom_chords:
        array_names = [f"customChord{i}" for i in range(len(custom_chords))]
        lines.append(f"#define CUSTOM_CHORDS_LIST {', '.join(array_names)}")
        sizes = [f"(sizeof({n}) / sizeof(int))" for n in array_names]
        lines.append(f"#define CUSTOM_CHORD_SIZES_LIST {', '.join(sizes)}")
        names = ", ".join(f'"{c["name"]}"' for c in custom_chords)
        lines.append(f"#define CUSTOM_CHORD_NAMES_LIST {names}")
    else:
        lines.append("#define CUSTOM_CHORDS_LIST")
        lines.append("#define CUSTOM_CHORD_SIZES_LIST")
        lines.append("#define CUSTOM_CHORD_NAMES_LIST")
    lines += ["", "#endif", ""]
    with open(path, "w") as f:
        f.write("\n".join(lines))


def format_set_summary(index, chord_set, chord_names):
    names = ", ".join(
        chord_names[chord_set[f]] if 0 <= chord_set[f] < len(chord_names) else "?"
        for _, f in BUTTON_SLOTS
    )
    name = chord_set.get("name") or f"Set {index + 1}"
    return f"{name}: {names}"


# The MIDI_Notes:: identifiers available for a single-note button, exactly as
# spelled in Notes.hpp (flats, not sharps) -- used directly so no name
# translation is needed when writing note_sets.h.
NOTE_LETTERS = ["C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"]

# NOTE_NAMES (used for the fretboard's cell labels/colors) spells accidentals
# as sharps; NOTE_LETTERS/MIDI_Notes:: identifiers use flats -- this converts
# a fretboard cell's note name to the matching MIDI_Notes:: identifier.
SHARP_TO_FLAT = {"C#": "Db", "D#": "Eb", "F#": "Gb", "G#": "Ab", "A#": "Bb"}


def to_note_letter(note_name):
    return SHARP_TO_FLAT.get(note_name, note_name)


def read_note_sets(path):
    """Parse the noteSets[] initializer rows out of note_sets.h, in order.
    Returns a list of dicts with "name" plus the BUTTON_SLOTS field names
    (values are MIDI_Notes:: identifiers, e.g. "C", "Db", ...)."""
    if not os.path.isfile(path):
        return []
    with open(path, "r") as f:
        text = f.read()
    match = re.search(r"noteSets\[\]\s*=\s*\{(.*?)\};", text, re.DOTALL)
    if not match:
        return []
    rows = re.findall(
        r'\{\s*"([^"]*)"\s*,\s*MIDI_Notes::(\w+)\s*,\s*MIDI_Notes::(\w+)\s*,\s*'
        r'MIDI_Notes::(\w+)\s*,\s*MIDI_Notes::(\w+)\s*,\s*MIDI_Notes::(\w+)\s*\}',
        match.group(1),
    )
    fields = [f for _, f in BUTTON_SLOTS]
    result = []
    for row in rows:
        name, *notes = row
        s = dict(zip(fields, notes))
        s["name"] = name
        result.append(s)
    return result


def write_note_sets(path, sets):
    lines = [
        '// Note sets ("banks") for the 5 High (single-note) buttons: each row',
        "// assigns one note to each button at once. Ok/Back cycles through these",
        "// sets on the device (separate from Up/Down, which cycles chord sets).",
        "// Generated by flash_gui.py's note set editor -- edit by hand only if",
        "// you're not using the GUI.",
        "#ifndef NOTE_SETS_H",
        "#define NOTE_SETS_H",
        "",
        "struct NoteSet {",
        "  const char* name;",
        "  MIDI_Notes::Note green;",
        "  MIDI_Notes::Note red;",
        "  MIDI_Notes::Note yellow;",
        "  MIDI_Notes::Note blue;",
        "  MIDI_Notes::Note orange;",
        "};",
        "",
        "const NoteSet noteSets[] = {",
    ]
    for i, s in enumerate(sets):
        name = s.get("name") or f"Set {i + 1}"
        values = ", ".join(f"MIDI_Notes::{s[f]}" for _, f in BUTTON_SLOTS)
        lines.append(f'  {{"{name}", {values}}},')
    lines += [
        "};",
        "",
        "const uint8_t numNoteSets = sizeof(noteSets) / sizeof(NoteSet);",
        "",
        "#endif",
        "",
    ]
    with open(path, "w") as f:
        f.write("\n".join(lines))


def format_note_set_summary(index, note_set):
    notes = ", ".join(note_set[f] for _, f in BUTTON_SLOTS)
    name = note_set.get("name") or f"Set {index + 1}"
    return f"{name}: {notes}"


def list_serial_ports(arduino_cli):
    try:
        result = subprocess.run(
            [arduino_cli, "board", "list", "--format", "json"],
            capture_output=True, text=True, timeout=20,
        )
        data = json.loads(result.stdout or "{}")
        ports = [p["port"]["address"] for p in data.get("detected_ports", [])]
        return sorted(ports)
    except Exception:
        return []


def build_dfu_shell_cmd(dfu, hex_path):
    # dfu-programmer's "erase" exits non-zero whenever it actually had to
    # erase non-blank memory (even though the erase itself succeeds), so
    # its exit code must not gate the rest of the chain.
    return (
        f"{shlex.quote(dfu)} atmega16u2 erase; "
        f"{shlex.quote(dfu)} atmega16u2 flash {shlex.quote(hex_path)} && "
        f"{shlex.quote(dfu)} atmega16u2 reset"
    )


def run_privileged_shell(shell_cmd, timeout=120):
    applescript_string = shell_cmd.replace("\\", "\\\\").replace('"', '\\"')
    osascript_cmd = [
        "osascript", "-e",
        f'do shell script "{applescript_string}" with administrator privileges',
    ]
    try:
        result = subprocess.run(osascript_cmd, capture_output=True, text=True, timeout=timeout)
        return result.returncode, (result.stdout or "") + (result.stderr or "")
    except subprocess.TimeoutExpired:
        return -1, "Timed out."
    except Exception as exc:  # noqa: BLE001
        return -1, str(exc)


def kill_process_group(proc):
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
    except ProcessLookupError:
        pass
    try:
        proc.wait(timeout=5)
    except Exception:  # noqa: BLE001
        pass


def run_with_timeout_killing_children(cmd, timeout, on_start=None):
    """Like subprocess.run(..., timeout=...), but on timeout kills the whole
    process group instead of just the direct child. Without this, a timed-out
    `arduino-cli compile --upload` leaves its avrdude grandchild running
    forever if avrdude is the one actually stuck (e.g. on a bad serial port)
    -- subprocess.run's own timeout handling only kills arduino-cli itself.

    `on_start`, if given, is called with the Popen object right after it's
    created, so the caller can track it (e.g. to kill it if the GUI closes
    mid-command, not just on timeout)."""
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        start_new_session=True,  # its own process group, so we can kill it whole
    )
    if on_start is not None:
        on_start(proc)
    try:
        output, _ = proc.communicate(timeout=timeout)
        return proc.returncode, output
    except subprocess.TimeoutExpired:
        kill_process_group(proc)
        return -1, f"Timed out after {timeout}s -- killed the process and any children (e.g. avrdude)."


class ChordSetDialog(tk.Toplevel):
    """Modal dialog to pick/edit the 5 chords (one per button) for one set."""

    def __init__(self, parent, chord_names, title, initial=None):
        super().__init__(parent)
        self.title(title)
        self.resizable(False, False)
        self.result = None
        self.chord_names = chord_names
        self.configure(bg=SECTIONS["sets"]["tint"])
        bg = SECTIONS["sets"]["tint"]

        initial = initial or {f: 0 for _, f in BUTTON_SLOTS}

        name_row = tk.Frame(self, bg=bg)
        name_row.pack(fill="x", padx=10, pady=(10, 4))
        tk.Label(name_row, text="Set name:", width=8, anchor="w", bg=bg, fg=TEXT, font=FONT_BODY).pack(side="left")
        self.name_var = tk.StringVar(value=initial.get("name", ""))
        tk.Entry(
            name_row, textvariable=self.name_var, width=30, font=FONT_BODY,
            bg="white", fg=TEXT, insertbackground=TEXT,
        ).pack(side="left")

        self.vars = {}
        for label, field in BUTTON_SLOTS:
            row = tk.Frame(self, bg=bg)
            row.pack(fill="x", padx=10, pady=4)
            swatch = tk.Label(row, text=" ", bg=BUTTON_COLORS[label], width=2)
            swatch.pack(side="left", padx=(0, 6))
            tk.Label(row, text=f"{label}:", width=7, anchor="w", bg=bg, fg=TEXT, font=FONT_BODY).pack(side="left")
            var = tk.StringVar()
            idx = initial[field]
            if chord_names and 0 <= idx < len(chord_names):
                var.set(chord_names[idx])
            combo = ttk.Combobox(row, textvariable=var, values=chord_names, state="readonly", width=30)
            combo.pack(side="left")
            self.vars[field] = var

        btn_row = tk.Frame(self, bg=bg)
        btn_row.pack(fill="x", padx=10, pady=(6, 10))
        mk_button(btn_row, "Cancel", self.destroy, "neutral").pack(side="right")
        mk_button(btn_row, "OK", self._on_ok, "sets").pack(side="right", padx=(0, 8))

        self.transient(parent)
        self.grab_set()

    def _on_ok(self):
        set_name = self.name_var.get().strip()
        if not set_name:
            messagebox.showerror("Name required", "Give this chord set a name (e.g. Verse, Chorus).")
            return
        result = {"name": set_name}
        for _, field in BUTTON_SLOTS:
            name = self.vars[field].get()
            result[field] = self.chord_names.index(name) if name in self.chord_names else 0
        self.result = result
        self.destroy()


class NoteSetDialog(tk.Toplevel):
    """Modal dialog to pick the single note assigned to each High button,
    using the exact same fretboard (standard tuning, frets 0-24, note-colored
    cells) as the Custom Chord Builder -- one shared grid, since a note set
    only needs a pitch class per button, not a specific string/fret/octave.
    Click a colored button tab to make it "active," then click any fret cell
    to assign that note to it (multiple fretboard positions can share the
    same note, which is fine -- only the note letter gets stored)."""

    def __init__(self, parent, title, initial=None):
        super().__init__(parent)
        self.title(title)
        self.resizable(False, False)
        self.result = None
        bg = SECTIONS["notes"]["tint"]
        self.configure(bg=bg)

        initial = initial or {f: "C" for _, f in BUTTON_SLOTS}
        self.notes = {field: initial.get(field, "C") for _, field in BUTTON_SLOTS}
        self.active_field = BUTTON_SLOTS[0][1]
        self.cells = [[] for _ in GUITAR_STRINGS]

        name_row = tk.Frame(self, bg=bg)
        name_row.pack(fill="x", padx=10, pady=(10, 4))
        tk.Label(name_row, text="Set name:", width=8, anchor="w", bg=bg, fg=TEXT, font=FONT_BODY).pack(side="left")
        self.name_var = tk.StringVar(value=initial.get("name", ""))
        tk.Entry(
            name_row, textvariable=self.name_var, width=30, font=FONT_BODY,
            bg="white", fg=TEXT, insertbackground=TEXT,
        ).pack(side="left")

        tk.Label(
            self, text="Pick a button below, then click its note on the fretboard:",
            bg=bg, fg=TEXT, font=FONT_BODY,
        ).pack(anchor="w", padx=10, pady=(6, 2))

        tabs_row = tk.Frame(self, bg=bg)
        tabs_row.pack(fill="x", padx=10, pady=(0, 8))
        self.tab_labels = {}
        for label, field in BUTTON_SLOTS:
            tab = tk.Label(
                tabs_row, text=f"{label}: {self.notes[field]}", bg=BUTTON_COLORS[label], fg="white",
                font=("Helvetica", 10, "bold"), padx=10, pady=6, relief="raised", borderwidth=1,
                cursor="pointinghand",
            )
            tab.pack(side="left", padx=3)
            tab.bind("<Button-1>", lambda e, f=field: self._set_active(f))
            self.tab_labels[field] = tab

        container = tk.Frame(self, bg=bg)
        container.pack(padx=10, pady=(0, 6))
        canvas = tk.Canvas(container, height=420, highlightthickness=0, bg=bg)
        scrollbar = tk.Scrollbar(container, orient="vertical", command=canvas.yview)
        grid_frame = tk.Frame(canvas, bg=bg)

        def _on_grid_configure(event):
            canvas.configure(scrollregion=canvas.bbox("all"), width=event.width)
        grid_frame.bind("<Configure>", _on_grid_configure)
        canvas.create_window((0, 0), window=grid_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        bind_mousewheel_scroll(canvas)

        header_bg = SECTIONS["notes"]["accent"]
        for col, (label, _) in enumerate(GUITAR_STRINGS):
            tk.Label(
                grid_frame, text=label, width=4, bg=header_bg, fg="white", font=("", 10, "bold")
            ).grid(row=0, column=col + 1, padx=1, pady=1)
        tk.Label(grid_frame, text="", width=3, bg=header_bg).grid(row=0, column=0)

        for fret in range(0, MAX_FRET + 1):
            tk.Label(grid_frame, text=str(fret), width=3, bg=bg, fg=TEXT).grid(row=fret + 1, column=0)
            for col, (_, open_midi) in enumerate(GUITAR_STRINGS):
                name = note_name_at(open_midi, fret)
                cell = tk.Label(
                    grid_frame, text=name, width=4, bg=note_color(name), fg=TEXT,
                    relief="raised", borderwidth=1, cursor="pointinghand",
                )
                cell.bind("<Button-1>", lambda e, c=col, fr=fret: self._on_cell_click(c, fr))
                cell.grid(row=fret + 1, column=col + 1, padx=1, pady=1)
                self.cells[col].append(cell)

        self._refresh_fretboard()

        btn_row = tk.Frame(self, bg=bg)
        btn_row.pack(fill="x", padx=10, pady=(6, 10))
        mk_button(btn_row, "Cancel", self.destroy, "neutral").pack(side="right")
        mk_button(btn_row, "OK", self._on_ok, "notes").pack(side="right", padx=(0, 8))

        self.transient(parent)
        self.grab_set()

    def _set_active(self, field):
        self.active_field = field
        self._refresh_tabs()
        self._refresh_fretboard()

    def _on_cell_click(self, col, fret):
        _, open_midi = GUITAR_STRINGS[col]
        note = to_note_letter(note_name_at(open_midi, fret))
        self.notes[self.active_field] = note
        self._refresh_tabs()
        self._refresh_fretboard()

    def _refresh_tabs(self):
        for label, field in BUTTON_SLOTS:
            tab = self.tab_labels[field]
            tab.configure(text=f"{label}: {self.notes[field]}")
            if field == self.active_field:
                tab.configure(relief="sunken", borderwidth=3)
            else:
                tab.configure(relief="raised", borderwidth=1)

    def _refresh_fretboard(self):
        active_note = self.notes[self.active_field]
        for col, (_, open_midi) in enumerate(GUITAR_STRINGS):
            for fret, cell in enumerate(self.cells[col]):
                name = note_name_at(open_midi, fret)
                if to_note_letter(name) == active_note:
                    cell.configure(text=f"[{name}]", relief="sunken", borderwidth=3)
                else:
                    cell.configure(text=name, relief="raised", borderwidth=1)

    def _on_ok(self):
        set_name = self.name_var.get().strip()
        if not set_name:
            messagebox.showerror("Name required", "Give this note set a name (e.g. Lead, Riff).")
            return
        result = {"name": set_name}
        for _, field in BUTTON_SLOTS:
            result[field] = self.notes[field]
        self.result = result
        self.destroy()


class PdfViewerWindow(tk.Toplevel):
    """Just displays a PDF tab page as an image, with page/zoom controls, so
    you can read the tab by eye and manually enter it into the chord/note
    builder yourself. Deliberately does NOT try to parse the tab -- PDF tab
    layouts vary too much (measure numbers, grace notes, palm-mute markings,
    multi-string chords whose numbers sit close together) for automatic
    extraction to be reliable. This window is non-modal: it doesn't grab
    focus, so you can keep it open and switch back and forth to the builder
    dialog while you transcribe by hand."""

    def __init__(self, parent, title="Tab Reference"):
        super().__init__(parent)
        self.title(title)
        self.resizable(False, False)
        bg = APP_BG
        self.configure(bg=bg)

        self.doc_path = None
        self.num_pages = 0
        self.page_index = 0
        self.zoom = 1.5
        self.photo_image = None  # keep a reference so Tk doesn't garbage-collect it

        top_row = tk.Frame(self, bg=bg)
        top_row.pack(fill="x", padx=10, pady=(10, 4))
        self.file_var = tk.StringVar(value="(no file chosen)")
        tk.Label(
            top_row, textvariable=self.file_var, bg=bg, fg=TEXT, font=FONT_BODY, width=36, anchor="w"
        ).pack(side="left")
        choose_btn = mk_button(top_row, "Choose PDF...", self._choose_pdf, "neutral")
        choose_btn.pack(side="left", padx=(8, 0))
        if not PDF_SUPPORT:
            choose_btn.configure(state="disabled")
            tk.Label(
                self, text="PyMuPDF isn't installed -- install it with: pip3 install pymupdf",
                bg=bg, fg="#B00020", font=FONT_BODY,
            ).pack(anchor="w", padx=10)

        nav_row = tk.Frame(self, bg=bg)
        nav_row.pack(fill="x", padx=10, pady=(0, 6))
        mk_button(nav_row, "< Prev", self._prev_page, "neutral").pack(side="left")
        self.page_label_var = tk.StringVar(value="Page - / -")
        tk.Label(nav_row, textvariable=self.page_label_var, bg=bg, fg=TEXT, font=FONT_BODY, width=12).pack(
            side="left", padx=8
        )
        mk_button(nav_row, "Next >", self._next_page, "neutral").pack(side="left")
        mk_button(nav_row, "Zoom -", self._zoom_out, "neutral").pack(side="left", padx=(16, 0))
        mk_button(nav_row, "Zoom +", self._zoom_in, "neutral").pack(side="left", padx=(4, 0))

        canvas_frame = tk.Frame(self, bg=bg)
        canvas_frame.pack(padx=10, pady=(0, 10))
        self.canvas = tk.Canvas(
            canvas_frame, width=700, height=820, bg="white",
            highlightthickness=1, highlightbackground=SECTIONS["neutral"]["accent"],
        )
        vbar = tk.Scrollbar(canvas_frame, orient="vertical", command=self.canvas.yview)
        hbar = tk.Scrollbar(canvas_frame, orient="horizontal", command=self.canvas.xview)
        self.canvas.configure(yscrollcommand=vbar.set, xscrollcommand=hbar.set)
        self.canvas.grid(row=0, column=0)
        vbar.grid(row=0, column=1, sticky="ns")
        hbar.grid(row=1, column=0, sticky="ew")

        # No transient()/grab_set() -- stays open and usable alongside
        # whichever dialog launched it.

    def _choose_pdf(self):
        initialdir = TABS_DIR if os.path.isdir(TABS_DIR) else SCRIPT_DIR
        path = filedialog.askopenfilename(
            parent=self, title="Choose a tab PDF", initialdir=initialdir,
            filetypes=[("PDF files", "*.pdf")],
        )
        if not path:
            return
        try:
            num_pages, _, _, _ = render_pdf_page(path, 0, self.zoom)
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("Couldn't open PDF", str(exc))
            return
        self.doc_path = path
        self.num_pages = num_pages
        self.page_index = 0
        self.file_var.set(os.path.basename(path))
        self._render_page()

    def _render_page(self):
        if self.doc_path is None:
            return
        try:
            _, png_bytes, width, height = render_pdf_page(self.doc_path, self.page_index, self.zoom)
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("Couldn't render page", str(exc))
            return
        self.photo_image = tk.PhotoImage(data=png_bytes)
        self.canvas.delete("all")
        self.canvas.create_image(0, 0, anchor="nw", image=self.photo_image)
        self.canvas.configure(scrollregion=(0, 0, width, height))
        self.page_label_var.set(f"Page {self.page_index + 1} / {self.num_pages}")

    def _prev_page(self):
        if self.doc_path and self.page_index > 0:
            self.page_index -= 1
            self._render_page()

    def _next_page(self):
        if self.doc_path and self.page_index < self.num_pages - 1:
            self.page_index += 1
            self._render_page()

    def _zoom_in(self):
        self.zoom = min(self.zoom + 0.25, 4.0)
        self._render_page()

    def _zoom_out(self):
        self.zoom = max(self.zoom - 0.25, 0.5)
        self._render_page()


class ChordBuilderDialog(tk.Toplevel):
    """Fretboard grid to build a custom chord: click one fret per string
    (at most one), or click the selected fret again to mute that string."""

    def __init__(self, parent, initial_notes=None, initial_name=""):
        super().__init__(parent)
        self.title("Custom Chord Builder")
        self.resizable(False, False)
        self.result = None
        bg = SECTIONS["custom"]["tint"]
        self.configure(bg=bg)

        # selection[string_index] = fret number played on that string, or
        # None if that string is muted (not played).
        self.selection = [0] * len(GUITAR_STRINGS)
        if initial_notes:
            self.selection = [None] * len(GUITAR_STRINGS)
            remaining = list(initial_notes)
            for i, (_, open_midi) in enumerate(GUITAR_STRINGS):
                for note in list(remaining):
                    if 0 <= note - open_midi <= MAX_FRET:
                        self.selection[i] = note - open_midi
                        remaining.remove(note)
                        break

        self.buttons = [[] for _ in GUITAR_STRINGS]

        tk.Label(
            self,
            text="Click a note to play it on that string. Click the selected\n"
                 "note again to mute (not play) that string.",
            justify="left", bg=bg, fg=TEXT, font=FONT_BODY,
        ).pack(anchor="w", padx=10, pady=(10, 4))

        name_row = tk.Frame(self, bg=bg)
        name_row.pack(fill="x", padx=10, pady=(0, 8))
        tk.Label(name_row, text="Chord name:", bg=bg, fg=TEXT, font=FONT_BODY).pack(side="left")
        self.name_var = tk.StringVar(value=initial_name)
        tk.Entry(
            name_row, textvariable=self.name_var, width=30, font=FONT_BODY,
            bg="white", fg=TEXT, insertbackground=TEXT,
        ).pack(side="left", padx=(6, 0))
        mk_button(name_row, "Paste Tab...", self._open_paste_tab, "custom").pack(side="left", padx=(10, 0))
        mk_button(name_row, "View PDF Tab...", self._open_pdf_viewer, "custom").pack(side="left", padx=(10, 0))

        # --- Tab entry: type/pick a fret 0-24 (or "x" for muted) per string,
        # kept in sync with clicks on the fretboard grid below. ---
        tab_frame = tk.LabelFrame(
            self, text="Tab entry (fret 0-24, or x for muted)", padx=8, pady=6,
            bg=bg, fg=SECTIONS["custom"]["accent"], font=("Helvetica", 10, "bold"),
        )
        tab_frame.pack(fill="x", padx=10, pady=(0, 8))

        tab_values = ["x"] + [str(i) for i in range(MAX_FRET + 1)]
        self.tab_vars = []
        for col, (label, _) in enumerate(GUITAR_STRINGS):
            string_col = tk.Frame(tab_frame, bg=bg)
            string_col.pack(side="left", padx=4)
            tk.Label(string_col, text=label, font=("", 9, "bold"), bg=bg, fg=TEXT).pack()
            var = tk.StringVar(value=self._tab_value(col))
            combo = ttk.Combobox(
                string_col, textvariable=var, values=tab_values, width=3, state="readonly"
            )
            combo.pack()
            combo.bind("<<ComboboxSelected>>", lambda e, c=col: self._on_tab_change(c))
            self.tab_vars.append(var)

        container = tk.Frame(self, bg=bg)
        container.pack(padx=10, pady=(0, 6))
        canvas = tk.Canvas(container, height=420, highlightthickness=0, bg=bg)
        scrollbar = tk.Scrollbar(container, orient="vertical", command=canvas.yview)
        grid_frame = tk.Frame(canvas, bg=bg)

        def _on_grid_configure(event):
            canvas.configure(scrollregion=canvas.bbox("all"), width=event.width)
        grid_frame.bind("<Configure>", _on_grid_configure)
        canvas.create_window((0, 0), window=grid_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        bind_mousewheel_scroll(canvas)

        header_bg = SECTIONS["custom"]["accent"]
        # Header row: open-string letters
        for col, (label, _) in enumerate(GUITAR_STRINGS):
            tk.Label(
                grid_frame, text=label, width=4, bg=header_bg, fg="white", font=("", 10, "bold")
            ).grid(row=0, column=col + 1, padx=1, pady=1)
        tk.Label(grid_frame, text="", width=3, bg=header_bg).grid(row=0, column=0)

        for fret in range(0, MAX_FRET + 1):
            tk.Label(grid_frame, text=str(fret), width=3, bg=bg, fg=TEXT).grid(row=fret + 1, column=0)
            for col, (_, open_midi) in enumerate(GUITAR_STRINGS):
                name = note_name_at(open_midi, fret)
                # A clickable Label, not a tk.Button: macOS's native Aqua
                # rendering ignores -background on real buttons, which would
                # make every cell render plain gray instead of note-colored.
                btn = tk.Label(
                    grid_frame, text=name, width=4, bg=note_color(name), fg=TEXT,
                    relief="raised", borderwidth=1, cursor="pointinghand",
                )
                btn.bind("<Button-1>", lambda e, c=col, fr=fret: self._on_cell_click(c, fr))
                btn.grid(row=fret + 1, column=col + 1, padx=1, pady=1)
                self.buttons[col].append(btn)

        for col in range(len(GUITAR_STRINGS)):
            self._refresh_column(col)

        btn_row = tk.Frame(self, bg=bg)
        btn_row.pack(fill="x", padx=10, pady=(0, 10))
        mk_button(btn_row, "Cancel", self.destroy, "neutral").pack(side="right")
        mk_button(btn_row, "OK", self._on_ok, "custom").pack(side="right", padx=(0, 8))

        self.transient(parent)
        self.grab_set()

    def _tab_value(self, col):
        fret = self.selection[col]
        return "x" if fret is None else str(fret)

    def _on_cell_click(self, col, fret):
        if self.selection[col] == fret:
            self.selection[col] = None
        else:
            self.selection[col] = fret
        self.tab_vars[col].set(self._tab_value(col))
        self._refresh_column(col)

    def _on_tab_change(self, col):
        val = self.tab_vars[col].get()
        self.selection[col] = None if val == "x" else int(val)
        self._refresh_column(col)

    def _open_paste_tab(self):
        bg = SECTIONS["custom"]["tint"]
        dialog = tk.Toplevel(self)
        dialog.title("Paste Tab")
        dialog.resizable(False, False)
        dialog.configure(bg=bg)
        dialog.transient(self)
        dialog.grab_set()

        tk.Label(
            dialog,
            text="Paste a standard 6-line tab block for ONE chord shape\n"
                 "(top-to-bottom = high string to low string, e.g. e/B/G/D/A/E):",
            justify="left", bg=bg, fg=TEXT, font=FONT_BODY,
        ).pack(anchor="w", padx=10, pady=(10, 4))

        text_widget = tk.Text(dialog, width=40, height=8, font=("Courier", 11), bg="white", fg=TEXT)
        text_widget.pack(padx=10, pady=(0, 8))
        text_widget.insert("1.0", "e|--0--|\nB|--1--|\nG|--0--|\nD|--2--|\nA|--3--|\nE|-----|")
        text_widget.focus_set()

        def do_parse():
            try:
                parsed = parse_ascii_tab(text_widget.get("1.0", "end"))
            except ValueError as exc:
                messagebox.showerror("Couldn't parse tab", str(exc))
                return
            self.selection = parsed
            for col in range(len(GUITAR_STRINGS)):
                self.tab_vars[col].set(self._tab_value(col))
                self._refresh_column(col)
            dialog.destroy()

        btn_row = tk.Frame(dialog, bg=bg)
        btn_row.pack(fill="x", padx=10, pady=(0, 10))
        mk_button(btn_row, "Cancel", dialog.destroy, "neutral").pack(side="right")
        mk_button(btn_row, "Parse", do_parse, "custom").pack(side="right", padx=(0, 8))

    def _open_pdf_viewer(self):
        if not PDF_SUPPORT:
            messagebox.showerror(
                "PyMuPDF not installed", "Install it with: pip3 install pymupdf"
            )
            return
        PdfViewerWindow(self, title="Tab Reference")

    def _refresh_column(self, col):
        selected_fret = self.selection[col]
        for fret, btn in enumerate(self.buttons[col]):
            name = btn.cget("text").strip("[]")
            if fret == selected_fret:
                btn.configure(text=f"[{name}]", relief="sunken", borderwidth=3)
            else:
                btn.configure(text=name, relief="raised", borderwidth=1)

    def _on_ok(self):
        notes = []
        for col, (_, open_midi) in enumerate(GUITAR_STRINGS):
            fret = self.selection[col]
            if fret is not None:
                notes.append(open_midi + fret)
        if not notes:
            messagebox.showerror("No notes selected", "Select at least one string to play.")
            return
        name = self.name_var.get().strip()
        if not name:
            messagebox.showerror("Name required", "Give this chord a name.")
            return
        self.result = {"name": name, "notes": notes}
        self.destroy()


class FlashApp:
    def __init__(self, root):
        self.root = root
        root.title("RiffMIDI Board Programmer")
        # NOTE: resizable(False, False) is set at the END of __init__, not
        # here. Setting it before the window has its final size causes macOS
        # Tk to ignore any later root.geometry() call and snap the window
        # down to a tiny fallback size (observed: ~300x200) instead.
        root.protocol("WM_DELETE_WINDOW", self._on_close)
        root.configure(bg=APP_BG)

        style = ttk.Style()
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        style.configure("TCombobox", fieldbackground="white", padding=3)

        self.active_proc = None
        self.dfu_path = find_dfu_programmer()
        self.arduino_cli_path = find_arduino_cli()
        self.builtin_chord_names = parse_chord_names(INO_PATH)
        self.custom_chords = parse_custom_chords(CUSTOM_CHORDS_PATH)
        self.chord_names = []
        self._recompute_chord_names()
        self.all_buttons = []
        self.chord_sets = read_chord_sets(CHORD_SETS_PATH) or [
            {"name": "Set 1", **{f: 0 for _, f in BUTTON_SLOTS}}
        ]
        self.note_sets = read_note_sets(NOTE_SETS_PATH) or [
            {"name": "Lead", "green": "C", "red": "G", "yellow": "A", "blue": "F", "orange": "Bb"}
        ]

        pad = {"padx": 12, "pady": 6}

        # The app's content can get taller than a laptop screen (6 sections
        # plus the log box), and the window is non-resizable, so without
        # this, anything past the screen edge is simply unreachable -- no
        # scrollbar, no way to drag the window up. Wrap everything in a
        # scrollable canvas instead of packing straight onto root.
        outer = tk.Frame(root, bg=APP_BG)
        outer.pack(fill="both", expand=True)
        self._canvas = tk.Canvas(outer, bg=APP_BG, highlightthickness=0)
        scrollbar = tk.Scrollbar(outer, orient="vertical", command=self._canvas.yview)
        content = tk.Frame(self._canvas, bg=APP_BG)

        def _on_content_configure(event):
            self._canvas.configure(scrollregion=self._canvas.bbox("all"))
        content.bind("<Configure>", _on_content_configure)
        self._canvas.create_window((0, 0), window=content, anchor="nw")
        self._canvas.configure(yscrollcommand=scrollbar.set)
        self._canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        bind_mousewheel_scroll(self._canvas)

        header = tk.Label(
            content, text="🎸 RiffMIDI Board Programmer", bg=APP_BG, fg=TEXT,
            font=("Helvetica", 16, "bold"),
        )
        header.pack(anchor="w", padx=14, pady=(14, 2))

        # --- Custom chords ---
        custom_frame = mk_section(content, "1. Custom Chords", "custom")
        custom_frame.pack(fill="x", **pad)

        custom_list_row = mk_frame(custom_frame, "custom")
        custom_list_row.pack(fill="x")
        self.custom_listbox = tk.Listbox(
            custom_list_row, width=60, height=4, exportselection=False,
            bg="white", fg=TEXT, font=FONT_BODY, selectbackground=SECTIONS["custom"]["button"],
            highlightthickness=1, highlightbackground=SECTIONS["custom"]["accent"], relief="flat",
        )
        self.custom_listbox.pack(side="left", fill="x", expand=True)

        custom_btn_col = mk_frame(custom_list_row, "custom")
        custom_btn_col.pack(side="left", padx=(8, 0))
        self.add_custom_button = self._add_button(
            custom_btn_col, "Build Custom Chord...", self.add_custom_chord, "custom", fill="x", pady=2
        )
        self.delete_custom_button = self._add_button(
            custom_btn_col, "Delete Custom Chord", self.delete_custom_chord, "custom", fill="x", pady=2
        )

        self._refresh_custom_listbox()

        # --- Chord sets ---
        sets_frame = mk_section(content, "2. Chord Sets (Up/Down select cycles these on the board)", "sets")
        sets_frame.pack(fill="x", **pad)

        list_row = mk_frame(sets_frame, "sets")
        list_row.pack(fill="x")
        self.sets_listbox = tk.Listbox(
            list_row, width=60, height=6, exportselection=False,
            bg="white", fg=TEXT, font=FONT_BODY, selectbackground=SECTIONS["sets"]["button"],
            highlightthickness=1, highlightbackground=SECTIONS["sets"]["accent"], relief="flat",
        )
        self.sets_listbox.pack(side="left", fill="x", expand=True)

        set_btn_col = mk_frame(list_row, "sets")
        set_btn_col.pack(side="left", padx=(8, 0))
        self.add_set_button = self._add_button(set_btn_col, "Add Set", self.add_set, "sets", fill="x", pady=2)
        self.edit_set_button = self._add_button(set_btn_col, "Edit Set", self.edit_set, "sets", fill="x", pady=2)
        self.delete_set_button = self._add_button(set_btn_col, "Delete Set", self.delete_set, "sets", fill="x", pady=2)
        self.move_up_button = self._add_button(set_btn_col, "Move Up", self.move_set_up, "sets", fill="x", pady=2)
        self.move_down_button = self._add_button(set_btn_col, "Move Down", self.move_set_down, "sets", fill="x", pady=2)

        if not self.chord_names:
            mk_label(sets_frame, "Could not read chord list from RiffMIDI.ino.", "sets", fg="#B00020").pack(anchor="w")

        self.save_sets_button = self._add_button(
            sets_frame, "Save Sets to chord_sets.h", self.save_sets, "sets", anchor="w", pady=(8, 0)
        )

        self._refresh_sets_listbox()

        # --- Note sets ---
        notesets_frame = mk_section(content, "3. Note Sets (Ok/Back select cycles these on the board)", "notes")
        notesets_frame.pack(fill="x", **pad)

        notesets_list_row = mk_frame(notesets_frame, "notes")
        notesets_list_row.pack(fill="x")
        self.notesets_listbox = tk.Listbox(
            notesets_list_row, width=60, height=4, exportselection=False,
            bg="white", fg=TEXT, font=FONT_BODY, selectbackground=SECTIONS["notes"]["button"],
            highlightthickness=1, highlightbackground=SECTIONS["notes"]["accent"], relief="flat",
        )
        self.notesets_listbox.pack(side="left", fill="x", expand=True)

        noteset_btn_col = mk_frame(notesets_list_row, "notes")
        noteset_btn_col.pack(side="left", padx=(8, 0))
        self.add_noteset_button = self._add_button(noteset_btn_col, "Add Set", self.add_noteset, "notes", fill="x", pady=2)
        self.edit_noteset_button = self._add_button(noteset_btn_col, "Edit Set", self.edit_noteset, "notes", fill="x", pady=2)
        self.delete_noteset_button = self._add_button(noteset_btn_col, "Delete Set", self.delete_noteset, "notes", fill="x", pady=2)
        self.move_up_noteset_button = self._add_button(noteset_btn_col, "Move Up", self.move_noteset_up, "notes", fill="x", pady=2)
        self.move_down_noteset_button = self._add_button(noteset_btn_col, "Move Down", self.move_noteset_down, "notes", fill="x", pady=2)
        self.view_pdf_button = self._add_button(
            noteset_btn_col, "View PDF Tab...", self.open_pdf_viewer, "notes", fill="x", pady=2
        )

        self.save_notesets_button = self._add_button(
            notesets_frame, "Save Sets to note_sets.h", self.save_notesets, "notes", anchor="w", pady=(8, 0)
        )

        self._refresh_notesets_listbox()

        # --- Step 4: flash Arduino firmware ---
        step2_frame = mk_section(content, "4. Flash Arduino (USB-Serial) Firmware", "arduino")
        step2_frame.pack(fill="x", **pad)
        mk_label(step2_frame, "Puts the board in upload mode so the sketch can be sent to it.", "arduino").pack(anchor="w")
        self.flash_arduino_button = self._add_button(
            step2_frame, "Flash Arduino Firmware...", self.flash_arduino_firmware, "arduino", anchor="w", pady=(6, 0)
        )

        # --- Step 4: upload sketch ---
        step3_frame = mk_section(content, "5. Compile && Upload Sketch", "upload")
        step3_frame.pack(fill="x", **pad)

        port_row = mk_frame(step3_frame, "upload")
        port_row.pack(fill="x")
        mk_label(port_row, "Serial port:", "upload").pack(side="left")
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(port_row, textvariable=self.port_var, values=[], width=30)
        self.port_combo.pack(side="left", padx=(6, 6))
        self.refresh_ports_button = self._add_button(
            port_row, "Refresh Ports", self.refresh_ports, "upload", side="left"
        )

        self.upload_button = self._add_button(
            step3_frame, "Compile && Upload", self.upload_sketch, "upload", anchor="w", pady=(6, 0)
        )

        # --- Step 5: flash MIDI firmware ---
        step4_frame = mk_section(content, "6. Flash MIDI Firmware", "midi")
        step4_frame.pack(fill="x", **pad)
        mk_label(step4_frame, "Restores the board to a class-compliant MIDI controller.", "midi").pack(anchor="w")
        self.flash_midi_button = self._add_button(
            step4_frame, "Flash MIDI Firmware...", self.flash_midi_firmware, "midi", anchor="w", pady=(6, 0)
        )

        warnings = []
        if self.dfu_path is None:
            warnings.append("dfu-programmer not found on PATH.")
        if self.arduino_cli_path is None:
            warnings.append("arduino-cli not found on PATH (needed for step 5).")
        if not PDF_SUPPORT:
            warnings.append("PyMuPDF not installed (needed to view PDF tabs): pip3 install pymupdf")
        if not os.path.isfile(ARDUINO_HEX):
            warnings.append(f"Missing {os.path.basename(ARDUINO_HEX)}.")
        if not os.path.isfile(MIDI_HEX):
            warnings.append(f"Missing {os.path.basename(MIDI_HEX)}.")
        if warnings:
            tk.Label(
                content, text="\n".join(warnings), bg=APP_BG, fg="#B00020", font=FONT_BODY, justify="left"
            ).pack(anchor="w", **pad)

        tk.Label(content, text="Log:", bg=APP_BG, fg=TEXT, font=FONT_HEADER).pack(anchor="w", padx=12)
        self.log = scrolledtext.ScrolledText(
            content, width=80, height=18, state="disabled",
            bg="#1E1E2E", fg="#D4D4D4", insertbackground="#D4D4D4", font=FONT_MONO,
            relief="flat", padx=8, pady=8,
        )
        self.log.pack(padx=12, pady=(0, 14))

        self.refresh_ports()

        # Size the canvas itself (not just the window) to fit the content but
        # never taller than the screen -- the scrollbar handles the rest.
        # Without this, the canvas's own requested size just follows its
        # embedded content's full unclipped height, which fights any
        # root.geometry() call during the next layout pass and snaps the
        # window back to (too-tall) auto-sized dimensions.
        root.update_idletasks()
        content_width = content.winfo_reqwidth()
        screen_h = root.winfo_screenheight()
        canvas_height = min(content.winfo_reqheight(), screen_h - 80)
        self._canvas.configure(width=content_width, height=canvas_height)
        root.update_idletasks()
        root.geometry(f"{content_width + scrollbar.winfo_reqwidth() + 4}x{canvas_height + 4}")
        root.update_idletasks()
        root.resizable(False, False)

    def _add_button(self, parent, text, command, section="neutral", **pack_kwargs):
        btn = mk_button(parent, text, command, section)
        btn.pack(**pack_kwargs)
        self.all_buttons.append(btn)
        return btn

    # ---------- shared helpers ----------

    def append_log(self, text):
        self.log.configure(state="normal")
        self.log.insert(tk.END, text)
        self.log.see(tk.END)
        self.log.configure(state="disabled")

    def set_buttons_enabled(self, enabled):
        state = "normal" if enabled else "disabled"
        for btn in self.all_buttons:
            btn.configure(state=state)

    def _refresh_sets_listbox(self):
        self.sets_listbox.delete(0, tk.END)
        for i, s in enumerate(self.chord_sets):
            self.sets_listbox.insert(tk.END, format_set_summary(i, s, self.chord_names))

    def _selected_set_index(self):
        selection = self.sets_listbox.curselection()
        return selection[0] if selection else None

    def _refresh_notesets_listbox(self):
        self.notesets_listbox.delete(0, tk.END)
        for i, s in enumerate(self.note_sets):
            self.notesets_listbox.insert(tk.END, format_note_set_summary(i, s))

    def _selected_noteset_index(self):
        selection = self.notesets_listbox.curselection()
        return selection[0] if selection else None

    def _recompute_chord_names(self):
        self.chord_names = self.builtin_chord_names + [c["name"] for c in self.custom_chords]

    def _refresh_custom_listbox(self):
        self.custom_listbox.delete(0, tk.END)
        for i, c in enumerate(self.custom_chords):
            note_names = "-".join(NOTE_NAMES[n % 12] for n in c["notes"])
            self.custom_listbox.insert(tk.END, f"{c['name']} ({note_names})")

    # ---------- custom chords ----------

    def add_custom_chord(self):
        dialog = ChordBuilderDialog(self.root)
        self.root.wait_window(dialog)
        if dialog.result is None:
            return
        if dialog.result["name"] in self.chord_names:
            messagebox.showerror("Name already used", "Pick a different chord name.")
            return
        self.custom_chords.append(dialog.result)
        write_custom_chords(CUSTOM_CHORDS_PATH, self.custom_chords)
        self._recompute_chord_names()
        self._refresh_custom_listbox()
        self.append_log(f"\n=== Added custom chord '{dialog.result['name']}' to custom_chords.h ===\n")

    def delete_custom_chord(self):
        selection = self.custom_listbox.curselection()
        if not selection:
            messagebox.showerror("No chord selected", "Select a custom chord to delete.")
            return
        index = selection[0]
        is_last = index == len(self.custom_chords) - 1
        if not is_last:
            proceed = messagebox.askokcancel(
                "Deleting a non-last custom chord",
                "This isn't the last custom chord in the list. Deleting it will shift "
                "the index of every custom chord after it, which can silently change "
                "the chords in any existing chord set that references one of them.\n\n"
                "Continue anyway?",
            )
            if not proceed:
                return
        del self.custom_chords[index]
        write_custom_chords(CUSTOM_CHORDS_PATH, self.custom_chords)
        self._recompute_chord_names()
        self._refresh_custom_listbox()

    def _run_dfu_flash(self, hex_path, label):
        if self.dfu_path is None:
            messagebox.showerror("dfu-programmer not found", "Install it with: brew install dfu-programmer")
            return
        if not os.path.isfile(hex_path):
            messagebox.showerror("Missing file", f"Could not find {hex_path}")
            return

        proceed = messagebox.askokcancel(
            "Put board in DFU mode",
            "Press the DFU/reset button on the atmega16u2 now, then click OK.\n\nClick Cancel to abort.",
        )
        if not proceed:
            return

        self.set_buttons_enabled(False)
        self.append_log(f"\n=== Flashing '{label}' ({os.path.basename(hex_path)}) ===\n")
        threading.Thread(target=self._dfu_flash_thread, args=(hex_path, label), daemon=True).start()

    def _dfu_flash_thread(self, hex_path, label):
        shell_cmd = build_dfu_shell_cmd(self.dfu_path, hex_path)
        returncode, output = run_privileged_shell(shell_cmd)
        self.root.after(0, self._dfu_flash_done, returncode, output, label)

    def _dfu_flash_done(self, returncode, output, label):
        self.append_log(output if output else "(no output)\n")
        self.set_buttons_enabled(True)
        if returncode == 0:
            self.append_log(f"=== '{label}' flashed successfully ===\n")
            messagebox.showinfo("Done", f"'{label}' flashed successfully.")
        else:
            self.append_log(f"=== Flash FAILED (exit code {returncode}) ===\n")
            if "User canceled" not in output:
                messagebox.showerror("Flash failed", "Flashing failed. See log for details.")

    # ---------- step 1: manage chord sets ----------

    def add_set(self):
        if not self.chord_names:
            messagebox.showerror("Cannot add set", "Chord list wasn't loaded from RiffMIDI.ino.")
            return
        dialog = ChordSetDialog(self.root, self.chord_names, "Add Chord Set")
        self.root.wait_window(dialog)
        if dialog.result is not None:
            self.chord_sets.append(dialog.result)
            self._refresh_sets_listbox()
            self.sets_listbox.selection_set(len(self.chord_sets) - 1)

    def edit_set(self):
        index = self._selected_set_index()
        if index is None:
            messagebox.showerror("No set selected", "Select a chord set to edit.")
            return
        dialog = ChordSetDialog(self.root, self.chord_names, f"Edit Chord Set {index + 1}", self.chord_sets[index])
        self.root.wait_window(dialog)
        if dialog.result is not None:
            self.chord_sets[index] = dialog.result
            self._refresh_sets_listbox()
            self.sets_listbox.selection_set(index)

    def delete_set(self):
        index = self._selected_set_index()
        if index is None:
            messagebox.showerror("No set selected", "Select a chord set to delete.")
            return
        if len(self.chord_sets) <= 1:
            messagebox.showerror("Cannot delete", "At least one chord set is required.")
            return
        del self.chord_sets[index]
        self._refresh_sets_listbox()

    def move_set_up(self):
        index = self._selected_set_index()
        if index is None or index == 0:
            return
        self.chord_sets[index - 1], self.chord_sets[index] = self.chord_sets[index], self.chord_sets[index - 1]
        self._refresh_sets_listbox()
        self.sets_listbox.selection_set(index - 1)

    def move_set_down(self):
        index = self._selected_set_index()
        if index is None or index >= len(self.chord_sets) - 1:
            return
        self.chord_sets[index + 1], self.chord_sets[index] = self.chord_sets[index], self.chord_sets[index + 1]
        self._refresh_sets_listbox()
        self.sets_listbox.selection_set(index + 1)

    def save_sets(self):
        if not self.chord_names:
            messagebox.showerror("Cannot save", "Chord list wasn't loaded from RiffMIDI.ino.")
            return
        write_chord_sets(CHORD_SETS_PATH, self.chord_sets, self.chord_names)
        self.append_log(f"\n=== Wrote chord_sets.h with {len(self.chord_sets)} set(s) ===\n")
        messagebox.showinfo("Saved", "chord_sets.h updated with your chord sets.")

    # ---------- note sets (High buttons) ----------

    def add_noteset(self):
        dialog = NoteSetDialog(self.root, "Add Note Set")
        self.root.wait_window(dialog)
        if dialog.result is not None:
            self.note_sets.append(dialog.result)
            self._refresh_notesets_listbox()
            self.notesets_listbox.selection_set(len(self.note_sets) - 1)

    def edit_noteset(self):
        index = self._selected_noteset_index()
        if index is None:
            messagebox.showerror("No set selected", "Select a note set to edit.")
            return
        dialog = NoteSetDialog(self.root, f"Edit Note Set {index + 1}", self.note_sets[index])
        self.root.wait_window(dialog)
        if dialog.result is not None:
            self.note_sets[index] = dialog.result
            self._refresh_notesets_listbox()
            self.notesets_listbox.selection_set(index)

    def delete_noteset(self):
        index = self._selected_noteset_index()
        if index is None:
            messagebox.showerror("No set selected", "Select a note set to delete.")
            return
        if len(self.note_sets) <= 1:
            messagebox.showerror("Cannot delete", "At least one note set is required.")
            return
        del self.note_sets[index]
        self._refresh_notesets_listbox()

    def move_noteset_up(self):
        index = self._selected_noteset_index()
        if index is None or index == 0:
            return
        self.note_sets[index - 1], self.note_sets[index] = self.note_sets[index], self.note_sets[index - 1]
        self._refresh_notesets_listbox()
        self.notesets_listbox.selection_set(index - 1)

    def move_noteset_down(self):
        index = self._selected_noteset_index()
        if index is None or index >= len(self.note_sets) - 1:
            return
        self.note_sets[index + 1], self.note_sets[index] = self.note_sets[index], self.note_sets[index + 1]
        self._refresh_notesets_listbox()
        self.notesets_listbox.selection_set(index + 1)

    def save_notesets(self):
        write_note_sets(NOTE_SETS_PATH, self.note_sets)
        self.append_log(f"\n=== Wrote note_sets.h with {len(self.note_sets)} set(s) ===\n")
        messagebox.showinfo("Saved", "note_sets.h updated with your note sets.")

    def open_pdf_viewer(self):
        if not PDF_SUPPORT:
            messagebox.showerror(
                "PyMuPDF not installed", "Install it with: pip3 install pymupdf"
            )
            return
        PdfViewerWindow(self.root, title="Tab Reference")

    # ---------- step 2: flash Arduino firmware ----------

    def flash_arduino_firmware(self):
        self._run_dfu_flash(ARDUINO_HEX, "Arduino (USB-Serial) Firmware")

    # ---------- step 3: upload sketch ----------

    def refresh_ports(self):
        if self.arduino_cli_path is None:
            return
        ports = list_serial_ports(self.arduino_cli_path)
        self.port_combo.configure(values=ports)
        if ports and self.port_var.get() not in ports:
            self.port_var.set(ports[0])

    def upload_sketch(self):
        if self.arduino_cli_path is None:
            messagebox.showerror("arduino-cli not found", "Install it with: brew install arduino-cli")
            return
        port = self.port_var.get().strip()
        if not port:
            messagebox.showerror("No port selected", "Pick the board's serial port (use Refresh Ports if needed).")
            return

        self.set_buttons_enabled(False)
        self.append_log(f"\n=== Compiling and uploading sketch to {port} ===\n")
        threading.Thread(target=self._upload_thread, args=(port,), daemon=True).start()

    def _upload_thread(self, port):
        cmd = [
            self.arduino_cli_path, "compile",
            "--fqbn", FQBN,
            "--library", CONTROL_SURFACE_LIB,
            "--build-path", BUILD_CACHE_DIR,
            "--upload", "-p", port,
            SCRIPT_DIR,
        ]
        try:
            returncode, output = run_with_timeout_killing_children(
                cmd, timeout=180, on_start=self._set_active_proc
            )
            self.root.after(0, self._upload_done, returncode, output)
        except Exception as exc:  # noqa: BLE001
            self.root.after(0, self._upload_done, -1, str(exc))
        finally:
            self._set_active_proc(None)

    def _set_active_proc(self, proc):
        self.active_proc = proc

    def _on_close(self):
        if self.active_proc is not None:
            proceed = messagebox.askokcancel(
                "Operation in progress",
                "A compile/upload is still running. Closing now will kill it "
                "(and any child process it spawned, like avrdude). Continue?",
            )
            if not proceed:
                return
            kill_process_group(self.active_proc)
        self.root.destroy()

    def _upload_done(self, returncode, output):
        self.append_log(output if output else "(no output)\n")
        self.set_buttons_enabled(True)
        if returncode == 0:
            self.append_log("=== Sketch uploaded successfully ===\n")
            messagebox.showinfo("Done", "Sketch uploaded successfully.")
        else:
            self.append_log("=== Compile/upload FAILED ===\n")
            messagebox.showerror("Upload failed", "Compile/upload failed. See log for details.")

    # ---------- step 4: flash MIDI firmware ----------

    def flash_midi_firmware(self):
        self._run_dfu_flash(MIDI_HEX, "MIDI Firmware")


def main():
    root = tk.Tk()
    FlashApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
