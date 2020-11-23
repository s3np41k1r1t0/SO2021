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

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define EXIT_CMD "x"

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
int indexInsert = 0, indexRemove = 0;
int waitingThreads = 0;
int printing = 0;

int sockfd;
struct sockaddr_un server_addr, client_addr;
socklen_t addrlen;

//mutex to protect input commands
pthread_mutex_t mutex_comandos;

//waiting conditions to insert and remove commands from the queue
pthread_cond_t canInsert, canRemove;
pthread_cond_t canPrint;

//initializes command mutex
void command_mutex_init(){
    if(pthread_mutex_init(&mutex_comandos, NULL) != 0){
	fprintf(stderr,"Error initializing mutex\n");
	exit(EXIT_FAILURE);
    }
}

//destroys command mutex
void command_mutex_destroy(){
    if(pthread_mutex_destroy(&mutex_comandos) != 0){
	fprintf(stderr,"Error initializing mutex\n");
	exit(EXIT_FAILURE);
    }
}

//locks command mutex
void command_lock(){

    if(pthread_mutex_lock(&mutex_comandos) != 0){
	fprintf(stderr,"Error locking mutex\n");
	exit(EXIT_FAILURE);
    }
}

//unlocks command mutex
void command_unlock(){
    if(pthread_mutex_unlock(&mutex_comandos) != 0){
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

//initializes waiting condition to insert commands
void cond_insert_init(){
    if(pthread_cond_init(&canInsert, NULL) != 0){
	fprintf(stderr, "Error initializing insert condition\n");
	exit(EXIT_FAILURE);
    }
}

//initializes waiting condition to remove commands
void cond_remove_init(){
    if(pthread_cond_init(&canRemove, NULL) != 0){
	fprintf(stderr, "Error initializing remove condition\n");
	exit(EXIT_FAILURE);
    }
}

//destroys waiting condition to insert commands
void cond_insert_destroy(){
    if(pthread_cond_destroy(&canInsert) != 0){
	fprintf(stderr, "Error destroying insert condition\n");
	exit(EXIT_FAILURE);
    }
}

//destroys waiting condition to remove commands
void cond_remove_destroy(){
    if(pthread_cond_destroy(&canRemove) != 0){
	fprintf(stderr, "Error destroying remove condition\n");
	exit(EXIT_FAILURE);
    }
}

//initializes waiting conditions
void cond_init(){
    cond_insert_init();
    cond_remove_init();
    cond_print_init();
}

//destroys waiting conditions
void cond_destroy(){
    cond_insert_destroy();
    cond_remove_destroy();
    cond_print_destroy();
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

//int insertCommand(char* data) {
//    command_lock();
//    while(numberCommands == MAX_COMMANDS) pthread_cond_wait(&canInsert, &mutex_comandos);
//
//    strcpy(inputCommands[indexInsert], data);
//
//    indexInsert++;
//    indexInsert%=MAX_COMMANDS;
//    numberCommands++;
//
//    //if it is the exit command signal all threads to remove commands
//    if(!strcmp(data,EXIT_CMD)) pthread_cond_broadcast(&canRemove);
//    else pthread_cond_signal(&canRemove);
//
//    command_unlock();
//
//    return 1;
//}

void send_client(char *output){
    sendto(sockfd, output, strlen(output), 0, (struct sockaddr *)&client_addr, addrlen);
}

int removeCommand(char *command) {
    //command_lock();
    //waitingThreads++;
    //if(waitingThreads == numberThreads-1)pthread_cond_signal(&canPrint);
    //while(numberCommands == 0 || printing) pthread_cond_wait(&canRemove, &mutex_comandos);

    //strcpy(command,inputCommands[indexRemove]);

    addrlen = sizeof(struct sockaddr_un);
    int c = recvfrom(sockfd, command, MAX_INPUT_SIZE-1, 0, (struct sockaddr *)&client_addr, &addrlen);
    
    if (c <= 0) return 1;
    
    command[c] = '\0';
    
    //command_unlock();

    return 0;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

//void processInput(FILE *input){
//    char line[MAX_INPUT_SIZE];
//
//    /* break loop with ^Z or ^D */
//    while (fgets(line, sizeof(line)/sizeof(char), input)) {
//        char token, type;
//        char name[MAX_INPUT_SIZE];
//
//        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);
//
//        /* perform minimal validation */
//        if (numTokens < 1) {
//            continue;
//        }
//        switch (token) {
//            case 'c':
//                if(numTokens != 3)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//            
//            case 'l':
//                if(numTokens != 2)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//            
//            case 'd':
//                if(numTokens != 2)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//
//            case 'm':
//                if(numTokens != 3)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//            
//            case 'p':
//                if(numTokens != 2)
//                    errorParse();
//                if(insertCommand(line))
//                    break;
//                return;
//
//            case '#':
//                break;
//
//            default: { /* error */
//                errorParse();
//            }
//        }
//    }
//    //inserts symbolic command when there are no more commands
//    insertCommand(EXIT_CMD);
//}

void applyCommand(){
    while (1){
        char command[MAX_INPUT_SIZE], out_buffer[300] = {0};
        
        if (removeCommand(command)){
            return;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        char destination[MAX_INPUT_SIZE];
        FILE *outputfile;
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
                        sprintf(out_buffer, "%d\n", ret);
                        send_client(out_buffer);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        ret = create(name, T_DIRECTORY);
                        sprintf(out_buffer, "this is a really long buffer %d", ret);
                        send_client(out_buffer);
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
                    sprintf(out_buffer, "%d", ret);
                    send_client(out_buffer);
                }
                else{
                    printf("Search: %s not found\n", name);
                    sprintf(out_buffer, "%d", ret);
                    send_client(out_buffer);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                ret = delete(name);
                sprintf(out_buffer, "%d", ret);
                send_client(out_buffer);
                break;
            case 'm':
                sprintf(out_buffer, "Move: %s to %s\n", name, destination);
                ret = move(name, destination);
                sprintf(out_buffer, "%d", ret);
                send_client(out_buffer);
                break;
            case 'p':
                //command_lock();
                //printing = 1;
                //while (waitingThreads != numberThreads-1) pthread_cond_wait(&canPrint, &mutex_comandos);
                printf("Printing in file %s\n", name);
                outputfile = fopen(name,"w");
                if(check_file_open(outputfile, name) != FAIL){
                    print_tecnicofs_tree(outputfile);
                    close_file(outputfile, name);
                }
                //sprintf(out_buffer, "%d", ret);
                //send_client(out_buffer);
                //printing = 0;
                //pthread_cond_broadcast(&canRemove);
                //command_unlock();
                break;
            default: { /* error */
                send_client("Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

//creates the threads and joins them
void applyCommands(){
    applyCommand();

    /*pthread_t tid[numberThreads];

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
    }*/
}

//verifies if the number of threads is valid
void check_numberThreads(char *numT){
    numberThreads = atoi(numT);
    if(numberThreads < 1 ){
        fprintf(stderr,"Invalid number of threads (must be greater than 0)\n");
        exit(EXIT_FAILURE);
    }
}

//sets the current time
//void get_time(struct timeval *time){
//    if (gettimeofday(time,NULL) != 0){
//        fprintf(stderr,"Error getting the time of the day\n");
//        exit(EXIT_FAILURE);
//    }
//}

int main(int argc, char ** argv){
    char *path;
   
    //verifica o numero de argumentos
    if(argc != 3){
        fprintf(stderr,"Usage: ./tecnicofs numthreads nomesocket\n");
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    path = argv[2];

    unlink(path);

    addrlen = setSockAddrUn (path, &server_addr);

    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

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
