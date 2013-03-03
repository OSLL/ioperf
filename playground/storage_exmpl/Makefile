main: main.o buffer.o 
	gcc main.o buffer.o -o main

main.o: main.c buffer.h iodata.h
	gcc -c main.c


buffer.o: buffer.c buffer.h iodata.h
	gcc -c buffer.c

clean:
	-rm *.o
	-rm main