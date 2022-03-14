#include "nb_runtime.h"
#include <unistd.h>
#include <stdio.h>
struct data_queue_t* nb__new_data_queue(void) {
	struct data_queue_t * q = malloc(sizeof(struct data_queue_t));
	q->current_elems = 0;
	return q;
}
void nb__free_data_queue(struct data_queue_t* q) {
	free(q);
}
void nb__insert_data_queue(struct data_queue_t* q, char* buff, int len) {
	int index = q->current_elems;
	q->current_elems++;
	q->data_queue_elems[index] = buff;
	q->data_queue_elems_size[index] = len;	
}

#define MAX_CONNECTIONS (512)
static int current_open_connections = 0;
static nb__connection_t* open_connections[MAX_CONNECTIONS];
static unsigned open_connections_id[MAX_CONNECTIONS];

void nb__add_connection(nb__connection_t* c, unsigned sa) {
	int index = current_open_connections;
	current_open_connections++;
	open_connections[index] = c;
	open_connections_id[index] = sa;
}
void nb__delete_connection(unsigned sa) {
	int index = -1;
	int i;
	for (i = 0; i < current_open_connections; i++) {
		if (open_connections_id[i] == sa) {
			index = i;
			break;
		}
	}
	if (index != -1) {
		open_connections[index] = open_connections[current_open_connections - 1];
		open_connections_id[index] = open_connections_id[current_open_connections - 1];
		current_open_connections--;
	}
}
nb__connection_t* nb__retrieve_connection(unsigned sa) {
	int i;
	for (i = 0; i < current_open_connections; i++) {
		if (open_connections_id[i] == sa) {
			return open_connections[i];
		}
	}
	return NULL;
}

int nb__read(nb__connection_t* c, char* buff, int max_len) {
	if (c->input_queue->current_elems == 0)
		return 0;
	char* payload = c->input_queue->data_queue_elems[0];
	int payload_size = c->input_queue->data_queue_elems_size[0];
	int i;
	// TODO: Replace this with a circular queue
	for (i = 1; i < c->input_queue->current_elems; i++) {
		c->input_queue->data_queue_elems[i-1] = c->input_queue->data_queue_elems[i];
		c->input_queue->data_queue_elems_size[i-1] = c->input_queue->data_queue_elems_size[i];
	}
	c->input_queue->current_elems--;
	int copy_size = max_len < payload_size? max_len: payload_size;
	memcpy(buff, payload, copy_size);
	return copy_size;
}

static void nb__cycle_connections(void) {
	int i;
	for (i = 0; i < current_open_connections; i++) {
		nb__connection_t *c = open_connections[i];
		if (c->input_queue->current_elems) {
			c->callback_f(QUEUE_EVENT_READ_READY, c);
		}
	}
}

void nb__main_loop_step(void) {
	nb__run_ingress_step();	
	nb__cycle_connections();
}

// Set this at init 
unsigned nb__my_host_id = 0;




void nb__debug_packet(char* p) {
	int i = 0;
	for (i = 0; i < 20; i++) {
		printf("%x ", (unsigned char) p[i]);
	}
	printf("\n");
}