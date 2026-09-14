#!/usr/bin/env python3
"""GUI to flash the Arduino Mega's atmega16u2 with either the stock
Arduino USB-serial firmware or the MIDI (HIDUINO-style) firmware,
using dfu-programmer.
"""

import os
import shlex
import shutil
import subprocess
import threading
import tkinter as tk
from tkinter import messagebox, scrolledtext

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

FIRMWARES = {
    "Arduino (USB-Serial) Firmware": "Arduino-usbserial-atmega16u2-Mega2560-Rev3.hex",
    "MIDI Firmware": "arduino_midi.hex",
}


def find_dfu_programmer():
    path = shutil.which("dfu-programmer")
    if path:
        return path
    for candidate in ("/opt/homebrew/bin/dfu-programmer", "/usr/local/bin/dfu-programmer"):
        if os.path.isfile(candidate):
            return candidate
    return None


class FlashApp:
    def __init__(self, root):
        self.root = root
        root.title("Arduino 16u2 Flasher")
        root.resizable(False, False)

        self.dfu_path = find_dfu_programmer()

        pad = {"padx": 12, "pady": 6}

        tk.Label(root, text="Select firmware to flash:", font=("", 12, "bold")).pack(
            anchor="w", **pad
        )

        self.selection = tk.StringVar(value=list(FIRMWARES.keys())[0])
        for label in FIRMWARES:
            tk.Radiobutton(root, text=label, variable=self.selection, value=label).pack(
                anchor="w", padx=24
            )

        if self.dfu_path is None:
            tk.Label(
                root,
                text="Warning: dfu-programmer not found on PATH.",
                fg="red",
            ).pack(anchor="w", **pad)

        missing = [f for f in FIRMWARES.values() if not os.path.isfile(os.path.join(SCRIPT_DIR, f))]
        if missing:
            tk.Label(
                root,
                text="Missing hex file(s): " + ", ".join(missing),
                fg="red",
            ).pack(anchor="w", **pad)

        self.flash_button = tk.Button(
            root, text="Flash Board...", command=self.start_flash, width=20
        )
        self.flash_button.pack(**pad)

        tk.Label(root, text="Log:").pack(anchor="w", padx=12)
        self.log = scrolledtext.ScrolledText(root, width=70, height=16, state="disabled")
        self.log.pack(padx=12, pady=(0, 12))

    def append_log(self, text):
        self.log.configure(state="normal")
        self.log.insert(tk.END, text)
        self.log.see(tk.END)
        self.log.configure(state="disabled")

    def start_flash(self):
        if self.dfu_path is None:
            messagebox.showerror("dfu-programmer not found", "Install it with: brew install dfu-programmer")
            return

        label = self.selection.get()
        hex_filename = FIRMWARES[label]
        hex_path = os.path.join(SCRIPT_DIR, hex_filename)
        if not os.path.isfile(hex_path):
            messagebox.showerror("Missing file", f"Could not find {hex_filename} in {SCRIPT_DIR}")
            return

        proceed = messagebox.askokcancel(
            "Put board in DFU mode",
            "Press the DFU/reset button on the atmega16u2 now "
            "(so the board enumerates in DFU mode), then click OK to continue.\n\n"
            "Click Cancel to abort.",
        )
        if not proceed:
            return

        self.flash_button.configure(state="disabled")
        self.append_log(f"\n=== Flashing '{label}' ({hex_filename}) ===\n")

        thread = threading.Thread(target=self._run_flash, args=(hex_path, label), daemon=True)
        thread.start()

    def _run_flash(self, hex_path, label):
        dfu = self.dfu_path
        shell_cmd = (
            f"{shlex.quote(dfu)} atmega16u2 erase && "
            f"{shlex.quote(dfu)} atmega16u2 flash {shlex.quote(hex_path)} && "
            f"{shlex.quote(dfu)} atmega16u2 reset"
        )

        applescript_string = shell_cmd.replace("\\", "\\\\").replace('"', '\\"')
        osascript_cmd = [
            "osascript",
            "-e",
            f'do shell script "{applescript_string}" with administrator privileges',
        ]

        try:
            result = subprocess.run(
                osascript_cmd, capture_output=True, text=True, timeout=120
            )
            output = (result.stdout or "") + (result.stderr or "")
            self.root.after(0, self._flash_done, result.returncode, output, label)
        except subprocess.TimeoutExpired:
            self.root.after(0, self._flash_done, -1, "Timed out waiting for dfu-programmer.", label)
        except Exception as exc:  # noqa: BLE001
            self.root.after(0, self._flash_done, -1, str(exc), label)

    def _flash_done(self, returncode, output, label):
        self.append_log(output if output else "(no output)\n")
        self.flash_button.configure(state="normal")
        if returncode == 0:
            self.append_log(f"=== '{label}' flashed successfully ===\n")
            messagebox.showinfo("Done", f"'{label}' flashed successfully.")
        else:
            self.append_log(f"=== Flash FAILED (exit code {returncode}) ===\n")
            if "User canceled" not in output:
                messagebox.showerror("Flash failed", f"Flashing failed. See log for details.")


def main():
    root = tk.Tk()
    FlashApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
