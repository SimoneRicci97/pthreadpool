#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

#include "pretvalqueue.h"


void* _retval_get(exitq_t* q, int* term);
void _retval_insert(exitq_t* q, void* extstatus);
void _retval_destroy(exitq_t* q);

exitq_t* new_retvalq() {
	exitq_t* q = malloc(sizeof(exitq_t));
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->empty_semaphore, NULL);
	q->insert = _retval_insert;
	q->get = _retval_get;
	q->destroy = _retval_destroy;
	return q;
}

void* _retval_get(exitq_t* q, int* term) {
	if(q == NULL) return NULL;
	pthread_mutex_lock(&q->mutex);
	while(q->size == 0 && !*term) {
		pthread_cond_wait(&q->empty_semaphore, &q->mutex);
	}
	if(*term) {
		pthread_mutex_unlock(&q->mutex);
		return NULL;
	}
	exit_item_t* exstatus = q->head;
	q->head = q->head->next;
	exstatus->next = NULL;
	if(q->head != NULL) q->head->prec = NULL;
	q->size--;
	pthread_mutex_unlock(&q->mutex);
	void* retval = exstatus->exit_status;
	return retval;
}

void _retval_insert(exitq_t* q, void* extstatus) {
	if(q == NULL) return;
	pthread_mutex_lock(&q->mutex);
	exit_item_t* item = malloc(sizeof(exit_item_t));
	item->next = NULL;
	item->prec = NULL;
	item->exit_status = extstatus;
	if(q->head == NULL) {
		q->head = item;
		q->tail = item;
	} else {
		item->prec = q->tail;
		q->tail->next = item;
		q->tail = item;
	}
	q->size++;
	pthread_cond_signal(&q->empty_semaphore);
	pthread_mutex_unlock(&q->mutex);
}

void _retval_destroy(exitq_t* q) {
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->empty_semaphore);
	exit_item_t* ptr = q->head;
	while(q->head != NULL) {
		q->head = q->head->next;
		q->head->prec = NULL;
		free(ptr);
		ptr = q->head;
	}
	free(q);
}
