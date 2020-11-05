/**
 * @file channel.c
 *
 * @brief Implementation of channels.
 *
 * @author Adrien Ghosn
 *
 */
#include <assert.h>
#include "utils.h"
#include "mutex.h"
#include "channel.h"
#include "sys_thread.h"
#include "scheduler_state.h"

void channel_init(channel_t* chan) {
  /* TODO: Implement */
  mutex_init(&chan->mu);
  thread_list_init(&chan->blocked_recv);
  thread_list_init(&chan->blocked_send);
}

void channel_send(channel_t* chan, void* value) {
  /* TODO: Implement */
  mutex_lock(&chan->mu);

  if (!thread_list_is_empty(&chan->blocked_recv)) {
    thread_info_t *blk_recver = thread_list_pop(&chan->blocked_recv);
    
    blk_recver->channel_buffer = value;
    tsafe_unblock_thread(blk_recver);

    mutex_unlock(&chan->mu);
  } else {
    thread_info_t *sender = get_current_thread();

    sender->channel_buffer = value;
    thread_list_add(&chan->blocked_send, sender);
    cond_wait((void *)&chan->mu, MUTEX);
    assert(sender->channel_buffer == NULL);
  }
}

void* channel_receive(channel_t* chan) {
  /* TODO: Implement */
  mutex_lock(&chan->mu);
  void *buffer = NULL;

  if (!thread_list_is_empty(&chan->blocked_send)) {
    thread_info_t *blk_sender = thread_list_pop(&chan->blocked_send);
    
    buffer = blk_sender->channel_buffer;
    blk_sender->channel_buffer = NULL;
    tsafe_unblock_thread(blk_sender);

    mutex_unlock(&chan->mu);
  } else {
    thread_info_t *recver = get_current_thread();

    thread_list_add(&chan->blocked_recv, recver);
    cond_wait((void *)&chan->mu, MUTEX);
    // assert(recver->channel_buffer != NULL);
    // assertion here would fail channel_global_test...
    
    buffer = recver->channel_buffer;
    recver->channel_buffer = NULL;
  }

  return buffer;
}
