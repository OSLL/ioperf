#include "buffer.h"
#include <stdio.h>

/**
 * Gets positive reminder by value of b.
 *
 * arg a integer value
 * arg b unsigned integer value
 * return reminder
 */ 

static inline int mod(int a, int b)
{
	int ret = a % b;
	if (a < 0) {
		ret += b;
	}
	return ret;
}


void init(buffer *d) 
{
	d->size = 0;
	memset(d->data, 0, MAX_BUFFER_SIZE * sizeof(io_data));
	d->tail = 0;
	d->head = MAX_BUFFER_SIZE - 1;
}

int push_back(buffer *d, io_data element)
{
	if (d->size >= MAX_BUFFER_SIZE) { 
		return 0;
	}

	d->tail = mod(d->tail - 1, MAX_BUFFER_SIZE);
	d->data[d->tail] = element;
	++d->size;
	return 1;
}

int push_front(buffer *d, io_data element)
{
	if (d->size >= MAX_BUFFER_SIZE) { 
		return 0;
	}

	d->head = mod(d->head + 1, MAX_BUFFER_SIZE);
	d->data[d->head] = element;
	++d->size;
	return 1;
}

io_data pop_front(buffer *d)
{
	assert(d->size != 0);
	if (d->size == 0) {
		;	//FIXME handle error
	}

	size_t tmp = d->head;
	d->head = mod(d->head - 1, MAX_BUFFER_SIZE);
	--d->size;
	return d->data[tmp];
}

io_data pop_back(buffer *d)
{
	assert(d->size != 0);
	if (d->size == 0) {
		; 	//FIXME handle error
	}

	size_t tmp = d->tail;
	d->tail = mod(d->tail + 1, MAX_BUFFER_SIZE);
	--d->size;
	return d->data[tmp]; 
}

io_data * front(buffer *d)
{
	
	return (d->size == 0) ? NULL : &(d->data[d->head]);
}

io_data * back(buffer *d)
{	
	return (d->size == 0) ? NULL : &(d->data[d->tail]);
}

