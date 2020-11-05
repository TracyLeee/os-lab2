/**
 * @brief implementation of mutex functions.
 *
 * @author Adrien Ghosn
 */

#include <assert.h>
#include "utils.h"
#include "mutex.h"
#include "sys_thread.h"

void mutex_init(mutex_t* m) {
  /* TODO: Implement */
  m->owner = DEFAULT_TARGET;
  spinlock_init(&m->lock);
  thread_list_init(&m->blocked);
}

void mutex_lock(mutex_t* m) {
  /* TODO: Implement */
  spinlock_lock(&m->lock);

  thread_info_t* current = get_current_thread();

  /* 'if' statement could be used here to avoid the second condition... */
  /* but it is recommended by the OSTEP book that 'while' should always be used */
  while (__sync_val_compare_and_swap(&m->owner, DEFAULT_TARGET, current->id) != DEFAULT_TARGET && 
         __sync_val_compare_and_swap(&m->owner, current->id, current->id) != current->id) {
    thread_list_add(&m->blocked, current);
    cond_wait((void *)&m->lock, SPINLOCK);
    spinlock_lock(&m->lock);
  }

  spinlock_unlock(&m->lock);
}

void mutex_unlock(mutex_t* m) {
  /* TODO: Implement */
  spinlock_lock(&m->lock);

  thread_info_t* current = get_current_thread();
  thread_info_t* next = NULL;
  tid_t next_id;

  if (thread_list_is_empty(&m->blocked)) {
    next_id = DEFAULT_TARGET;
  } else {
    next = thread_list_pop(&m->blocked);
    next_id = next->id;
  }

  if (__sync_val_compare_and_swap(&m->owner, current->id, next_id) != current->id) {
    if (next) thread_list_add(&m->blocked, next);
    spinlock_unlock(&m->lock);
    assert(false);
  }

  if (next) tsafe_unblock_thread(next);

  spinlock_unlock(&m->lock);
}


