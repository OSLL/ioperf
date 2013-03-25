#include "buffer.h"
#include <stdio.h>



int main() 
{
	buffer b;
	buffer * pb = &b;
	init(pb);

	int i = 0;
	for (; i != 50; ++i) {
		assert(push_back(pb, make_io_data(i,1,1)) == 1);
		assert(push_front(pb, make_io_data(99-i,2,2)) == 1);
	}
	printf("%d\n", b.size);

	i = 0;
	for (; i != 100; ++i) {
		printf("%d ", b.data[i].start);
	}
	printf("\n");

	
	// printf("%d \n", pop_back(pb).start);
	// printf("%d \n", pop_front(pb).start);

	// i = 0;
	// for (; i != 50; ++i) {
	// 	printf("%d ", pop_back(pb).start);
	// 	printf("%d ", pop_front(pb).start);
	// }
	// printf("\n");


	i = 0;
	for (; i != 50; ++i) {
		printf("%d ", pop_back(pb).start);
		if (i%2) 
			printf("%d ", pop_back(pb).start);
		else
			printf("%d ", pop_front(pb).start);
	}
	printf("\n");
	printf("%d\n", b.size);
	return 0;
}