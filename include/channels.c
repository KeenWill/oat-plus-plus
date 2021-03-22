#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channels.h"

void enqueue(queue_ty *q, int64_t *val) {
  node_ty *new_node = malloc(sizeof(node_ty));
  if (new_node == NULL) {
    perror("Enqueue: ");
    exit(1);
  }

  new_node->value = val;
  new_node->next = NULL;
  new_node->prev = NULL;

  if (q->head == NULL) {
    q->head = new_node;
    q->tail = new_node;
  } else {
    new_node->next = q->head;
    q->head->prev = new_node;
    q->head = new_node;
  }
}

int64_t *dequeue(queue_ty *q) {
  if (q->tail == NULL) {
    return NULL;
  }

  node_ty *last = q->tail;
  int64_t *ret = last->value;

  if (q->head == q->tail) {
    q->head = NULL;
    q->tail = NULL;
  } else {
    q->tail = last->prev;
    q->tail->next = NULL;
  }

  free(last);
  return ret;
}

int64_t *chan_create() {
  channel_ty *chn = malloc(sizeof(channel_ty));
  queue_ty q;
  q.head = NULL;
  q.tail = NULL;
  chn->q = q;

  pthread_mutex_init(&(chn->lock), NULL);
  pthread_cond_init(&(chn->available), NULL);

  return (int64_t *)chn;
}

void chan_send(channel_ty *handle, int64_t *val) {
  if (pthread_mutex_lock(&(handle->lock)) != 0) {
    perror("chan_send mutex lock: ");
    exit(1);
  }

  // Lock acquired
  enqueue(&(handle->q), val);

  // Unlock and notify
  if (pthread_mutex_unlock(&(handle->lock)) != 0) {
    perror("chan_send mutex unlock: ");
    exit(1);
  }
  pthread_cond_signal(&(handle->available));
}

int64_t *chan_recv(channel_ty *handle) {
  if (pthread_mutex_lock(&(handle->lock)) != 0) {
    perror("chan_recv mutex lock: ");
    exit(1);
  }
  while (1) {
    // Lock acquired
    if (handle->q.head != NULL) {
      int64_t *ret = dequeue(&(handle->q));
      if (pthread_mutex_unlock(&(handle->lock)) != 0) {
        perror("chan_recv mutex unlock: ");
        exit(1);
      }
      return ret;
    } else {
      pthread_cond_wait(&(handle->available), &(handle->lock));
    }
  }
}