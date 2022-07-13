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
#include <string.h> 
#include <pthread.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

const int BUFFER_SIZE = 4000;
char buffer[4000];
int is_exit = 0;
int encryption_key = 1;

pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;

struct sender_params {
	List* input_list;
	int sender_socket;
	struct addrinfo* sender_info;
};

struct reciever_params {
	int receiver_socket;
	List* output_list;
};

void *keyboard_input_thread(void * ptr) {
	List* input_list = ptr;
	printf("Welcome to LetS-Talk! Please type your message now.\n");
	// Get keywords
	while(!is_exit) {
		if(fgets(buffer, BUFFER_SIZE, stdin)){
			List_add(input_list, (char *)buffer);
		}
	} 
	pthread_exit(NULL);
}

int isValidIpAddress(char *ipAddress)
{
	if(strcmp(ipAddress, "localhost") == 0) {
		return 1;
	}
	struct sockaddr_in sa;
	int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
	return result;
}

void *console_output_thread(void * ptr) {
	List* output_list = ptr;
	while(!is_exit) {
		if(List_count(output_list)) {
			char * msg = List_remove(output_list);  
			printf("%s", msg);
			fflush(stdout);
			if(strcmp(msg, "!exit\n") == 0) {
					is_exit = 1;
			}
		}
	}
	pthread_exit(NULL);

}

void *udp_sender_thread(void * ptr) {
	struct sender_params *params = ptr;
	int i;
	while(!is_exit) {
		if(List_count(params->input_list)) {
			char * message = List_remove(params->input_list);
			// encryption
			for(i = 0; i < strlen(message); i++) {
				if(message[i] != '\n' && message[i] != '\0') {
					message[i] += encryption_key;
				}
			}
			int numbytes;
			if ((numbytes = sendto(params->sender_socket, message, strlen(message), 0,
				params->sender_info->ai_addr, params->sender_info->ai_addrlen)) == -1) {
				perror("Sender Error: ");
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
				char buf[BUFFER_SIZE];
				socklen_t addr_len;
				int numbytes2;
				addr_len = params->sender_info->ai_addrlen;
				struct timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = BUFFER_SIZE;
				if (setsockopt(params->sender_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
					perror("Sender Error: ");
				}
				if ((numbytes2 = recvfrom(params->sender_socket, buf, BUFFER_SIZE , 0,
					(params->sender_info)->ai_addr, &addr_len)) == -1) {
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

void *udp_receiver_thread(void * ptr) {
	struct reciever_params *params = ptr;
	struct sockaddr_storage address;
	int i;
	while(!is_exit) {
		char buf[BUFFER_SIZE];
		socklen_t address_count;
		int input_bytes;
		address_count = sizeof (address);
		if ((input_bytes = recvfrom(params->receiver_socket, buf, BUFFER_SIZE , 0,
			(struct sockaddr *)&(address), &address_count)) == -1) {
			perror("Reciever error: ");
			exit(0);
		}

		buf[input_bytes] = '\0';
		// decryption
		for(i = 0; i < strlen(buf); i++) {
			if(buf[i] != '\n' && buf[i] != '\0') {
					buf[i] -= encryption_key;
			}
		}

		List_add(params->output_list, (char *) buf);
		
		if(strcmp(buf, "!status\n") == 0) {
			int status_bytes;
			if ((status_bytes = sendto(params->receiver_socket, "Online", strlen("Online"), 0,
					(struct sockaddr *)&(address), address_count)) == -1) {
					perror("Sender Error: ");
					exit(1);
			}
		}        
	}
	pthread_exit(NULL);
}

int main (int argc, char ** argv) 
{

	if(argc < 4 || !isValidIpAddress(argv[2])) {
		printf("Usage:\n");
		printf("\t./lets-talk <local port> <remote host> <remote port>\n");
		printf("Examples:\n");
		printf("\t./lets-talk 3000 192.168.0.513 3001\n");
		printf("\t./lets-talk 3000 some-computer-name 3001\n");
		return 0;
	}

	//receiver
	int socket_info; 
	struct addrinfo hints, *server_info, *p;
	int addr_info; 
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	if ((addr_info = getaddrinfo(NULL, argv[1], &hints, &server_info)) != 0) {
		freeaddrinfo(server_info);
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_info));
		return 1;
	}

	for(p = server_info; p != NULL; p = p->ai_next) {
		if ((socket_info = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("Reciever Error: ");
			continue;
		}

		if (bind(socket_info, p->ai_addr, p->ai_addrlen) == -1) {
			close(socket_info);
			perror("Reciever Error: ");
			continue;
		}

		break;
	}

	if (p == NULL) {
		freeaddrinfo(server_info);
		fprintf(stderr, "Reciever Error: failed to bind socket\n");
		return 2;
	}

	//sender
	int sender_socket_info;
	struct addrinfo sender_hints, *sender_server_info, *sender_p;
	int sender_addr_info;

	memset(&sender_hints, 0, sizeof sender_hints);
	sender_hints.ai_family = AF_INET;
	sender_hints.ai_socktype = SOCK_DGRAM;
	if ((sender_addr_info = getaddrinfo(argv[2], argv[3], &sender_hints, &sender_server_info)) != 0) {
		freeaddrinfo(sender_server_info);
		fprintf(stderr, "Sender Error: %s\n", gai_strerror(sender_addr_info));
		return 1;
	}

	// loop through all the results and make a socket
	for(sender_p = sender_server_info; sender_p != NULL; sender_p = sender_p->ai_next) {
		if ((sender_socket_info = socket(sender_p->ai_family, sender_p->ai_socktype, sender_p->ai_protocol)) == -1) {
			perror("Sender Error: ");
			continue;
		}

		break;
	}

	if (sender_p == NULL) {
		freeaddrinfo(sender_server_info);
		fprintf(stderr, "Sender Error: failed to create socket\n");
		return 2;
	}
	
	List * input_list;
	input_list = List_create();
	
	List * output_list;
	output_list = List_create();

	struct reciever_params receiver;
	receiver.receiver_socket = socket_info;
	receiver.output_list = output_list;
	
	struct sender_params sender;
	sender.sender_socket = sender_socket_info;
	sender.sender_info = sender_server_info;
	sender.input_list = input_list;

	pthread_t keyboard_input, udp_sender, udp_receiver, console_output;

	//create threads for receiving and printing messages
	pthread_create(&keyboard_input, NULL, keyboard_input_thread, input_list);
	pthread_create(&console_output, NULL, console_output_thread, output_list);
	pthread_create(&udp_sender, NULL, udp_sender_thread, &sender);
	pthread_create(&udp_receiver, NULL, udp_receiver_thread, &receiver);
	

	while(!is_exit);
	pthread_cancel(keyboard_input);
	pthread_cancel(udp_sender);
	pthread_cancel(udp_receiver);
	pthread_cancel(console_output);

	pthread_join(keyboard_input, NULL);
	pthread_join(udp_sender, NULL);
	pthread_join(udp_receiver, NULL);
	pthread_join(console_output, NULL);

	freeaddrinfo(server_info);
	freeaddrinfo(sender_server_info);

	pthread_exit(0);
}
