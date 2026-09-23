#include "phoenix.h"

tcb_t *phoenix_current = NULL;

typedef struct { tcb_t *head, *tail; } rq_t;

static rq_t     ready[PHOENIX_MAX_PRIORITIES];
static uint32_t ready_bitmap = 0;
static tcb_t   *all_tasks[PHOENIX_MAX_TASKS];
static int      num_tasks = 0;
static tcb_t    idle_tcb;
static stack_t  idle_stack[PHOENIX_IDLE_STACK_SIZE];

/* ---------- Critical Sections ---------- */
uint32_t phoenix_enter_critical(void) {
    uint32_t pm;
    __asm volatile ("mrs %0, primask" : "=r"(pm));
    __asm volatile ("cpsid i" ::: "memory");
    return pm;
}
void phoenix_exit_critical(uint32_t pm) {
    __asm volatile ("msr primask, %0" :: "r"(pm) : "memory");
}

/* ---------- Ready Queue Internals ---------- */
static void rq_push(rq_t *q, tcb_t *t) {
    t->prev = q->tail; t->next = NULL;
    if (q->tail) q->tail->next = t; else q->head = t;
    q->tail = t;
}
static void rq_remove(rq_t *q, tcb_t *t) {
    if (t->prev) t->prev->next = t->next; else q->head = t->next;
    if (t->next) t->next->prev = t->prev; else q->tail = t->prev;
    t->prev = t->next = NULL;
}
static void rq_push_b(tcb_t *t) {
    rq_push(&ready[t->priority], t);
    ready_bitmap |= (1u << t->priority);
}
static void rq_remove_b(tcb_t *t) {
    rq_remove(&ready[t->priority], t);
    if (!ready[t->priority].head) ready_bitmap &= ~(1u << t->priority);
}
static tcb_t *pick_next(void) {
    if (ready_bitmap == 0) return &idle_tcb;
    uint8_t p = (uint8_t)__builtin_ctz(ready_bitmap);
    return ready[p].head;
}

/* ---------- Public helpers ---------- */
void phoenix_ready_add(tcb_t *t) { t->state = TASK_READY; rq_push_b(t); }
void phoenix_ready_remove(tcb_t *t) { rq_remove_b(t); }

/* ---------- Stack init ---------- */
static stack_t *task_stack_init(stack_t *top, void (*entry)(void*), void *arg) {
    stack_t *sp = (stack_t *)((uintptr_t)top & ~7UL);
    *(--sp) = 0x01000000;
    *(--sp) = (stack_t)entry;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0; *(--sp) = 0; *(--sp) = 0;
    *(--sp) = (stack_t)arg;
    for (int i = 0; i < 8; i++) *(--sp) = 0;
    return sp;
}

/* ---------- Idle task ---------- */
static void idle_task(void *arg) {
    (void)arg;
    while (1) __asm volatile ("wfi");
}

/* ---------- Kernel API ---------- */
void phoenix_init(void) {
    for (int i = 0; i < PHOENIX_MAX_PRIORITIES; i++)
        ready[i].head = ready[i].tail = NULL;
    ready_bitmap = 0;
    num_tasks = 0;
    phoenix_current = NULL;

    idle_tcb.sp = task_stack_init(&idle_stack[PHOENIX_IDLE_STACK_SIZE],
                                  idle_task, NULL);
    idle_tcb.priority = PHOENIX_MAX_PRIORITIES - 1;
    idle_tcb.state = TASK_READY;
    idle_tcb.ticks_remaining = 0;
    idle_tcb.prev = idle_tcb.next = NULL;
    idle_tcb.name = "idle";
    rq_push_b(&idle_tcb);
}

int phoenix_task_create(const char *name, void (*entry)(void *), void *arg,
                        uint8_t priority, stack_t *stack, uint32_t stack_words) {
    if (num_tasks >= PHOENIX_MAX_TASKS - 1) return PHOENIX_ERR;
    if (priority >= PHOENIX_MAX_PRIORITIES - 1) return PHOENIX_ERR_PARAM;

    uint32_t ps = phoenix_enter_critical();
    tcb_t *t = (tcb_t *)stack;
    stack_t *stk_top = stack + stack_words;

    t->sp = task_stack_init(stk_top, entry, arg);
    t->priority = priority;
    t->state = TASK_READY;
    t->ticks_remaining = 0;
    t->prev = t->next = NULL;
    t->name = name;

    all_tasks[num_tasks++] = t;
    rq_push_b(t);
    phoenix_exit_critical(ps);

    if (phoenix_current) phoenix_pendsv_trigger();
    return PHOENIX_OK;
}

void phoenix_start(void) {
    *(volatile uint8_t *)0xE000ED20 = 0xFF;
    *(volatile uint8_t *)0xE000ED22 = 0xFF;

    volatile uint32_t *SYST_CSR = (uint32_t *)0xE000E010;
    volatile uint32_t *SYST_RVR = (uint32_t *)0xE000E014;
    volatile uint32_t *SYST_CVR = (uint32_t *)0xE000E018;
    *SYST_RVR = (SYSTEM_CLOCK_HZ / PHOENIX_TICK_RATE_HZ) - 1;
    *SYST_CVR = 0;
    *SYST_CSR = 0x07;

    phoenix_current = pick_next();
    phoenix_current->state = TASK_RUNNING;

    __asm volatile ("svc 0");
    while (1) {}
}

void phoenix_yield(void) { phoenix_pendsv_trigger(); }

void phoenix_delay(uint32_t ticks) {
    uint32_t ps = phoenix_enter_critical();
    phoenix_current->ticks_remaining = ticks;
    phoenix_current->state = TASK_BLOCKED;
    rq_remove_b(phoenix_current);
    phoenix_exit_critical(ps);
    phoenix_pendsv_trigger();
}

void phoenix_schedule(void) {
    tcb_t *cur = phoenix_current;
    if (cur && cur->state == TASK_RUNNING) {
        rq_remove(&ready[cur->priority], cur);
        rq_push(&ready[cur->priority], cur);
    }
    tcb_t *next = pick_next();
    next->state = TASK_RUNNING;
    phoenix_current = next;
}

__attribute__((weak)) void SysTick_Handler(void) {
    phoenix_sched_tick();
}

void phoenix_sched_tick(void) {
    uint32_t ps = phoenix_enter_critical();
    for (int i = 0; i < num_tasks; i++) {
        tcb_t *t = all_tasks[i];
        if (t && t->state == TASK_BLOCKED && t->ticks_remaining > 0) {
            if (--t->ticks_remaining == 0) phoenix_ready_add(t);
        }
    }
    phoenix_exit_critical(ps);
    phoenix_pendsv_trigger();
}
