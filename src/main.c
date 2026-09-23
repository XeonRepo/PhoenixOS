#include "phoenix.h"

/* ---- mps2-an385 UART0 ---- */
#define UART0_BASE   0x40004000UL
#define UART0_DATA   (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_STATE  (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART0_CTRL   (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART0_BAUD   (*(volatile uint32_t *)(UART0_BASE + 0x10))

static void uart_init(void) {
    UART0_BAUD = 25;
    UART0_CTRL = 0x01;
}
static void uart_putc(char c) {
    while (UART0_STATE & 0x01) {}
    UART0_DATA = (uint32_t)c;
}
static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

/* ---- Boot logo (text only) ---- */
static const char *logo[] = {
    "",
    "  ==========================================================",
    "  ||                                                      ||",
    "  ||    ____  _   _  ___  _____ _   _ ___  __            ||",
    "  ||   |  _ \\| | | |/ _ \\| ____| \\ | |_ _| \\ \\           ||",
    "  ||   | |_) | |_| | | | |  _| |  \\| || |   \\ \\          ||",
    "  ||   |  __/|  _  | |_| | |___| |\\  || |   / /          ||",
    "  ||   |_|   |_| |_|\\___/|_____|_| \\_|___| /_/           ||",
    "  ||                                                      ||",
    "  ||         R E A L - T I M E   K E R N E L              ||",
    "  ||            for ARM Cortex-M3                         ||",
    "  ||                                                      ||",
    "  ||                Version 0.2                           ||",
    "  ||            IMS Engineering College                   ||",
    "  ||                                                      ||",
    "  ==========================================================",
    "",
    NULL
};

static void print_logo(void) {
    for (int i = 0; logo[i] != NULL; i++) {
        uart_puts(logo[i]);
        uart_puts("\n");
    }
}

/* ---- Task stacks ---- */
static stack_t t_producer_stack[256];
static stack_t t_consumer_stack[256];
static stack_t t_heartbeat_stack[256];
static stack_t t_low_stack[256];
static stack_t t_high_stack[256];

/* ---- Sync primitives ---- */
static phoenix_sem_t   sem;
static phoenix_mutex_t mtx;

/* ============================================================
 *  PRODUCER / CONSUMER using SEMAPHORE
 * ============================================================ */
static void task_producer(void *arg) {
    (void)arg;
    while (1) {
        uart_puts("[PRODUCER] posting semaphore\n");
        phoenix_sem_post(&sem);
        phoenix_delay(500);
    }
}

static void task_consumer(void *arg) {
    (void)arg;
    while (1) {
        uart_puts("[CONSUMER] waiting...\n");
        phoenix_sem_wait(&sem);
        uart_puts("[CONSUMER] got signal!\n");
    }
}

/* ============================================================
 *  MUTEX DEMO — Low & High priority both use shared resource
 * ============================================================ */
static void task_low_mutex(void *arg) {
    (void)arg;
    while (1) {
        uart_puts("[LOW ] trying to lock mutex...\n");
        phoenix_mutex_lock(&mtx);
        uart_puts("[LOW ] acquired mutex — critical section\n");
        for (volatile int i = 0; i < 200000; i++) {}   /* simulate work */
        uart_puts("[LOW ] releasing mutex\n");
        phoenix_mutex_unlock(&mtx);
        phoenix_delay(800);
    }
}

static void task_high_mutex(void *arg) {
    (void)arg;
    while (1) {
        uart_puts("[HIGH] trying to lock mutex...\n");
        phoenix_mutex_lock(&mtx);
        uart_puts("[HIGH] acquired mutex — critical section\n");
        for (volatile int i = 0; i < 100000; i++) {}
        uart_puts("[HIGH] releasing mutex\n");
        phoenix_mutex_unlock(&mtx);
        phoenix_delay(600);
    }
}

/* ---- Heartbeat ---- */
static void task_heartbeat(void *arg) {
    (void)arg;
    while (1) {
        uart_puts("[HEARTBEAT] alive\n");
        phoenix_delay(1000);
    }
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    uart_init();
    print_logo();

    uart_puts("[BOOT] Initializing PhoenixOS kernel...\n");
    phoenix_init();

    uart_puts("[BOOT] Initializing semaphore and mutex...\n");
    phoenix_sem_init(&sem, 0);
    phoenix_mutex_init(&mtx);

    uart_puts("[BOOT] Creating tasks...\n");
    /* Semaphore demo tasks */
    phoenix_task_create("consumer", task_consumer,    NULL, 2, t_consumer_stack,  256);
    phoenix_task_create("producer", task_producer,    NULL, 3, t_producer_stack,  256);

    /* Mutex demo tasks */
    phoenix_task_create("high",     task_high_mutex,  NULL, 1, t_high_stack,      256);
    phoenix_task_create("low",      task_low_mutex,   NULL, 4, t_low_stack,       256);

    /* Heartbeat */
    phoenix_task_create("heartbeat",task_heartbeat,   NULL, 5, t_heartbeat_stack, 256);

    uart_puts("[BOOT] Starting scheduler...\n\n");
    phoenix_start();
    while (1) {}
}
