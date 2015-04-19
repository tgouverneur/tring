CC=gcc -pedantic -Wall


.SILENT:


all:	tring

tring:	tring.c struct.h
	echo -n "Compilation of tring.c "
	$(CC) -c tring.c
	echo "ok"
	echo -n "Linking..."
	$(CC) -o tring tring.o
	echo "ok"

clean:	
	rm -f *.o tring

