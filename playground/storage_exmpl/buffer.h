#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>
#include "iodata.h"

#define MAX_BUFFER_SIZE ((size_t)100)

typedef struct {
	io_data data[MAX_BUFFER_SIZE];
	size_t size;
	size_t head;
	size_t tail;
} buffer;

void init(buffer *d);

int push_back(buffer *d, io_data element);

io_data pop_back(buffer *d);

int push_front(buffer *d, io_data element);

io_data pop_front(buffer *d);

io_data * front(buffer *d);

io_data * back(buffer *d);


#endif //BUFFER_H