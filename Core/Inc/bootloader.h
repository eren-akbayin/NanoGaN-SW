#pragma once

/* Call as the very first thing in main(), before HAL_Init()/anything else.
   If a bootloader request is pending, this jumps into the system memory
   DFU bootloader and never returns; otherwise it returns immediately. */
extern void Bootloader_CheckAndJump(void);

/* Marks a bootloader request and performs a real system reset.
   Never returns. */
extern void Bootloader_RequestEntry(void);

/* Plain reset back into the application (no DFU flag set).
   Never returns. */
extern void Bootloader_Reboot(void);
