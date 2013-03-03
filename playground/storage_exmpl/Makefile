main: main.o buffer.o iodata.o
	gcc main.o buffer.o iodata.o -o main

main.o: main.c buffer.h iodata.h
	gcc -c main.c

iodata.o: iodata.c iodata.h
	gcc -c iodata.c

buffer.o: buffer.c buffer.h iodata.h
	gcc -c buffer.c

clean:
	-rm *.o
	-rm main