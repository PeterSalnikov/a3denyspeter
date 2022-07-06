/*
required threads:

1. one thread awaits input from the keyboard.
2. the other thread awaits a UDP datagram from another process.
3. there will also be a thread that prints messages (sent and received) to the screen.
4. finally, a thread that sends data to the remote UNIX process over the network using UDP (use localhost IP(127.0.0.1) while testing it on the same machine with two terminals).
***two lists are required*** one for keyboard/sender, one for receiver, printer
*/

#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include"list.h"
/*
*/

//input thread: on receipt of the input, adds the input to a list of messages that need to be sent to the remote lets-talk client.



void* input_thread(void *lst)
{
//waits for and receives input
//adds the input to the list of messages
	int buf = 4000;
	int ret;
	while(1)
	{
		printf("we are in the thread\n");
		char input[buf];
		fgets(input, buf, stdin);
			
		ret = List_add(lst, (char *)input);
		if(ret == 1)
			pthread_exit(&ret);
		//testing output
		char *test = List_curr(lst);
		printf("%s", test);
		
		if(strcmp(input, "!exit\n") == 0)
		{
			//end the thread, still add to the list for the sender
			break;
		
		}
		//need cases for:
		//!status
	}
	
	pthread_exit(&ret);
}

void *sender_thread(void *lst)
{



}

int main(int argc, char *argv)
{
	//int *exit_status;
	pthread_t input;
	pthread_t udp;
	pthread_t print;
	
	List *input_list = List_create();
	
	int *ptr;
	
	pthread_create(&input, NULL, input_thread, input_list);
	pthread_join(input, (void**)&ptr);
	List *output_list = List_create();
	
	return 0;

}
