CC	= gcc
CFLAGS = -Wall -pedantic 

.PHONY: all test 1 test2 clean


supermercato: supermercato.c lib/dati.a lib/parsing.a lib/random.a 
	$(CC) -pthread supermercato.c -o supermercato lib/dati.a lib/parsing.a lib/random.a 

lib/dati.a: lib/dati1.o lib/dati2.o
	ar rvs lib/dati.a lib/dati1.o lib/dati2.o
		  
lib/parsing.a: lib/parsing1.o lib/parsing2.o
	ar rvs lib/parsing.a lib/parsing1.o lib/parsing2.o

lib/random.a: lib/random1.o lib/random2.o
	ar rvs lib/random.a lib/random1.o lib/random2.o


lib/dati1.o: lib/dati.c
	$(CC) $(CFLAGS) lib/dati.c -c -o lib/dati1.o

lib/dati2.o: lib/dati.h
	$(CC) $(CFLAGS) lib/dati.h -c -o lib/dati2.o

lib/parsing1.o: lib/parsing.c
	$(CC) $(CFLAGS) lib/parsing.c -c -o lib/parsing1.o

lib/parsing2.o: lib/parsing.h
	$(CC) $(CFLAGS) lib/parsing.h -c -o lib/parsing2.o

lib/random1.o: lib/random.c
	$(CC) $(CFLAGS) lib/random.c -c -o lib/random1.o

lib/random2.o: lib/random.h
	$(CC) $(CFLAGS) lib/random.h -c -o lib/random2.o



all:	supermercato
	chmod +x ./script/all.sh
	clear
	./script/all.sh

test1: supermercato
	chmod +x ./script/test1.sh
	./script/test1.sh


test2: supermercato
	chmod +x ./script/test2.sh
	./script/test2.sh

clean: 
	rm -f lib/dati.a
	rm -f lib/dati1.o
	rm -f lib/dati2.o
	rm -f lib/parsing.a
	rm -f lib/parsing1.o
	rm -f lib/parsing2.o
	rm -f lib/random.a
	rm -f lib/random1.o
	rm -f lib/random2.o
	rm -f supermercato
	rm -f filelog.txt

