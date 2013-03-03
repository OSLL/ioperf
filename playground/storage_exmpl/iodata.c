#include "iodata.h"

io_data make_io_data(size_t st, size_t end, size_t id)
{
	io_data d;
	d.start = st;
	d.end = end;
	d.id = id;
	return d;
}
