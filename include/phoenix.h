#ifndef PHOENIX_H
#define PHOENIX_H

#include <stdint.h>
#include <stddef.h>
#include "phoenix_config.h"

typedef enum {
    PHOENIX_OK = 0,
    PHOENIX_ERR,
    PHOENIX_ERR_PARAM,
    PHOENIX_ERR_BUSY
} phoenix_status_t;

typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED
} task_state_t;

typedef uint32_t stack_t;

typedef struct tcb {
    stack_t        *sp;
    uint8_t         priority;
    task_state_t    state;
    uint32_t        ticks_remaining;
    struct tcb     *prev;
    struct tcb     *next;
    const char     *name;
} tcb_t;

void phoenix_init(void);
int  phoenix_task_create(const char *name, void (*entry)(void *),
                         void *arg, uint8_t priority,
                         stack_t *stack, uint32_t stack_words);
void phoenix_start(void);
void phoenix_yield(void);
void phoenix_delay(uint32_t ticks);

uint32_t phoenix_enter_critical(void);
void     phoenix_exit_critical(uint32_t primask);
void     phoenix_ready_add(tcb_t *t);
void     phoenix_ready_remove(tcb_t *t);
void     phoenix_schedule(void);
void     phoenix_sched_tick(void);
extern void phoenix_pendsv_trigger(void);
extern tcb_t *phoenix_current;

/* ---- Counting Semaphore ---- */
typedef struct {
    int32_t  count;
    tcb_t   *wait_head;
    tcb_t   *wait_tail;
} phoenix_sem_t;

int phoenix_sem_init(phoenix_sem_t *sem, int32_t initial);
int phoenix_sem_wait(phoenix_sem_t *sem);
int phoenix_sem_post(phoenix_sem_t *sem);

/* ---- Mutex ---- */
typedef struct {
    tcb_t   *owner;
    uint8_t  locked;
    tcb_t   *wait_head;
    tcb_t   *wait_tail;
} phoenix_mutex_t;

int phoenix_mutex_init(phoenix_mutex_t *m);
int phoenix_mutex_lock(phoenix_mutex_t *m);
int phoenix_mutex_unlock(phoenix_mutex_t *m);

#endif
