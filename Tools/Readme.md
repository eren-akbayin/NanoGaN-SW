# STM32CubeProgrammer CLI Setup (Linux)

Steps to get `STM32_Programmer_CLI` working on a fresh Linux machine, including
the parts that commonly go wrong (PATH not applying to VS Code tasks, USB
permissions).

## 1. Install

Download `en.stm32cubeprg-lin-<version>.zip` from
[st.com](https://www.st.com/en/development-tools/stm32cubeprog.html)
(free account required), then:

```bash
unzip en.stm32cubeprg-lin-*.zip
cd STM32CubeProgrammer*/
./SetupSTM32CubeProgrammer-*.linux
```

Add `-console` for a headless/text-mode install.

**Note the install path.** It depends on who ran the installer:

| Run as | Install path |
|---|---|
| root / sudo | `/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/` |
| normal user | `~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/` |

The rest of this document assumes the `/usr/local` path. Adjust if yours differs.

Also make sure libusb is present:

```bash
sudo apt install libusb-1.0-0
```

## 2. Make the CLI available everywhere

Symlink into `/usr/local/bin`, which is already on the default system PATH:

```bash
sudo ln -s /usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI /usr/local/bin/
```

This works from interactive shells, VS Code tasks, Makefiles and under `sudo`.

If it later complains about missing shared libraries, symlink the wrapper
script instead — it sets `LD_LIBRARY_PATH` for the bundled `.so` files:

```bash
sudo rm /usr/local/bin/STM32_Programmer_CLI
sudo ln -s /usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer.sh /usr/local/bin/STM32_Programmer_CLI
```

<details>
<summary>Alternative: PATH via <code>~/.bashrc</code></summary>

```bash
echo 'export PATH=$PATH:/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin' >> ~/.bashrc
source ~/.bashrc
```

Two caveats:

- `>>` only writes the file. The **current** shell does not re-read it —
  run `source ~/.bashrc` or open a new terminal.
- `~/.bashrc` is only read by *interactive bash*. VS Code tasks run under
  `bash -c` (non-interactive) and will still report `command not found`.
  For those, use the symlink above, an absolute path, or a per-task
  `options.env.PATH` entry in `tasks.json`.

</details>

## 3. udev rules (required for non-root USB access)

```bash
sudo cp /usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/Drivers/rules/*.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

**Then unplug and replug the board.** Rules are applied at enumeration time and
do not affect an already-connected device.

## 4. Verify

```bash
lsusb | grep 0483
STM32_Programmer_CLI -l usb
```

Expected USB IDs:

- `0483:df11` — STM32 in DFU / bootloader mode
- `0483:374x` — ST-LINK debug probe

## 5. Flash

```bash
# DFU — raw binary, address required
STM32_Programmer_CLI -c port=usb1 -w firmware.bin 0x08000000 -v -s

# DFU — .elf / .hex carry their own load address, so omit it
STM32_Programmer_CLI -c port=usb1 -w firmware.elf -v -s

# SWD via ST-LINK
STM32_Programmer_CLI -c port=SWD -w firmware.elf -v -rst

# Mass erase
STM32_Programmer_CLI -c port=SWD -e all

# Option bytes
STM32_Programmer_CLI -c port=SWD -ob displ
```

Useful `-c` modifiers: `mode=UR` (connect under reset), `mode=HOTPLUG`,
`freq=4000`, `sn=<probe serial>` when several probes are attached.

## Troubleshooting

**`command not found` in a VS Code task but not in the terminal**
Non-interactive shells skip `~/.bashrc`. Use the symlink from step 2, or set
the full path in `tasks.json`:

```json
"command": "/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI"
```

**Device not detected in DFU mode**
Check `lsusb | grep 0483` first — that splits the problem in two:

- *Nothing listed:* the MCU is not in bootloader mode, or it's a cable/port
  issue. Use the USB port wired to the MCU (not the ST-LINK port), try a
  known-good data cable, and enter DFU by holding BOOT0 high, pulsing NRST,
  then releasing BOOT0. On some parts the `nBOOT0` / `BOOT_LOCK` option bytes
  must be set for BOOT0 to have any effect.
- *`0483:df11` listed but the CLI finds nothing:* permissions. Redo step 3.

**`sudo STM32_Programmer_CLI -l usb` works, plain does not**
Definitively a udev issue. Check the device node ownership using the bus and
device numbers from `lsusb`:

```bash
ls -l /dev/bus/usb/001/029
```

If ST's rules use `GROUP="plugdev"` (which Ubuntu 22.04+ does not create for
desktop users by default), either join the group:

```bash
sudo usermod -aG plugdev $USER   # log out and back in
```

or add a permissive rule of your own:

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="df11", MODE="0666"' | sudo tee /etc/udev/rules.d/49-stm32-dfu.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```

**`cp: cannot stat '.../Drivers/rules/*.rules'`**
The install path is wrong. Locate the real one:

```bash
find / -name "STM32_Programmer_CLI" -type f 2>/dev/null
```