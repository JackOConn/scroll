CFLAGS = -g

scroll: scroll.o
	gcc $(CFLAGS) scroll.o -o scroll

scroll.o: scroll.c
	gcc -c $(CFLAGS) scroll.c

clean:
	rm *.o