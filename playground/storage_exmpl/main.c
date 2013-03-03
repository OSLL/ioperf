#include "buffer.h"
#include "iodata.h"



int main() 
{
	buffer b;
	buffer * pb = &b;
	init(pb);

	push_back(pb,make_io_data(1,1,1));


	return 0;
}