#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <pthread.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define NOSYNC "nosync"
#define MUTEX "mutex"
#define RWLOCK "rwlock"

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t mutex_comandos = PTHREAD_MUTEX_INITIALIZER;

char mode;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rwlock_w = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rwlock_r = PTHREAD_RWLOCK_INITIALIZER;

int cond = 0;

void lock_read(){
    switch(mode){
        case('m'):
            pthread_mutex_lock(&mutex);
            break;
        case('r'):
            pthread_rwlock_rdlock(&rwlock_r);
            pthread_rwlock_wrlock(&rwlock_w);
            break;
        default:
            break;
    }
}

void lock_write(){
    switch(mode){
        case('m'):
            pthread_mutex_lock(&mutex);
            break;
        case('r'):
            pthread_rwlock_rdlock(&rwlock_r);
            pthread_rwlock_wrlock(&rwlock_w);
            break;
        default:
            break;
    }
}

void unlock(){
    switch(mode){
        case('m'):
            pthread_mutex_unlock(&mutex);
            break;
        case('r'):
            pthread_rwlock_unlock(&rwlock_r);
            pthread_rwlock_unlock(&rwlock_w);
            break;
        default:
            break;
    }
}


int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE *input){
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), input)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
}

void applyCommand(){
    pthread_mutex_lock(&mutex_comandos);
    const char* command = removeCommand();
    pthread_mutex_unlock(&mutex_comandos);

    if (command == NULL){
        return;
    }

    char token, type;
    char name[MAX_INPUT_SIZE];
    int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
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
                    lock_write();
                    create(name, T_FILE);
                    unlock();
                    break;
                case 'd':
                    printf("Create directory: %s\n", name);
                    lock_write();
                    create(name, T_DIRECTORY);
                    unlock();
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }
            break;
        case 'l': 
            lock_read();
            searchResult = lookup(name);
            unlock();
            if (searchResult >= 0)
                printf("Search: %s found\n", name);
            else
                printf("Search: %s not found\n", name);
            break;
        case 'd':
            printf("Delete: %s\n", name);
            lock_write();
            delete(name);
            unlock();
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
}

void applyCommands(){
    pthread_t tid[numberThreads];
    memset(&tid,0,sizeof(tid));

    while (numberCommands > 0){
        for(int i = 0; i < numberThreads && numberCommands > 0; i++)    
            pthread_create(&(tid[i]),NULL,(void *) &applyCommand,NULL);
    
        for(int i = 0; i < numberThreads; i++)
            if(tid[i] != 0) tid[i] = pthread_join(tid[i],NULL); 
    }
}


char check_strategy(char *strategy){
    if(!strcmp(strategy,NOSYNC) || !strcmp(strategy,MUTEX) || !strcmp(strategy,RWLOCK))
        return strategy[0];    
    
    else{
        fprintf(stderr,"Invalid sync strategy\n");
        exit(EXIT_FAILURE);
    }
}

void check_inputfile(FILE *inputfile){
    if(inputfile == NULL){ 
        fprintf(stderr,"Something went wrong while opening the files, please check if input file exists\n");
        exit(EXIT_FAILURE);
    }
}

void check_numberThreads(int numberThreads, char *strategy){
    if(numberThreads < 1 || (numberThreads != 1 && !strcmp(strategy, NOSYNC))){
        fprintf(stderr,"Invalid number of threads (must be greater than 0 or 1 if nosync is enabled)\n");
        exit(EXIT_FAILURE);
    }
}

void check_outputfile(FILE *outputfile){
    if(outputfile == NULL){
        fprintf(stderr,"Cannot open/create output file\n");
        exit(EXIT_SUCCESS);        
    }
}

//Usage: ./tecnicofs <inputfile> <outputfile> numthreads synchstrategy
int main(int argc, char ** argv){
    if(argc != 5) exit(EXIT_FAILURE);

    char *strategy = argv[4];
    
    //starts measuring time in clock cicles to archive better accuracy than time(NULL)
    clock_t start = clock();
   
    //validates the synchstrategy parameter and applies all commands previously read
    //implement strcmp with macros
    mode = check_strategy(strategy);
    
    init_fs();

    FILE *inputfile, *outputfile;
    inputfile = fopen(argv[1],"r");

    //validates if user has permissions to open input file and if it exists
    check_inputfile(inputfile);
     
    numberThreads = atoi(argv[3]);
    
    //validates numthreads parameter
    check_numberThreads(numberThreads, strategy);

    //reads all the commands from the input file
    processInput(inputfile);
    fclose(inputfile);

    applyCommands();

    //writes output to output file
    outputfile = fopen(argv[2],"w");
    
    check_outputfile(outputfile); 

    print_tecnicofs_tree(outputfile);
    fclose(outputfile);
    
    destroy_fs();
    
    clock_t end = clock();
    
    //calculates time diff and displays benchmark
    double delta = ((double)end-start)/CLOCKS_PER_SEC;
    printf("TecnicoFS completed in %.4f seconds.\n",delta);

    exit(EXIT_SUCCESS);
}
