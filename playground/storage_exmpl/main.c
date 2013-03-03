#include "buffer.h"
#include <stdio.h>



int main() 
{
	buffer b;
	buffer * pb = &b;
	init(pb);

	push_back(pb, make_io_data(1,1,1));

	io_data d = pop_back(pb);
	printf("%d \n", d.start);

	return 0;
}