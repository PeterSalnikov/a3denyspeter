all: lets-talk.o list.o
	gcc -o lets-talk lets-talk.o list.o -pthread
	
lets-talk.o: lets-talk.c list.h
	gcc -c lets-talk.c
	
list.o: list.c list.h
	gcc -c list.c
	
clean:
	rm -rf lets-talk.o list.o

valgrind: 
	valgrind --leak-check=full ./lets-talk 3000 localhost 3001
