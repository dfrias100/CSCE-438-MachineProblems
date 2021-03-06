#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"


int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);

int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    display_title();
    
	while (1) {
	
		int sockfd = connect_to(argv[1], atoi(argv[2]));
    
		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0 && reply.status == SUCCESS) {
			printf("Now you are in the chatmode\n");
			close(sockfd);
			process_chatmode(argv[1], reply.port);
		}
	
		close(sockfd);
    }

    return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 * 
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

	// Creating a sock address struct
	struct sockaddr_in sin;
	
	// Setting the details of the structs
    memset(&sin, 0, sizeof(sin)); // Filling the entire struct to 0
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port); 
    
    // Finding the host name and copying it into the struct
    if (struct hostent* phe = gethostbyname(host)) {
        
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length); 
    	
    } else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        exit(1);
    }
    
    // Creating the actual socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
        exit(1);
    }

	// Now, we connect to the server
    if (connect(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        exit(1);
    }

	return sockfd;
}

/* 
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply    
 */
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------
	
	/* 
	   The server will parse the command, but the client will make sure
	   the command part is uppercase
	*/
	
	char tokenizedCommand[MAX_DATA];
	strncpy(tokenizedCommand, command, MAX_DATA); // strtok modifies command, so we'll just copy it to a temp variable
	char* token = strtok(tokenizedCommand, " "); // We just want the first part of the command
	
	// This doesn't check for validity, but we'll make it uppercase 
	switch(strlen(token)) {
		case 6:
			for (int i = 0; i < 6; i++)
				command[i] = toupper((unsigned char) command[i]);
			break;
		case 4:
			for (int i = 0; i < 4; i++)
				command[i] = toupper((unsigned char) command[i]);
			break;
		default:
			break;
	}

	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------
	send(sockfd, command, MAX_DATA, 0);

	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------
	
	// Recv will automatically initialize these variables with the reply
	struct Reply reply;
	if(recv(sockfd, &reply, sizeof(reply), 0) <= 0) {
		reply.status = FAILURE_UNKNOWN;
	}
	
	return reply;
}

/* 
 * Get into the chat mode
 * 
 * @parameter host     host address
 * @parameter port     port
 */
void process_chatmode(const char* host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------
	int chatroom_socket = connect_to(host, port);
	
	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	
	// We'll use an fd_set to process either output or input when needed
	fd_set room;
	FD_ZERO(&room);
	FD_SET(STDIN_FILENO, &room);
	FD_SET(chatroom_socket, &room);
	
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
	
	while (1) {
		fd_set active_read_set = room;
		fflush(stdout);
		select(chatroom_socket + 1, &active_read_set, NULL, NULL, NULL);
    	
    	// We got a message from the server
		if (FD_ISSET(chatroom_socket, &active_read_set)) {
			char incomingMessage[MAX_DATA];
			if (recv(chatroom_socket, incomingMessage, MAX_DATA, 0) == 0) {
				// Server closed connection, the server will send the kill message
				display_title();
				return;
			}
			display_message(incomingMessage);
		}
	
		// The user typed something and needs to be sent
		if (FD_ISSET(STDIN_FILENO, &active_read_set)) {
			char message[MAX_DATA];
			get_message(message, MAX_DATA);
			send(chatroom_socket, message, MAX_DATA, 0);
		}
	}
}

