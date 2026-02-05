#include "ZT_queue.h"

#define IS_NULL(QUEUE) \
	do { \
		if (QUEUE == NULL) { \
			printf("[ERROR] queue is NULL\n"); \
			return -1; \
		} \
	} while(0)

#define MUTEX_UNLOCK(MUTEX) \
	do { \
		if (pthread_mutex_unlock(MUTEX) < 0) { \
			printf("[ERROR] mutex unlock: %s\n", strerror(errno)); \
		} \
	} while(0)

#define SEM_POST(SEM) \
	do { \
		if (sem_post(SEM) < 0) { \
			printf("[ERROR] sem post: %s\n", strerror(errno)); \
		} \
	} while(0)

#define LOG_ERR(fmt, ...) \
	printf ("[ERROR] (%s:%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

int init_queue(Queue *queue) {

	IS_NULL(queue);

	queue->front = NULL;
	queue->rear = NULL;

	if(pthread_mutex_init(&queue->mutex, NULL) != 0) {
		printf("[ERRPR] mutex_init: %s\n", strerror(errno));
		return -1;
	}

	if(sem_init(&queue->sem_empty, 0, MAX_QUEUE) != 0) {
		printf("[ERROR] sem_empty init: %s\n", strerror(errno));
		return -1;
	}

	if(sem_init(&queue->sem_full, 0, 0) != 0) {
		printf("[ERROR] sem_full init: %s\n", strerror(errno));
		return -1;
	}

	return 0;	
}

int enqueue(Queue *queue, address_info_t *data) { // producer

	IS_NULL(queue);

	if(data == NULL) {
		printf("[ERROR] enqueue: data is NULL\n");
		return -1;
	}

	if(sem_wait(&queue->sem_empty) < 0) {
		printf("[ERROR] sem wait: %s\n", strerror(errno));
		return -1;
	}

	if(pthread_mutex_lock(&queue->mutex) < 0) {
		printf("[ERROR] mutex lock: %s\n", strerror(errno));
		return -1;
	}

	Node* new_node = (Node*)malloc(sizeof(Node));

	if(new_node != NULL) {

		new_node->address_info = *data;
		new_node->next = NULL;

		if(queue->rear == NULL) {
			queue->front = new_node;
			queue->rear = new_node;
		} else {
			queue->rear->next = new_node;
			queue->rear = new_node;
		}

		queue->size ++;

	} else {
		printf("[ERROR] failed create node\n");
		MUTEX_UNLOCK(&queue->mutex);
		SEM_POST(&queue->sem_empty);
		return -1;
	}

	MUTEX_UNLOCK(&queue->mutex);
	SEM_POST(&queue->sem_full);

	return 0;
}

int dequeue(Queue *queue, address_info_t *data) { // consumer

	IS_NULL(queue);

	if(data == NULL) {
		printf("[ERROR] dequeue: data is NULL\n");
		return -1;
	}

	if(sem_wait(&queue->sem_full) < 0) {
		printf("[ERROR] sem wait: %s\n", strerror(errno));
		return -1;
	}

	if(pthread_mutex_lock(&queue->mutex) != 0) {
		printf("[ERROR] mutex lock: %s\n", strerror(errno));
		return -1;
	}

	bool empty;
	if(is_empty(queue, &empty) < 0) {
		printf("[ERROR] failed check queue is empty\n");
		MUTEX_UNLOCK(&queue->mutex);
		SEM_POST(&queue->sem_full);
	}

	if(!empty) {

		Node *temp = queue->front;

		*data = temp->address_info;
		queue->front = temp->next;

		free(temp);

	} else {
		printf("[ERROR] queue is empty\n");
		MUTEX_UNLOCK(&queue->mutex);
		SEM_POST(&queue->sem_full);
		return -1;
	}

	MUTEX_UNLOCK(&queue->mutex);
	SEM_POST(&queue->sem_empty);

	return 0;
}

int is_empty(Queue *queue, bool *flag) {

	IS_NULL(queue);

	if(pthread_mutex_lock(&queue->mutex) < 0) {
		printf("[ERROR] mutex lock: %s\n", strerror(errno));
		return -1;
	}

	int val = 0;
	if(sem_getvalue(&queue->sem_empty, &val) < 0) {
		printf("[ERROR] sem getvalue: %s\n", strerror(errno));
		destroy_queue(queue);
		return -1;
	}

	if(val == MAX_QUEUE) {
		*flag = true;
	} else {
		*flag = false;
	}

	MUTEX_UNLOCK(&queue->mutex);

	return 0;

}

int is_full(Queue *queue, bool *flag) {

	IS_NULL(queue);

	if(pthread_mutex_lock(&queue->mutex) < 0) {
		printf("[ERROR] mutex lock: %s\n", strerror(errno));
		return -1;
	}

	int  val;
	if(sem_getvalue(&queue->sem_empty, &val) < 0) {
		printf("[ERROR] sem getvalue: %s\n", strerror(errno));
		destroy_queue(queue);
		return -1;
	}

	if(val == 0) {
		*flag = true;
	} else {
		*flag = false;
	}

	MUTEX_UNLOCK(&queue->mutex);

	return 0;

}

int destroy_queue(Queue *queue) {

	IS_NULL(queue);

	bool empty = false;
	while(1) {

		if(is_empty(queue, &empty) < 0) {
			printf("[ERROR] failed check queue is empty\n");
			return -1;
		}

		if(empty) break;

		address_info_t address_info;

		if (dequeue(queue, &address_info) < 0) {
			printf("[ERROR] failed dequeue\n");
			return -1;
		}

	}


	if(pthread_mutex_destroy(&queue->mutex) < 0) {
		printf("[ERROR] mutex destroy: %s\n", strerror(errno));
	}

	if(sem_destroy(&queue->sem_empty) < 0) {
		printf("[ERROR] sem destroy: %s\n", strerror(errno));
	}

	if(sem_destroy(&queue->sem_full) < 0) {
		printf("[ERROR] sem destroy: %s\n", strerror(errno));
	}

	return 0;
}

