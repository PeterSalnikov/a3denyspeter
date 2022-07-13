/*
required threads:

1. one thread awaits input from the keyboard.
2. the other thread awaits a UDP datagram from another process.
3. there will also be a thread that prints messages (sent and received) to the screen.
4. finally, a thread that sends data to the remote UNIX process over the network using UDP (use localhost IP(127.0.0.1) while testing it on the same machine with two terminals).
***two lists are required*** one for keyboard/sender, one for receiver, printer
*/

#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include <pthread.h>
#include <netdb.h>
#include <sys/time.h>
// #define STDIN 0 

pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
bool is_exit = 0;
char buffer[4000];
int encryption_key = 1;

void *keyboard_input_thread(List* output_list) {
	printf("Welcome to LetS-Talk! Please type your message now.\n");
	while(!is_exit) {
		if(fgets(buffer, 4000, stdin)){
			List_add(output_list, (char *)buffer);
		}
	} 
	pthread_exit(NULL);
}

void *console_output_thread(List* input_list) {
	while(!is_exit) {
		if(List_count(input_list)) {
			char * msg = List_remove(input_list);  
			printf("%s", msg);
			fflush(stdout);
			if(strcmp(msg, "!exit\n") == 0) {
					is_exit = 1;
			}
		}
	}
	pthread_exit(NULL);

}

void *udp_sender_thread(List* output_list, int sender_socket, struct addrinfo* sender_info) {
	int i;
	while(!is_exit) {
		if(List_count(output_list)) {
			char * message = List_remove(output_list);
			// encryption
			for(i = 0; i < strlen(message); i++) {
				if(message[i] != '\n' && message[i] != '\0') {
					message[i] += encryption_key;
				}
			}
			int numbytes;
			if ((numbytes = sendto(sender_socket, message, strlen(message), 0,
				sender_info->ai_addr, sender_info->ai_addrlen)) == -1) {
				perror("Send Error: ");
				exit(1);
			}
			for(i = 0; i < strlen(message); i++) {
				if(message[i] != '\n' && message[i] != '\0') {
					message[i] -= encryption_key;
				}
			}
			if(strcmp(message, "!exit\n") == 0) {
				is_exit = 1;
			}
			if(strcmp(message, "!status\n") == 0) {
				char buf[4000];
				socklen_t addr_len;
				int numbytes2;
				addr_len = sender_info->ai_addrlen;
				struct timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = 40000;
				if (setsockopt(sender_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
					perror("Error: ");
				}
				if ((numbytes2 = recvfrom(sender_socket, buf, 4000 , 0,
					(sender_info)->ai_addr, &addr_len)) == -1) {
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

void *udp_receiver_thread(struct sockaddr_storage address, int receiver_socket, List* input_list) {
	int i;
	while(!is_exit) {
		char buf[4000];
		socklen_t address_count;
		int input_bytes;
		address_count = sizeof address;
		if ((input_bytes = recvfrom(receiver_socket, buf, 4000 , 0,
			(struct sockaddr *)&(address), &address_count)) == -1) {
			perror("Recieve error: ");
			exit(1);
		}

		buf[input_bytes] = '\0';
		// decryption
		for(i = 0; i < strlen(buf); i++) {
			if(buf[i] != '\n' && buf[i] != '\0') {
					buf[i] -= encryption_key;
			}
		}

		List_add(input_list, (char *) buf);
		
		if(strcmp(buf, "!status\n") == 0) {
			int status_bytes;
			if ((status_bytes = sendto(receiver_socket, "Online", strlen("Online"), 0,
					(struct sockaddr *)&(address), address_count)) == -1) {
					perror("Send Error: ");
					exit(1);
			}
		}        
	}
	pthread_exit(NULL);
}

int main (int argc, char ** argv) 
{
	//RECEIVER
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
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
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

	//SENDER
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
		if ((sender_sockfd = socket(sender_p->ai_family, sender_p->ai_socktype, sender_p->ai_protocol)) == -1) {
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
	
	List * send_list;
	send_list = List_create();
	
	List * receive_list;
	receive_list = List_create();

	struct thread_params params;
	params.receiver_p = p;
	params.receiveraddr = their_addr;
	params.receiver_socketfd = sockfd;
	params.sender_socketfd = sender_sockfd;
	params.sender_servinfo = sender_servinfo;
	params.sender_p = sender_p;
	params.sending_list = send_list;
	params.receiving_list = receive_list;

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