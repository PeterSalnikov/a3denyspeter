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
#include <netdb.h>
#include <sys/time.h>

#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/types.h>
#include <sys/socket.h>
#define STDIN 0 
/*
*/

//input thread: on receipt of the input, adds the input to a list of messages that need to be sent to the remote lets-talk client.

pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
bool term_signal = 1;
char buffer[4000];
int encryption_key = 1;
struct thread_params {
    int receiver_socketfd;
    int sender_socketfd;
    struct sockaddr_storage receiveraddr;
    struct addrinfo * sender_servinfo, *sender_p, *receiver_p;
    List * sending_list;
    List * receiving_list;
};

void *receive_msg(void * ptr) {
    struct thread_params *params = ptr;
    int i;
    while(term_signal) {
        char buf[4000];
        socklen_t addr_len;
        int numbytes;
        addr_len = sizeof params->receiveraddr;
        if ((numbytes = recvfrom(params->receiver_socketfd, buf, 4000 , 0,
            (struct sockaddr *)&(params->receiveraddr), &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        buf[numbytes] = '\0';
        // decryption
        for(i = 0; i < strlen(buf); i++) {
            if(buf[i] != '\n' && buf[i] != '\0') {
                buf[i] -= encryption_key;
            }
        }

        List_add(params->receiving_list, (char *) buf);
        
        if(strcmp(buf, "!status\n") == 0) {
            int numbytes2;
            char * msg = "Online";
            if ((numbytes2 = sendto(params->receiver_socketfd, msg, strlen(msg), 0,
                (struct sockaddr *)&(params->receiveraddr), addr_len)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            
        }        
    }
    pthread_exit(NULL);

}
void *print_msg(void * ptr) {
    struct thread_params *params = ptr;
    while(term_signal) {
        if(List_count(params->receiving_list)) {
            char * msg = List_remove(params->receiving_list);  
            printf("%s", msg);
            fflush(stdout);
            if(strcmp(msg, "!exit\n") == 0) {
                term_signal = 0;
            }
        }
    }
    pthread_exit(NULL);

}
void *send_msg(void * ptr) {
    struct thread_params *params = ptr;
    int i;
    while(term_signal) {
        if(List_count(params->sending_list)) {
            char * msg = List_remove(params->sending_list);
            // encryption
            for(i = 0; i < strlen(msg); i++) {
                if(msg[i] != '\n' && msg[i] != '\0') {
                    msg[i] += encryption_key;
                }
            }
            int numbytes;
            if ((numbytes = sendto(params->sender_socketfd, msg, strlen(msg), 0,
                (params->sender_p)->ai_addr, (params->sender_p)->ai_addrlen)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            for(i = 0; i < strlen(msg); i++) {
                if(msg[i] != '\n' && msg[i] != '\0') {
                    msg[i] -= encryption_key;
                }
            }
            if(strcmp(msg, "!exit\n") == 0) {
                term_signal = 0;
            }
            if(strcmp(msg, "!status\n") == 0) {
                char buf[4000];
                socklen_t addr_len;
                int numbytes2;
                addr_len = (params->sender_p)->ai_addrlen;
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                if (setsockopt(params->sender_socketfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
                    perror("Error");
                }
                if ((numbytes2 = recvfrom(params->sender_socketfd, buf, 4000 , 0,
                    (params->sender_p)->ai_addr, &addr_len)) == -1) {
                    strcpy(buf, "Offline");
                    printf("Offline\n");
                }
                if(strcmp(buf, "Online") == 0) {
                    buf[numbytes2] = '\0';
                    printf("%s\n", buf);
                }
                
            }     
        }
    }
    pthread_exit(NULL);

}
void *input_msg(void * ptr) {
    struct thread_params *params = ptr;
    printf("Welcome to lets-talk! Please type your message now\n");
    while(term_signal) {
        if(fgets(buffer, 4000, stdin)){
            List_add(params->sending_list, (char *)buffer);
        }
    } 
    pthread_exit(NULL);
}


void* input_thread(void *lst)
{
//waits for and receives input
//adds the input to the list of messages
	int buf = 4000;
	int ret;
	while(1)
	{
		char input[buf];
		fgets(input, buf, stdin);
			
		ret = List_add(lst, (char *)input);
		if(ret == 1)
			pthread_exit(&ret);
		
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

void *print_message(void *lst)
{

	char *msg = List_curr(lst);
	printf("%s", msg);

}

void *sender_thread(void *lst)
{



}

void *await_udp()
{



}

int main(int argc, char *argv)
{

	//Recieve
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	struct sockaddr_storage their_addr;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
			freeaddrinfo(servinfo);
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
					perror("listener: socket");
					continue;
			}

			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
					close(sockfd);
					perror("listener: bind");
					continue;
			}

			break;
	}

    if (p == NULL) {
        freeaddrinfo(servinfo);
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

	//Send
    int sender_sockfd;
    struct addrinfo sender_hints, *sender_servinfo, *sender_p;
    int sender_rv;

    memset(&sender_hints, 0, sizeof sender_hints);
    sender_hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    sender_hints.ai_socktype = SOCK_DGRAM;

    if ((sender_rv = getaddrinfo(argv[2], argv[3], &sender_hints, &sender_servinfo)) != 0) {
        freeaddrinfo(sender_servinfo);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sender_rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(sender_p = sender_servinfo; sender_p != NULL; sender_p = sender_p->ai_next) {
        if ((sender_sockfd = socket(sender_p->ai_family, sender_p->ai_socktype,
                sender_p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (sender_p == NULL) {
        freeaddrinfo(sender_servinfo);
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

	//int *exit_status;
	pthread_t input;
	pthread_t udp;
	pthread_t print;

	// pthread_t receiving_thr, printer_thr, sending_thr, keyboard_thr;
	
	List *input_list = List_create();
	List *output_list = List_create();
	
	int *ptr;

	struct thread_params params;
    params.receiver_p = p;
    params.receiveraddr = their_addr;
    params.receiver_socketfd = sockfd;
    params.sender_socketfd = sender_sockfd;
    params.sender_servinfo = sender_servinfo;
    params.sender_p = sender_p;
    params.sending_list = output_list;
    params.receiving_list = input_list;
	
	// pthread_create(&input, NULL, input_thread, input_list);
	// pthread_join(input, (void**)&ptr);
	
	pthread_t receiving_thr, printer_thr, sending_thr, keyboard_thr;
	//create threads for receiving and printing messages
	pthread_create(&keyboard_thr, NULL, input_msg, &params);
	pthread_create(&sending_thr, NULL, send_msg, &params);
	pthread_create(&receiving_thr, NULL, receive_msg, &params);
	pthread_create(&printer_thr, NULL, print_msg, &params);

	while(term_signal);
	pthread_cancel(keyboard_thr);
	pthread_cancel(sending_thr);
	pthread_cancel(receiving_thr);
	pthread_cancel(printer_thr);

	pthread_join(keyboard_thr, NULL);
	pthread_join(sending_thr, NULL);
	pthread_join(receiving_thr, NULL);
	pthread_join(printer_thr, NULL);

	freeaddrinfo(servinfo);
	freeaddrinfo(sender_servinfo);

	pthread_exit(0);

}
