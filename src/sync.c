#include "phoenix.h"

/* ============================================================
 *  COUNTING SEMAPHORE
 * ============================================================ */

int phoenix_sem_init(phoenix_sem_t *s, int32_t init) {
    s->count = init;
    s->wait_head = s->wait_tail = NULL;
    return PHOENIX_OK;
}

int phoenix_sem_wait(phoenix_sem_t *s) {
    uint32_t ps = phoenix_enter_critical();
    if (s->count > 0) {
        s->count--;
        phoenix_exit_critical(ps);
        return PHOENIX_OK;
    }

    /* Block current task */
    tcb_t *cur = phoenix_current;
    cur->state = TASK_BLOCKED;
    cur->prev = s->wait_tail;
    cur->next = NULL;
    if (s->wait_tail) s->wait_tail->next = cur;
    else              s->wait_head = cur;
    s->wait_tail = cur;

    phoenix_ready_remove(cur);
    phoenix_exit_critical(ps);
    phoenix_pendsv_trigger();
    return PHOENIX_OK;
}

int phoenix_sem_post(phoenix_sem_t *s) {
    uint32_t ps = phoenix_enter_critical();
    tcb_t *w = s->wait_head;
    if (w) {
        s->wait_head = w->next;
        if (!s->wait_head) s->wait_tail = NULL;
        w->prev = w->next = NULL;
        phoenix_ready_add(w);
        if (w->priority < phoenix_current->priority)
            phoenix_pendsv_trigger();
    } else {
        s->count++;
    }
    phoenix_exit_critical(ps);
    return PHOENIX_OK;
}

/* ============================================================
 *  MUTEX
 * ============================================================ */

int phoenix_mutex_init(phoenix_mutex_t *m) {
    m->owner = NULL;
    m->locked = 0;
    m->wait_head = m->wait_tail = NULL;
    return PHOENIX_OK;
}

int phoenix_mutex_lock(phoenix_mutex_t *m) {
    uint32_t ps = phoenix_enter_critical();

    /* Case 1: Mutex free → acquire */
    if (!m->locked) {
        m->locked = 1;
        m->owner = phoenix_current;
        phoenix_exit_critical(ps);
        return PHOENIX_OK;
    }

    /* Case 2: Current task already owns it (recursive) → error */
    if (m->owner == phoenix_current) {
        phoenix_exit_critical(ps);
        return PHOENIX_ERR_BUSY;
    }

    /* Case 3: Mutex held by someone else → block */
    tcb_t *cur = phoenix_current;
    cur->state = TASK_BLOCKED;
    cur->prev = m->wait_tail;
    cur->next = NULL;
    if (m->wait_tail) m->wait_tail->next = cur;
    else              m->wait_head = cur;
    m->wait_tail = cur;

    phoenix_ready_remove(cur);
    phoenix_exit_critical(ps);
    phoenix_pendsv_trigger();
    return PHOENIX_OK;
}

int phoenix_mutex_unlock(phoenix_mutex_t *m) {
    uint32_t ps = phoenix_enter_critical();

    /* Only owner can unlock */
    if (m->owner != phoenix_current) {
        phoenix_exit_critical(ps);
        return PHOENIX_ERR;
    }

    /* Hand off to next waiting task, if any */
    tcb_t *w = m->wait_head;
    if (w) {
        m->wait_head = w->next;
        if (!m->wait_head) m->wait_tail = NULL;
        w->prev = w->next = NULL;
        m->owner = w;
        phoenix_ready_add(w);
        if (w->priority < phoenix_current->priority)
            phoenix_pendsv_trigger();
    } else {
        m->owner = NULL;
        m->locked = 0;
    }
    phoenix_exit_critical(ps);
    return PHOENIX_OK;
}
