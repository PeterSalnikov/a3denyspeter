all: lets-talk.c
	gcc -g -Wall -pthread -o lets-talk lets_talk.c list.c
	
clean:
	rm -rf lets-talk.o list.o

valgrind: 
	valgrind --leak-check=full ./lets-talk 3000 localhost 3001
