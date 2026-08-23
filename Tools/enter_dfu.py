#!/usr/bin/env python3
"""Reboots the NanoGaN board into its USB DFU bootloader.

Sends the "dfu" command to every serial port exposed by the board's
TinyUSB CDC interfaces (VID:PID = CAFE:4001). Only the User CDC
interface (see Core/Src/core_tasks.c) understands the command; bytes
sent to the Scrutiny CDC interface are harmlessly ignored. Matching by
VID/PID alone (instead of the OS-specific USB interface number) keeps
this script portable across Windows/Linux/macOS.
"""
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial is required: pip install pyserial")

VID = 0xCAFE
PID = 0x4001


def main():
    ports = [p for p in list_ports.comports() if p.vid == VID and p.pid == PID]
    if not ports:
        sys.exit(
            f"No NanoGaN CDC ports found (VID={VID:04X}:PID={PID:04X}). "
            "Is the board connected and running the application (not already in DFU mode)?"
        )

    sent = False
    for p in ports:
        try:
            with serial.Serial(p.device, 115200, timeout=0.5) as ser:
                ser.write(b"dfu\n")
                ser.flush()
            print(f"Sent DFU request on {p.device}")
            sent = True
        except serial.SerialException as e:
            print(f"Could not open {p.device}: {e}")

    if not sent:
        sys.exit("Failed to send the DFU request on any port.")

    print("Waiting for the device to re-enumerate in DFU mode...")
    time.sleep(2)


if __name__ == "__main__":
    main()
