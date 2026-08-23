#include "bootloader.h"
#include "main.h"

/* STM32H723/733 system memory base address (AN2606, USB DFU boot loader). */
#define SYSMEM_BOOTLOADER_ADDR 0x1FF09800UL

/* Where our own vector table actually lives (see STM32H723XG_FLASH.ld). */
#define APP_VECTOR_TABLE_ADDR 0x08000000UL

#define BOOT_MAGIC 0xB00710ADu

/* RTC backup register: the backup domain is explicitly guaranteed by ST to
   survive any reset (NVIC_SystemReset() included) as long as power stays on,
   unlike plain RAM/DTCM whose retention across a software system reset isn't
   guaranteed. Used here to carry the "jump to DFU" request across the reset. */
static void backup_domain_enable(void)
{
    __HAL_RCC_RTC_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

void Bootloader_RequestEntry(void)
{
    backup_domain_enable();
    RTC->BKP0R = BOOT_MAGIC;

    __disable_irq();
    NVIC_SystemReset();
    while (1) { }
}

void Bootloader_Reboot(void)
{
    __disable_irq();
    NVIC_SystemReset();
    while (1) { }
}

void Bootloader_CheckAndJump(void)
{
    /* Guard against VTOR still pointing at the DFU ROM's vector table when we
       get here via its "leave DFU" jump back into the app (it never resets
       VTOR for us) — otherwise every interrupt (SysTick, USB, timers...)
       would dispatch through the wrong vector table. */
    SCB->VTOR = APP_VECTOR_TABLE_ADDR;

    backup_domain_enable();
    if (RTC->BKP0R != BOOT_MAGIC) return;
    RTC->BKP0R = 0;

    HAL_RCC_DeInit();
    HAL_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    __disable_irq();
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    /* Point interrupts at the DFU ROM's own vector table before handing it
       control — its USB/SysTick ISRs must not dispatch through ours. */
    SCB->VTOR = SYSMEM_BOOTLOADER_ADDR;
    __enable_irq();

    void (*bootEntry)(void) = (void (*)(void))(*(__IO uint32_t *)(SYSMEM_BOOTLOADER_ADDR + 4));

    __set_MSP(*(__IO uint32_t *)SYSMEM_BOOTLOADER_ADDR);
    bootEntry();

    while (1) { }
}
