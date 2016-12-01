#include <stdio.h>   
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>    
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>  
#include <pthread.h> 
#include "dir.h"
#include "usage.h"

#define BACKLOG 10

int port_number;

// Thread function prototype
void *connection_handler(void *);

// Function for sending formatted strings prototype
int send_str(int, const char*, ...);

// Functions for RETR
int send_path(int peer, char *file, uint32_t offset);
int send_file(int peer, FILE *f);

int main(int argc, char **argv) {
	
	if (argc != 2) {
		puts("Require only port number in argument. ./CSftp PORTNUMBER");
		return 0;
	}
	
	port_number = atoi(argv[1]);

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
    server.sin_port = htons( port_number );
     
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
	
	struct sockaddr_in svr_addr;
	int svr_addr_len = sizeof(svr_addr);
	getsockname(sock, (struct sockaddr*)&svr_addr, &svr_addr_len);

	int data_client = -1;
    struct sockaddr_in data_client_addr;
	int data_client_len = sizeof(data_client_addr);
	char cwd[1024];
	
    char *message , client_message[2000], command[4], parameter[1000];
	int logged_in = 0, ascii_type = 0, stream_mode = 0, fs_type = 0, passive_mode = 0;
	int pasv_port, pasv_sock;
	struct sockaddr_in pasv_server;
     
    message = "220 - Welcome to CPSC 317 Assignment 3 FTP server by Alan Tsin.\n";
    write(sock , message , strlen(message));
     
    message = "220 - To begin, specify your username. This server only supports the username: cs317\n";
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
					send_str(sock, "230 Login successful.\n");
				}
				// Does not support any other username
				else {
					send_str(sock, "430 This server only supports the username: cs317\n");
				}
			}
			// If already logged in
			else {
				send_str(sock, "430 Can't change from cs317 user.\n");
			}
			
		}
		
		else if (!strcasecmp(command, "TYPE")) {
			
			if (logged_in == 1) {
				
				sscanf(client_message, "%s%s", parameter, parameter);
				
				if (!strcasecmp(parameter, "A")) {
					
					if (ascii_type == 0) {
						ascii_type = 1;
						send_str(sock, "200 Setting TYPE to ASCII.\n"); 
					}
					
					else {
						send_str(sock, "200 TYPE is already ASCII.\n");
					}
					
				}
				// Does not support any other username
				else if (!strcasecmp(parameter, "I")) {
					
						if (ascii_type == 1) {
						ascii_type = 0;
						send_str(sock, "200 Setting TYPE to Image.\n");
					}
					
					else {
						send_str(sock, "200 TYPE is already Image.\n"); 
					}
					
				}
				
				else {
					send_str(sock, "504 This server only supports TYPE A and TYPE I.\n");
				}
			}
			
			
			else {
				send_str(sock, "530 - Must login first.\n");
			}
			
		}
		
		else if (!strcasecmp(command, "MODE")) {
			
			if (logged_in == 1) {
				
				sscanf(client_message, "%s%s", parameter, parameter);
				
				if (!strcasecmp(parameter, "S")) {
					
					if (stream_mode == 0) {
						stream_mode = 1;
						send_str(sock, "200 Entering Stream mode.\n"); 
					}
					
					else {
						send_str(sock, "200 Already in Stream mode.\n");
					}
					
				}

				else {
					send_str(sock, "504 This server only supports MODE S.\n");
				}
			}
			
			
			else {
				send_str(sock, "530 Must login first.\n");
			}
			
		}
		
		else if (!strcasecmp(command, "STRU")) {
			
			if (logged_in == 1) {
				
				sscanf(client_message, "%s%s", parameter, parameter);
				
				if (!strcasecmp(parameter, "F")) {
					
					if (fs_type == 0) {
						fs_type = 1;
						send_str(sock, "200 Data Structure set to File Structure.\n");
					}
					
					else {
						send_str(sock, "200 Data Structure is already set to File Structure.\n");
					}
					
				}

				else {
					send_str(sock, "504 This server only supports STRU F.\n");
				}
			}
			
			
			else {
				send_str(sock, "530 Must login first.\n");
			}
			
		}
		
		else if (!strcasecmp(command, "PASV")) {
			
			if (passive_mode == 0) {
				// Loop until a passive socket is successfully created
				
				do {
					// Pick a random port number
					pasv_port = (rand() % 64512 + 1024);
					// Create new socket
					pasv_sock = socket(AF_INET , SOCK_STREAM , 0);
				
					pasv_server.sin_family = AF_INET;
					pasv_server.sin_addr.s_addr = INADDR_ANY;
					pasv_server.sin_port = htons( pasv_port );
					
				} while ( bind(pasv_sock,(struct sockaddr *)&pasv_server , sizeof(pasv_server)) < 0 );

				if (pasv_sock < 0) {
					send_str(sock, "500 Error entering Passive mode.\n");
				}
			
				else {
					listen(pasv_sock , 1);
					passive_mode = 1;
					
					uint32_t t = svr_addr.sin_addr.s_addr;
					
					send_str(sock, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\n", 
									t&0xff, 
									(t>>8)&0xff, 
									(t>>16)&0xff, 
									(t>>24)&0xff, 
									pasv_port>>8, 
									pasv_port & 0xff);
					
				}
			}
			
			else {
				send_str(sock, "227 Already in passive mode. Port number: %d\n", pasv_port);
			}
			
		}
		
		else if (!strcasecmp(command, "NLST")) {
			
			if (logged_in == 1) {
			
				if (passive_mode == 1) {
						
					if (pasv_port > 1024 && pasv_port <= 65535 && pasv_sock >= 0) {
						ascii_type = 1;
						send_str(sock, "150 Here comes the directory listing.\n");
						
						listen(pasv_sock, BACKLOG);
						data_client = accept(pasv_sock, (struct sockaddr *)&data_client_addr, &data_client_len);
						
						getcwd(cwd, sizeof(cwd));
						listFiles(data_client, cwd);
						send_str(sock, "226 Transfer complete.\n");
						
						close(data_client);
						data_client = -1;
						
						close(pasv_sock);
						passive_mode = 0;
					}
						
					else {
						send_str(sock, "500 No passive server created.\n");
					}
				}
				
				else {
					send_str(sock, "425 Use PASV first.\n");
				}
			}
			
			else {
				send_str(sock, "530 Must login first.\n");
			}
			
		}
		
		else if (!strcasecmp(command, "RETR")) {
			
			if (logged_in == 1) {
			
				if (passive_mode == 1) {
						
					if (pasv_port > 1024 && pasv_port <= 65535 && pasv_sock >= 0) {
						ascii_type = 0;
						send_str(sock, "150 Opening binary mode data connection.\n");
						listen(pasv_sock, BACKLOG);
						data_client = accept(pasv_sock, (struct sockaddr *)&data_client_addr, &data_client_len);
						
						sscanf(client_message, "%s%s", parameter, parameter);
						
						int st = send_path(data_client, parameter, 0);
						
						if (st >= 0) {
							send_str(sock, "226 Transfer complete.\n");
						}
						
						else {
							send_str(sock, "451 File not found.\n");
						}
						
							close(data_client);
							data_client = -1;
						
							close(pasv_sock);
							passive_mode = 0;
						
					}
						
					else {
						send_str(sock, "500 No passive server created.\n");
					}
				}
				
				else {
					send_str(sock, "425 Use PASV first.\n");
				}
			}
			
			else {
				send_str(sock, "530 Must login first.\n");
			}
			
		}
		
		else if (!strcasecmp(command, "QUIT")) {
			send_str(sock, "221 User has quit. Terminating connection.\n");
			fflush(stdout);
			close(sock);
			close(pasv_sock);
			break;
		}
		
		else {
			send_str(sock, "530 Unrecognized command. This server only supports: USER, QUIT, TYPE, MODE, STRU, RETR, PASV, NLST.\n");
		}

    }
     
	if(read_size == -1) {
        perror("recv failed");
    }
     
    return 0;
}

int send_str(int peer, const char* fmt, ...) {
    va_list args;
    char msgbuf[1024];
    va_start(args, fmt);
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
    va_end(args);
    return write(peer , msgbuf , strlen(msgbuf));
}

int send_path(int peer, char *file, uint32_t offset) {
    FILE *f = fopen(file, "rb");
    if (f) {
        fseek(f, offset, SEEK_SET);
        int st = send_file(peer, f);
        if (st < 0) {
            return -2;
        }
    } else {
        return -1;
    }
    int ret = fclose(f);
    return ret == 0 ? 0 : -3;
}

int send_file(int peer, FILE *f) {
    char filebuf[1025];
    int n, ret = 0;
    while ((n = fread(filebuf, 1, 1024, f)) > 0) {
        int st = send(peer, filebuf, n, 0);
		
        if (st < 0) {
            ret = -1;
            break;
        } 
		
		else {
            filebuf[n] = 0;
        }
    }
    return ret;
}