#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>  
#include <pthread.h> 
#include "dir.h"
#include "usage.h"

#define PORT 8888

#define BACKLOG 10

//the thread function prototype
void *connection_handler(void *);

int main(int argc, char **argv) {

    int server_sock , client_sock , c , *new_sock;
	
    struct sockaddr_in server , client;
     
    //Create socket
    server_sock = socket(AF_INET , SOCK_STREAM , 0);
	
    if (server_sock == -1) {
        perror("Failed to create socket. Error.\n");
		return 1;
    }
	
    puts("Socket created.");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );
     
    //Bind
    if( bind(server_sock,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("Bind failed. Error.");
        return 1;
    }
	
    puts("Bind done.");
     
    //Listen
    listen(server_sock , BACKLOG);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
    while( (client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
        puts("Connection accepted.");
         
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0) {
            perror("Failed to create thread. Error.");
            return 1;
        }
         
        pthread_join( sniffer_thread , NULL);
		
        puts("Closing connection.");
    }
     
    if (client_sock < 0)
    {
        perror("Accept failed. Error.");
        return 1;
    }
     
    return 0;

}

void *connection_handler(void *server_sock) {

    int sock = *(int*)server_sock;
    int read_size;
    char *message , client_message[2000], command[4], parameter[100];
	int logged_in = 0, ascii_type = 0, stream_mode = 0, fs_type = 0;
     
    message = "<-- Welcome to CPSC 317 Assignment 3 FTP server by Alan Tsin.\n";
    write(sock , message , strlen(message));
     
    message = "<-- 220 To begin, specify your username. This server only supports the username: cs317\n";
    write(sock , message , strlen(message));
     
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ) {
		
		sscanf(client_message, "%s", command);
		
		if (!strcasecmp(command, "USER")) {
			// If not already logged in
			if (logged_in == 0) {
				// Compare provided username with "cs317"
				sscanf(client_message, "%s%s", parameter, parameter);
				if (!strcasecmp(parameter, "cs317")) {
					logged_in = 1;
					message = "<-- 230 Login successful.\n";
					write(sock , message , strlen(message));
				}
				// Does not support any other username
				else {
					message = "<-- 530 This server only supports the username: cs317\n";
					write(sock , message , strlen(message));
				}
			}
			// If already logged in
			else {
				message = "<-- 530 Can't change from cs317 user.\n";
				write(sock , message , strlen(message));
			}
			
		}
		
		else if (!strcasecmp(command, "TYPE")) {
			
			if (logged_in == 1) {
				
				sscanf(client_message, "%s%s", parameter, parameter);
				
				if (!strcasecmp(parameter, "A")) {
					
					if (ascii_type == 0) {
						ascii_type = 1;
						message = "<-- 200 Setting TYPE to ASCII.\n";
						write(sock , message , strlen(message));
					}
					
					else {
						message = "<-- 530 TYPE is already ASCII.\n";
						write(sock , message , strlen(message));
					}
					
				}
				// Does not support any other username
				else if (!strcasecmp(parameter, "I")) {
					
						if (ascii_type == 1) {
						ascii_type = 0;
						message = "<-- 200 Setting TYPE to Image.\n";
						write(sock , message , strlen(message));
					}
					
					else {
						message = "<-- 530 TYPE is already Image.\n";
						write(sock , message , strlen(message));
					}
					
				}
				
				else {
					message = "<-- 530 This server only supports TYPE A and TYPE I.\n";
					write(sock , message , strlen(message));
				}
			}
			
			
			else {
				message = "<-- 530 Must login first.\n";
				write(sock , message , strlen(message));
			}
			
		}
		
		else if (!strcasecmp(command, "MODE")) {
			
			if (logged_in == 1) {
				
				sscanf(client_message, "%s%s", parameter, parameter);
				
				if (!strcasecmp(parameter, "S")) {
					
					if (stream_mode == 0) {
						stream_mode = 1;
						message = "<-- 200 Entering Stream mode.\n";
						write(sock , message , strlen(message));
					}
					
					else {
						message = "<-- 530 Already in Stream mode.\n";
						write(sock , message , strlen(message));
					}
					
				}

				else {
					message = "<-- 530 This server only supports MODE S.\n";
					write(sock , message , strlen(message));
				}
			}
			
			
			else {
				message = "<-- 530 Must login first.\n";
				write(sock , message , strlen(message));
			}
			
		}
		
		else if (!strcasecmp(command, "STRU")) {
			
			if (logged_in == 1) {
				
				sscanf(client_message, "%s%s", parameter, parameter);
				
				if (!strcasecmp(parameter, "F")) {
					
					if (fs_type == 0) {
						fs_type = 1;
						message = "<-- 200 Data Structure set to File Structure.\n";
						write(sock , message , strlen(message));
					}
					
					else {
						message = "<-- 530 Data Structure is already set to File Structure.\n";
						write(sock , message , strlen(message));
					}
					
				}

				else {
					message = "<-- 530 This server only supports STRU F.\n";
					write(sock , message , strlen(message));
				}
			}
			
			
			else {
				message = "<-- 530 Must login first.\n";
				write(sock , message , strlen(message));
			}
			
		}
		
		else if (!strcasecmp(command, "QUIT")) {
			message = "User has quit\n";
			write(sock , message , strlen(message));
			fflush(stdout);
			break;
		}
		
		else {
			message = "<- 530 Unrecognized command. This server only supports: USER, QUIT, TYPE, MODE, STRU, RETR, PASV, NLST.\n";
			write(sock , message , strlen(message));
		}

    }
     
	if(read_size == -1) {
        perror("recv failed");
    }
         
    //Free the socket pointer
    free(server_sock);
     
    return 0;
}
