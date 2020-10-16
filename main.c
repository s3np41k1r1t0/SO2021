#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
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

//mutex para proteger os comandos de input
pthread_mutex_t mutex_comandos;

//inicializa o mutex dos comandos
void command_mutex_init(){
    if(pthread_mutex_init(&mutex_comandos, NULL) != 0){
        fprintf(stderr,"Error initializing mutex\n");
        exit(EXIT_FAILURE);
    }
}

//destroi o mutex dos comandos
void command_mutex_destroy(){
    if(pthread_mutex_destroy(&mutex_comandos) != 0){
        fprintf(stderr,"Error initializing mutex\n");
        exit(EXIT_FAILURE);
    }
}

//bloqueia o mutex dos comandos
void command_lock(){
    if(pthread_mutex_lock(&mutex_comandos) != 0){
        fprintf(stderr,"Error locking mutex\n");
        exit(EXIT_FAILURE);
    }
}

//desbloqueia o mutex dos comandos
void command_unlock(){
    if(pthread_mutex_unlock(&mutex_comandos) != 0){
        fprintf(stderr,"Error unlocking mutex\n");
        exit(EXIT_FAILURE);
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
    //protege a var global numberCommands e a queue de comandos
    command_lock();
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }
        command_unlock();

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
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        command_lock(); 
    }
    command_unlock();
}

//cria as threads e junta-as
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

//verifica se a estrategia eh valida e retorna o char correspondente 'a estrategia escolhida 
char check_strategy(char *strategy){
    if(!strcmp(strategy,NOSYNC) || !strcmp(strategy,MUTEX) || !strcmp(strategy,RWLOCK))
        return strategy[0];    
    
    fprintf(stderr,"Invalid sync strategy\n");
    exit(EXIT_FAILURE);
}

//verifica se o ficheiro de input eh valido
void check_inputfile(FILE *inputfile){
    if(inputfile == NULL){ 
        fprintf(stderr,"Something went wrong while opening the files, please check if input file exists\n");
        exit(EXIT_FAILURE);
    }
}

//verifica se o numero de threads eh valido
void check_numberThreads(char *numT, char *strategy){
    numberThreads = atoi(numT);
    if(numberThreads < 1 || (numberThreads != 1 && !strcmp(strategy, NOSYNC))){
        fprintf(stderr,"Invalid number of threads (must be greater than 0 or 1 if nosync is enabled)\n");
        exit(EXIT_FAILURE);
    }
}

//verfica se o ficheiro de output eh valido
void check_outputfile(FILE *outputfile){
    if(outputfile == NULL){
        fprintf(stderr,"Cannot open/create output file\n");
        exit(EXIT_SUCCESS);        
    }
}

//verifica se o ficheiro foi aberto sem erros
void check_file_open(FILE *file, char *file_name){
    if(file == NULL){
        fprintf(stderr,"Cannot open/create file: %s\n", file_name);
        exit(EXIT_SUCCESS);
    }
}

//fecha o ficheiro e verifica se o fechou sem erros
void close_file(FILE *file, char *file_name){
    if(fclose(file) != 0){
        fprintf(stderr,"Cannot close file: %s\n", file_name);
        exit(EXIT_SUCCESS);        
    }
}

//coloca o valor do tempo atual na estrutura time
void get_time(struct timeval *time){
    if (gettimeofday(time,NULL) != 0){
        fprintf(stderr,"Error getting the time of the day\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char ** argv){
    struct timeval start_time, end_time;
    double delta;
    FILE *inputfile, *outputfile;
   
    //verifica o numero de argumentos
    if(argc != 5){
        fprintf(stderr,"Usage: ./tecnicofs <inputfile> <outputfile> numthreads synchstrategy\n");
        exit(EXIT_FAILURE);
    }

    get_time(&start_time);    

    //verifica o parametro da estrategia de sincronizacao e aplica os comandos lidos previamente
    init_fs(check_strategy(argv[4]));
    
    command_mutex_init();
    
    //verifica o parametro do numero de threads
    check_numberThreads(argv[3], argv[4]);

    //verifica se o utilizador tem permissoes para abrir o ficheiro e se ele existe
    inputfile = fopen(argv[1],"r"); 
    check_file_open(inputfile, argv[1]);
    
    //le todos os comandos a partir do inputfile e fecha-o
    processInput(inputfile);
    close_file(inputfile, argv[1]);

    //corre as operacoes do filesystem
    applyCommands();

    //abre o outputfile, escreve o output e fecha o ficheiro
    outputfile = fopen(argv[2],"w");
    check_file_open(outputfile, argv[2]);
    print_tecnicofs_tree(outputfile);
    close_file(outputfile, argv[2]);
    
    //destroi o filesystem e os mutex/rwlocks
    destroy_fs();
    command_mutex_destroy();

    //calcula a diferenca de tempo e apresenta o benchmark
    get_time(&end_time);   
    delta = (end_time.tv_sec - start_time.tv_sec);
    delta += (end_time.tv_usec - start_time.tv_usec) / 1000000.0;   // microsegundos para segundos
    printf("TecnicoFS completed in %.4f seconds.\n",delta);

    exit(EXIT_SUCCESS);
}