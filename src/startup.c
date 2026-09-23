#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;

int  main(void);
void Reset_Handler(void);
void Default_Handler(void);
void PendSV_Handler(void);
void SVC_Handler(void);
void SysTick_Handler(void);

__attribute__((section(".vectors"), used))
const void *vectors[] = {
    &_estack,
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    Default_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler
};

void Reset_Handler(void) {
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    main();
    while (1) {}
}

void Default_Handler(void) {
    while (1) {}
}
