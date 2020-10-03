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

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

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

void applyCommands(){
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
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
    }
}

void applyMutex(){
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
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
    }
}

//int main(int argc, char* argv[]) {
//    /* init filesystem */
//    init_fs();
//
//    /* process input and print tree */
//    processInput();
//    applyCommands();
//    print_tecnicofs_tree(stdout);
//
//    /* release allocated memory */
//    destroy_fs();
//    exit(EXIT_SUCCESS);
//}

//Usage: ./tecnicofs <inputfile> <outputfile> numthreads synchstrategy
int main(int argc, char ** argv){
    if(argc != 5) exit(EXIT_FAILURE);
    
    //starts measuring time in clock cicles to archive better accuracy than time(NULL)
    clock_t start = clock();
   
    init_fs();
    
    FILE *inputfile, *outputfile;
    inputfile = fopen(argv[1],"r");

    //validates if user has permissions to open input file and if it exists
    if(inputfile == NULL){ 
        perror("Something went wrong while opening the files, please check if input file exists");
        exit(EXIT_FAILURE);
    }
     
    numberThreads = atoi(argv[3]);
    
    //validates numthreads parameter
    if(numberThreads < 1){
        perror("Invalid number of threads (must be greater than 0)");
        exit(EXIT_FAILURE);
    }

    //reads all the commands from the input file
    processInput(inputfile);
    fclose(inputfile);

    //validates the synchstrategy parameter and applies all commands previously read
    //implement strcmp with macros
    if(!strcmp(argv[4],"nosync")){
        if(numberThreads != 1){
            fprintf(stderr,"Invalid number of threads for nosync option (must be 1)\n");
            exit(EXIT_FAILURE);
        }

        applyCommands();
    }
    
    //TBI
    else if(!strcmp(argv[4],"mutex")){
        fprintf(stderr,"Not implemented yet\n");
        exit(EXIT_FAILURE);
    }
    
    //TBI
    else if(!strcmp(argv[4],"rwlock")){
        fprintf(stderr,"Not implemented yet\n");
        exit(EXIT_FAILURE);
    }

    else{
        fprintf(stderr,"Invalid synchstrategy!\n");
        exit(EXIT_FAILURE);
    }

    //writes output to output file
    outputfile = fopen(argv[2],"w");
    
    if(outputfile == NULL){
        fprintf(stderr,"Cannot open/create output file\n");
        exit(EXIT_SUCCESS);        
    } 

    print_tecnicofs_tree(outputfile);
    fclose(outputfile);
    
    destroy_fs();
    
    clock_t end = clock();
    
    //calculates time diff and displays benchmark
    double delta = ((double)end-start)/CLOCKS_PER_SEC;
    printf("TecnicoFS completed in %.4f seconds.\n",delta);

    exit(EXIT_SUCCESS);
}
