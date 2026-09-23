    .syntax unified
    .cpu cortex-m3
    .thumb
    .text

    .global phoenix_pendsv_trigger
    .type phoenix_pendsv_trigger, %function
phoenix_pendsv_trigger:
    LDR     R0, =0xE000ED04
    LDR     R1, =0x10000000
    STR     R1, [R0]
    DSB
    ISB
    BX      LR
    .size phoenix_pendsv_trigger, .-phoenix_pendsv_trigger

    .global PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
    MRS     R0, PSP
    CBZ     R0, PendSV_restore
    STMFD   R0!, {R4-R11}
    LDR     R1, =phoenix_current
    LDR     R1, [R1]
    STR     R0, [R1]

PendSV_restore:
    PUSH    {R3, LR}
    BL      phoenix_schedule
    POP     {R3, LR}
    LDR     R1, =phoenix_current
    LDR     R0, [R1]
    LDR     R0, [R0]
    LDMFD   R0!, {R4-R11}
    MSR     PSP, R0
    ORR     LR, LR, #0x04
    BX      LR
    .size PendSV_Handler, .-PendSV_Handler

    .global SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    LDR     R0, =phoenix_current
    LDR     R0, [R0]
    LDR     R0, [R0]
    LDMFD   R0!, {R4-R11}
    MSR     PSP, R0
    ORR     LR, LR, #0x04
    BX      LR
    .size SVC_Handler, .-SVC_Handler
