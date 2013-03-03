#ifndef IODATA_H
#define IODATA_H

#include <stdlib.h>
typedef struct {
	size_t start;
	size_t end;
	size_t id;
} io_data;

static inline io_data make_io_data(size_t st, size_t end, size_t id)
{
	io_data d;
	d.start = st;
	d.end = end;
	d.id = id;
	return d;
}

#endif // IODATA_H