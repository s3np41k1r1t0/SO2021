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

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define NOSYNC "nosync"
#define MUTEX "mutex"
#define RWLOCK "rwlock"
#define MUTEX_C 'm'
#define RWLOCK_C 'r'

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

//mutex para proteger os comandos de input
pthread_mutex_t mutex_comandos;

//variavel que controla qual o sistema de trinco a usar
//caso a variavel nao corresponda a MUTEX ou RWLOCK as funcoes 
//correspondentes aos trincos nao fazem nada
char mode;

//trincos que protegem o filesystem
pthread_mutex_t mutex;
pthread_rwlock_t rwlock;

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

//inicializa o trinco escolhido
void init_locks(){
    switch(mode){
        case(MUTEX_C):
            if(pthread_mutex_init(&mutex, NULL) != 0){
                fprintf(stderr,"Error initializing mutex\n");
                exit(EXIT_FAILURE);
            }
            break;
        case(RWLOCK_C):
            if(pthread_rwlock_init(&rwlock, NULL) != 0){
                fprintf(stderr,"Error initializing rwlock\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    }
}

//destroi o trinco escolhido
void destroy_locks(){
    switch(mode){
        case(MUTEX_C):
            if(pthread_mutex_destroy(&mutex) != 0){
                fprintf(stderr,"Error destroying mutex\n");
                exit(EXIT_FAILURE);
            }
            break;
        case(RWLOCK_C):
            if(pthread_rwlock_destroy(&rwlock) != 0){
                fprintf(stderr,"Error destroying rwlock\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    }
}

//bloqueia a leitura com o trinco escolhido
void lock_read(){
    switch(mode){
        case(MUTEX_C):
            if(pthread_mutex_lock(&mutex) != 0){
                fprintf(stderr,"Error locking mutex\n");
                exit(EXIT_FAILURE);
            }
            break;
        case(RWLOCK_C):
            if(pthread_rwlock_rdlock(&rwlock) != 0){
                fprintf(stderr,"Error locking rwlock\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    }
}

//bloqueia a escrita com o trinco escolhido
void lock_write(){
    switch(mode){
        case(MUTEX_C):
            if(pthread_mutex_lock(&mutex) != 0){
                fprintf(stderr,"Error locking mutex\n");
                exit(EXIT_FAILURE);
            }
            break;
        case(RWLOCK_C):
            if(pthread_rwlock_wrlock(&rwlock) != 0){
                fprintf(stderr,"Error locking rwlock\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            break;
    }
}

//desbloqueia a leitura/escrita com o trinco escolhido
void unlock(){
    switch(mode){
        case(MUTEX_C):
            if(pthread_mutex_unlock(&mutex) != 0){
                fprintf(stderr,"Error unlocking mutex\n");
                exit(EXIT_FAILURE);
            }
            break;
        case(RWLOCK_C):
            if(pthread_rwlock_unlock(&rwlock) != 0){
                fprintf(stderr,"Error unlocking rwlock\n");
                exit(EXIT_FAILURE);
            }
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
    while (1){
        //protege a var global numberCommands e a queue de comandos
        command_lock();
        const char* command = removeCommand();
        command_unlock();
        
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
                        lock_write();
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        lock_write();
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                unlock();
                break;
            case 'l': 
                lock_read(); 
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                unlock();
                break;
            case 'd':
                lock_write();
                printf("Delete: %s\n", name);
                delete(name);
                unlock();
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
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

//verifica se o numero de threads eh valido
void check_numberThreads(char *numT){
    numberThreads = atoi(numT);
    if(numberThreads < 1 ){
        fprintf(stderr,"Invalid number of threads (must be greater than 0)\n");
        exit(EXIT_FAILURE);
    }
}

//verifica se o ficheiro eh valido e foi aberto sem erros
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
    if(argc != 4){
        fprintf(stderr,"Usage: ./tecnicofs <inputfile> <outputfile> numthreads\n");
        exit(EXIT_FAILURE);
    }

    get_time(&start_time);    

    //verifica o parametro da estrategia de sincronizacao e aplica os comandos lidos previamente
    mode = 'r';
    init_fs();
    
    init_locks();
    command_mutex_init();
    
    //verifica o parametro do numero de threads
    check_numberThreads(argv[3]);

    //verifica se o utilizador tem permissoes para abrir os ficheiros e se ele existem
    inputfile = fopen(argv[1],"r"); 
    check_file_open(inputfile, argv[1]);
    outputfile = fopen(argv[2],"w");
    check_file_open(outputfile, argv[2]);
    
    //le todos os comandos a partir do inputfile e fecha-o
    processInput(inputfile);
    close_file(inputfile, argv[1]);

    //corre as operacoes do filesystem
    applyCommands();

    //escreve o output e fecha o ficheiro
    print_tecnicofs_tree(outputfile);
    close_file(outputfile, argv[2]);
    
    //destroi o filesystem e os mutex/rwlocks
    destroy_fs();
    command_mutex_destroy();
    destroy_locks();

    //calcula a diferenca de tempo e apresenta o benchmark
    get_time(&end_time);   
    delta = (end_time.tv_sec - start_time.tv_sec);
    delta += (end_time.tv_usec - start_time.tv_usec) / 1000000.0;   // microsegundos para segundos
    printf("TecnicoFS completed in %.4f seconds.\n",delta);

    exit(EXIT_SUCCESS);
}
