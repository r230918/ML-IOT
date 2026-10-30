#include <stdint.h>
#include "stm32f4xx_hal.h"

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern void SystemInit(void);
extern int main(void);
extern void SysTick_Handler(void);
extern void Default_Handler(void);

void Reset_Handler(void);

__attribute__((weak)) void NMI_Handler(void) { while(1); }
__attribute__((weak)) void HardFault_Handler(void) { while(1); }
__attribute__((weak)) void MemManage_Handler(void) { while(1); }
__attribute__((weak)) void BusFault_Handler(void) { while(1); }
__attribute__((weak)) void UsageFault_Handler(void) { while(1); }
__attribute__((weak)) void SVC_Handler(void) {}
__attribute__((weak)) void DebugMon_Handler(void) {}
__attribute__((weak)) void PendSV_Handler(void) {}
__attribute__((weak)) void Default_Handler(void) { while(1); }

/* Vector table aligned to 512 bytes */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))((uint32_t)&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
};

void Reset_Handler(void) {
    uint32_t *pSrc = &_sidata;
    uint32_t *pDest = &_sdata;

    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    SystemInit();
    main();

    while(1);
}
