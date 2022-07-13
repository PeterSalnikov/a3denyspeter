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

const int BUFFER_SIZE = 4000;
char buffer[BUFFER_SIZE];
int is_exit = 0;
int encryption_key = 1;

pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;

void *keyboard_input_thread(List* input_list) {
	printf("Welcome to LetS-Talk! Please type your message now.\n");
	// Get keywords
	while(!is_exit) {
		if(fgets(buffer, BUFFER_SIZE, stdin)){
			List_add(input_list, (char *)buffer);
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

void *udp_sender_thread(List* input_list, int sender_socket, struct addrinfo* sender_info) {
	int i;
	while(!is_exit) {
		if(List_count(input_list)) {
			char * message = List_remove(input_list);
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
				char buf[BUFFER_SIZE];
				socklen_t addr_len;
				int numbytes2;
				addr_len = sender_info->ai_addrlen;
				struct timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = BUFFER_SIZE;
				if (setsockopt(sender_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
					perror("Error: ");
				}
				if ((numbytes2 = recvfrom(sender_socket, buf, BUFFER_SIZE , 0,
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

void *udp_receiver_thread(struct sockaddr_storage address, int receiver_socket, List* output_list) {
	int i;
	while(!is_exit) {
		char buf[BUFFER_SIZE];
		socklen_t address_count;
		int input_bytes;
		address_count = sizeof address;
		if ((input_bytes = recvfrom(receiver_socket, buf, BUFFER_SIZE , 0,
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

		List_add(output_list, (char *) buf);
		
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
	//receiver
	int socket_info; //socket_info
	struct addrinfo hints, *serverinfo, *p;
	int addr_info; //stores result of getaddrinfo
	struct sockaddr_storage address_two;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // sing IPv6
	hints.ai_socktype = SOCK_DGRAM; //
	hints.ai_flags = AI_PASSIVE; //use my IP

	if ((addr_info = getaddrinfo(NULL, argv[1], &hints, &serverinfo)) != 0) {
		freeaddrinfo(serverinfo);
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_info));
		return 1;
	}

	// bind to first possible result
	for(p = serverinfo; p != NULL; p = p->ai_next) {
		if ((socket_info = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(socket_info, p->ai_addr, p->ai_addrlen) == -1) {
			close(socket_info);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		freeaddrinfo(serverinfo);
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	//SENDER
	int sender_socket_info;
	struct addrinfo sender_hints, *sender_server_info, *sender_p;
	int sender_addr_info;

	memset(&sender_hints, 0, sizeof sender_hints);
	sender_hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	sender_hints.ai_socktype = SOCK_DGRAM;

	if ((sender_addr_info = getaddrinfo(argv[2], argv[3], &sender_hints, &sender_server_info)) != 0) {
		freeaddrinfo(sender_server_info);
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sender_addr_info));
		return 1;
	}

	// loop through all the results and make a socket
	for(sender_p = sender_server_info; sender_p != NULL; sender_p = sender_p->ai_next) {
		if ((sender_socket_info = socket(sender_p->ai_family, sender_p->ai_socktype, sender_p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (sender_p == NULL) {
		freeaddrinfo(sender_server_info);
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	
	List * input_list;
	input_list = List_create();
	
	List * output_list;
	output_list = List_create();
/*
	struct thread_params params;
	params.receiver_p = p;
	params.receiveraddr = their_addr;
	params.receiver_socketfd = socket_info;
	
	params.sender_socketfd = sender_socket_info;
	params.sender_serverinfo = sender_serverinfo;
	params.sender_p = sender_p;
	
	params.sending_list = input_list;
	params.receiving_list = output_list;
*/
	struct receiver_param receiver;
	receiver.address = address_two;
	receiver.receiver_socket = socket_info;
	receiver.output_list = output_list
	
	struct sender_param sender;
	sender.sender_socket = sender_socket_info;
	sender.sender_info = sender_server_info;
	sender.input_list = input_list;

	pthread_t keyboard_input_thread, udp_sender_thread, udp_receiver_thread, console_output_thread;

	//create threads for receiving and printing messages
	//keyboard_input takes list
	pthread_create(&keyboard_input_thread, NULL, input_msg, input_list);
	//List* input_list, int sender_socket, struct addrinfo* sender_info
	pthread_create(&udp_sender_thread, NULL, send_msg, input_list);
	
	pthread_create(&udp_receiver_thread, NULL, receive_msg, &receiver);
	
	pthread_create(&console_output_thread, NULL, print_msg, &sender);

	while(is_exit);
	pthread_cancel(keyboard_input_thread);
	pthread_cancel(udp_sender_thread);
	pthread_cancel(udp_receiver_thread);
	pthread_cancel(console_output_thread);

	pthread_join(keyboard_input_thread, NULL);
	pthread_join(udp_sender_thread, NULL);
	pthread_join(udp_receiver_thread, NULL);
	pthread_join(console_output_thread, NULL);

	freeaddrinfo(servinfo);
	freeaddrinfo(sender_servinfo);

	pthread_exit(0);
}
