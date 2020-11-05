/**
 * @file main.c
 * @brief Multithreading example
 *
 * @author Mark Sutherland
 */
#include <stdlib.h>
#include <string.h>
#include "channel.h"
#include "mutex.h"
#include "scheduler_state.h"
#include "sched_policy.h"
#include "spinlock.h"
#include "thread.h"
#include "thread_info.h"

#include "utils.h"

#define NITERS  100000
#define NTHREADS 3
#define MESSAGE_SIZE  100000
int global;
char message[MESSAGE_SIZE], recd_message[MESSAGE_SIZE];

char get_rand_char(void) {
  int letter = rand() % (26 * 2);
  if(letter < 26)
    return 'A' + letter;
  else
    return 'a' + letter - 26;
}

void *channel_sender(void *arg) {
  channel_t *channel = (channel_t *)arg;

  for(unsigned i = 0; i < MESSAGE_SIZE - 1; i++) {
    message[i] = get_rand_char();
    channel_send(channel, &message[i]);
  }
  message[MESSAGE_SIZE - 1] = '\0';

  return NULL;
}

void *channel_receiver(void *arg) {
  channel_t *channel = (channel_t *)arg;

  char *c;
  for(unsigned i = 0; i < MESSAGE_SIZE - 1; i++) {
    c = channel_receive(channel);
    recd_message[i] = (c == NULL)? 'a': *c;
  }
  recd_message[MESSAGE_SIZE - 1] = '\0';

  return NULL;
}

void *test_global_mutex(void *arg) {
  mutex_t *mutex = (mutex_t *)arg;
  volatile unsigned wait = 0;

  // thread_info_t *current = get_current_thread();
  // printf("my id %u\n", current->id);

  bool error_detected = false;
  /* The variable global is used to track how many threads are 
   * running the critical section concurrently. Due to mutual
   * exclusion, this should always be 0 or 1 */
  for(int i = 0; i < NITERS; i++){
    mutex_lock(mutex);
    // printf("got lock, tid %u\n", mutex->owner);
    int n_critical_threads = __sync_fetch_and_add(&global, 1);
    if(n_critical_threads != 0)
      /* Store a flag, to only print when the first error is 
       * detected, not every time */
      if(!error_detected) {
        printf("Error in mutex\n");
        error_detected = true;
      }
    for(wait = 0; wait < 1000; wait++);
    __sync_fetch_and_add(&global, -1);
    // printf("releasing...\n");
    mutex_unlock(mutex);
    // printf("new owner %u\n", mutex->owner);
  }
  return NULL;
}

void *increment_global_spinlock(void *arg) {
  spinlock_t *lock = (spinlock_t *)arg;

  for(int i = 0; i < NITERS; i++){
    spinlock_lock(lock);
    global += 1;
    spinlock_unlock(lock);
  }

  return NULL;
}

/* Run threads with different types of locking to test them */
void *try_locks(void *arg) {
  tid_t threads[NTHREADS];

  global = 0;
  spinlock_t lock;
  spinlock_init(&lock);
  
  printf("Testing spinlock\n");
  for(int i = 0; i < NTHREADS; i++)
    l2_thread_create(&threads[i], increment_global_spinlock, (void *)&lock);

  for(int i = 0; i < NTHREADS; i++)
    l2_thread_join(threads[i], NULL);
  printf("End of spinlock test, "
         "Global expected to be %d, got %d\n", NTHREADS * NITERS, global);

  mutex_t mutex;
  mutex_init(&mutex);
  printf("Testing mutual exclusion\n");
  global = 0;
  for(int i = 0; i < NTHREADS; i++)
    l2_thread_create(&threads[i], test_global_mutex, &mutex);

  for(int i = 0; i < NTHREADS; i++)
    l2_thread_join(threads[i], NULL);
  printf("End of mutex test, "
         "Global expected to be %d, got %d\n", 0, global);

  tid_t send_thread, recv_thread;
  channel_t channel;
  channel_init(&channel);
  printf("Testing channel:\n");
  l2_thread_create(&send_thread, channel_sender, &channel);
  l2_thread_create(&recv_thread, channel_receiver, &channel);
  l2_thread_join(send_thread, NULL);
  l2_thread_join(recv_thread, NULL);
  printf("Sent successfully? %s\n", 
         (strcmp(message, recd_message) == 0)? "true": "false");
  return NULL;
}

int main(int argc, char **argv)
{
  tid_t locker_thread;
  initialize_scheduler_state(round_robin_policy, NTHREADS);
  l2_thread_create(&locker_thread, try_locks, NULL);
  init_sys_and_launch();

  return 0;
}
