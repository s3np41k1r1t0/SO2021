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

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define EXIT_CMD "x"

int numberThreads = 0;

int removingThreads = 0;
int printing = 0;

int sockfd;
struct sockaddr_un server_addr;

pthread_mutex_t mutex;

//waiting conditions to insert and remove commands from the queue
pthread_cond_t canWork, canPrint;

//initializes command mutex
void command_mutex_init(){
    if(pthread_mutex_init(&mutex, NULL) != 0){
	fprintf(stderr,"Error initializing mutex\n");
	exit(EXIT_FAILURE);
    }
}

//destroys command mutex
void command_mutex_destroy(){
    if(pthread_mutex_destroy(&mutex) != 0){
	fprintf(stderr,"Error initializing mutex\n");
	exit(EXIT_FAILURE);
    }
}

//locks command mutex
void command_lock(){
    if(pthread_mutex_lock(&mutex) != 0){
	fprintf(stderr,"Error locking mutex\n");
	exit(EXIT_FAILURE);
    }
}

//unlocks command mutex
void command_unlock(){
    if(pthread_mutex_unlock(&mutex) != 0){
	fprintf(stderr,"Error unlocking mutex\n");
	exit(EXIT_FAILURE);
    }
}

//initializes waiting condition to print
void cond_print_init(){
    if(pthread_cond_init(&canPrint, NULL) != 0){
	fprintf(stderr, "Error initializing insert condition\n");
	exit(EXIT_FAILURE);
    }
}

//destroys waiting condition to print
void cond_print_destroy(){
    if(pthread_cond_destroy(&canPrint) != 0){
	fprintf(stderr, "Error destroying insert condition\n");
	exit(EXIT_FAILURE);
    }
}

//TODO add commants
void cond_work_init(){
    if(pthread_cond_init(&canWork, NULL) != 0){
	fprintf(stderr, "Error initializing remove condition\n");
	exit(EXIT_FAILURE);
    }
}


void cond_work_destroy(){
    if(pthread_cond_destroy(&canWork) != 0){
	fprintf(stderr, "Error destroying insert condition\n");
	exit(EXIT_FAILURE);
    }
}

//initializes waiting conditions
void cond_init(){
    cond_print_init();
    cond_work_init();
}

//destroys waiting conditions
void cond_destroy(){
    cond_print_destroy();
    cond_work_destroy();
}

int setSockAddrUn(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
	return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

//TODO check this functions
//verifies if the file is valid and opened without errors
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

void send_client(char *output, struct sockaddr_un* client_addr, socklen_t addrlen){
    if(sendto(sockfd, output, strlen(output), 0,(struct sockaddr *) client_addr, addrlen) < 0){
        fprintf(stderr,"sendto error: %d", errno);
        exit(EXIT_FAILURE);
    } 
}

int removeCommand(char *command, struct sockaddr_un* client_addr, socklen_t* addrlen) {
    command_lock();
    while(printing) pthread_cond_wait(&canWork, &mutex);

    *addrlen = sizeof(struct sockaddr_un);
    int c = recvfrom(sockfd, command, MAX_INPUT_SIZE-1, 0,(struct sockaddr *) client_addr, addrlen);
    
    if (c <= 0) {
        command_unlock(); 
        return FAIL;
    }
    
    command[c] = '\0';

    if(command[0] != 'p') ++removingThreads;
    else ++printing;

    command_unlock();
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
        char command[MAX_INPUT_SIZE], out_buffer[MAX_INPUT_SIZE] = {0};
        
        if (removeCommand(command,&client_addr,&addrlen)){
            continue;
        }

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

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        ret = create(name, T_FILE);
                        sprintf(out_buffer, "%d", ret);
                        send_client(out_buffer,&client_addr,addrlen);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        ret = create(name, T_DIRECTORY);
                        sprintf(out_buffer, "%d", ret);
                        send_client(out_buffer,&client_addr,addrlen);
                        break;
                    default:
                            fprintf(stderr, "Error: invalid node type\n");
                            exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup_read_handler(name);
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                    sprintf(out_buffer, "%d", searchResult);
                    send_client(out_buffer,&client_addr,addrlen);
                }
                else{
                    printf("Search: %s not found\n", name);
                    sprintf(out_buffer, "%d", searchResult);
                    send_client(out_buffer,&client_addr,addrlen);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                ret = delete(name);
                sprintf(out_buffer, "%d", ret);
                send_client(out_buffer,&client_addr,addrlen);
                break;
            case 'm':
                printf("Move: %s to %s\n", name, destination);
                ret = move(name, destination);
                sprintf(out_buffer, "%d", ret);
                send_client(out_buffer,&client_addr,addrlen);
                break;
            case 'p':
                command_lock();
                while(removingThreads || printing > 1) pthread_cond_wait(&canPrint, &mutex);

                printf("Printing to file %s\n", name);
                print_to_file(name);
                send_client("0",&client_addr,addrlen);
                
                --printing;
                if(!printing) pthread_cond_broadcast(&canWork);
                else pthread_cond_signal(&canPrint);

                command_unlock();
                continue;
            default: { /* error */
                send_client("Error: command to apply\n",&client_addr,addrlen);
                exit(EXIT_FAILURE);
            }
        }
    	command_lock(); --removingThreads; if(removingThreads == 0) pthread_cond_signal(&canPrint); command_unlock();
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

    //threads cant end before the input parser
    //processInput(inputStream);

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

int main(int argc, char ** argv){
   
    //checks arguments
    check_arguments(argc);

    //initializes the socket
    init_socket(argv[2]);

    //initializes the filesystem
    init_fs();

    //initializes waiting conditions
    cond_init();

    //initializes command mutex
    command_mutex_init();
    
    //verifies the number of threads
    check_numberThreads(argv[1]);
    
    //does the filesystem operations
    applyCommands();
    
    //destroys the filesystem
    destroy_fs();

    //destroys command mutex
    command_mutex_destroy();
    
    //ends the program successefully
    exit(EXIT_SUCCESS);
}
