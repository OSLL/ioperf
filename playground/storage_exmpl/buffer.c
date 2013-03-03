#include "buffer.h"
#include <string.h>

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

	d->tail = (d->tail - 1) % MAX_BUFFER_SIZE;
	d->data[d->tail] = element;
	++d->size;
	return 1;
}

io_data pop_back(buffer *d)
{
	if (d->size == 0) {
		; //assert 
	}

	size_t tmp = d->tail;
	d->tail = (d->tail + 1) % MAX_BUFFER_SIZE;
	--d->size;
	return d->data[tmp]; 
}

int push_front(buffer *d, io_data element)
{
	if (d->size >= MAX_BUFFER_SIZE) { 
		return 0;
	}

	d->head = (d->head + 1) % MAX_BUFFER_SIZE;
	d->data[d->head] = element;
	++d->size;
	return 1;
}

io_data pop_front(buffer *d)
{
	if (d->size == 0) {
		;//assert 
	}

	size_t tmp = d->tail;
	d->head = (d->head - 1) % MAX_BUFFER_SIZE;
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

