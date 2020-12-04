// Andre Martins Esgalhado - 95533
// Bruno Miguel Da Silva Mendes - 95544
// Grupo - 16

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs/operations.h"
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

int sockfd;
struct sockaddr_un server_addr;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
	return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

//checks if a file was successefully opened
int check_file_open(FILE *file, char *file_name){
    if(file == NULL){
        fprintf(stderr,"Cannot open/create file: %s\n", file_name);
        return FAIL;
    }
    return SUCCESS;
}

//closes the file and checks for errors
void close_file(FILE *file, char *file_name){
    if(fclose(file) != 0){
        fprintf(stderr,"Cannot close file: %s\n", file_name);
        exit(EXIT_FAILURE);
    }
}

//sends result to specified client
void send_client(int * output, struct sockaddr_un* client_addr, socklen_t addrlen){
    if(sendto(sockfd, output, sizeof(int), 0,(struct sockaddr *) client_addr, addrlen) < 0){
        fprintf(stderr,"sendto error: %d", errno);
        exit(EXIT_FAILURE);
    } 
}

//receives command from a client and stores it in the command pointer
int removeCommand(char *command, struct sockaddr_un* client_addr, socklen_t* addrlen) {
    *addrlen = sizeof(struct sockaddr_un);
    int c = recvfrom(sockfd, command, MAX_INPUT_SIZE-1, 0,(struct sockaddr *) client_addr, addrlen);
    
    if (c <= 0) return FAIL;
    
    command[c] = '\0';

    return SUCCESS;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void applyCommand(){
    while (1){
        struct sockaddr_un client_addr;
        socklen_t addrlen;
        char command[MAX_INPUT_SIZE];
        
        if (removeCommand(command,&client_addr,&addrlen)) continue;

        char token, type, name[MAX_INPUT_SIZE], destination[MAX_INPUT_SIZE];
        int ret;

        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

        if (token == 'm'){
            numTokens = sscanf(command, "%c %s %s", &token, name, destination);
        }

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        ret = create(name, T_FILE);
                        send_client(&ret,&client_addr,addrlen);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        ret = create(name, T_DIRECTORY);
                        send_client(&ret,&client_addr,addrlen);
                        break;
                    default:
			fprintf(stderr, "Error: invalid node type\n");
			exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                ret = lookup_read_handler(name);
                if (ret >= 0){
                    printf("Search: %s found\n", name);
                    send_client(&ret,&client_addr,addrlen);
                }
                else{
                    printf("Search: %s not found\n", name);
                    send_client(&ret,&client_addr,addrlen);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                ret = delete(name);
                send_client(&ret,&client_addr,addrlen);
                break;
            case 'm':
                printf("Move: %s to %s\n", name, destination);
                ret = move(name, destination);
                send_client(&ret,&client_addr,addrlen);
                break;
            case 'p':
		printf("Printing to file %s\n", name);
                ret = print_tecnicofs_tree(name);
                send_client(&ret,&client_addr,addrlen);
                continue;
            default: { /* error */
		printf("Error: command to apply\n");
		ret = FAIL;
                send_client(&ret,&client_addr,addrlen);
                exit(EXIT_FAILURE);
            }
        }
    }
}

//creates the threads and joins them
void applyCommands(){
    pthread_t tid[numberThreads];

    for(int i = 0; i < numberThreads; i++){    
        if(pthread_create(&(tid[i]),NULL,(void *) &applyCommand,NULL) != 0){
            fprintf(stderr,"Error creating thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < numberThreads; i++){
        if(pthread_join(tid[i],NULL) != 0){
            fprintf(stderr,"Error joining thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

//verifies if the number of threads is valid
void check_numberThreads(char *numT){
    numberThreads = atoi(numT);
    if(numberThreads < 1 ){
        fprintf(stderr,"Invalid number of threads (must be greater than 0)\n");
        exit(EXIT_FAILURE);
    }
}

//initializes the socket
void init_socket(char *path){
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    //error is not verified on purpouse
    //file may not exist
    unlink(path);

    socklen_t addrlen = setSockAddrUn (path, &server_addr);

    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
}

//checks arguments
void check_arguments(int argc){
    if(argc != 3){
        fprintf(stderr,"Usage: ./tecnicofs numthreads nomesocket\n");
        exit(EXIT_FAILURE);
    }
}

//destroys the server after a SIGINT
void destroy_socket(int signum){
    destroy_fs();

    if(close(sockfd) < 0){
	perror("Cannot close socket fd");
	exit(EXIT_FAILURE);
    }
    
    if(unlink(server_addr.sun_path) < 0){
	perror("Cannot unlink socket path");
	exit(EXIT_FAILURE);
    }

    puts("\nGraceful Shutdown successeful");
    exit(EXIT_SUCCESS);
}

int main(int argc, char ** argv){
    //use ctrl-c to gracefully shutdown the server
    signal(SIGINT,destroy_socket);

    //checks arguments
    check_arguments(argc);

    //initializes the socket
    init_socket(argv[2]);

    //initializes the filesystem
    init_fs();
    
    //verifies the number of threads
    check_numberThreads(argv[1]);
    
    //does the filesystem operations
    applyCommands();
    
    //destroys the filesystem
    destroy_fs();

    //ends the program successefully
    exit(EXIT_SUCCESS);
}
