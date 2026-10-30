#include <stdint.h>
uint32_t SystemCoreClock = 180000000UL;
void SystemInit(void) {
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10*2)) | (3UL << (11*2)));
  #endif
}
