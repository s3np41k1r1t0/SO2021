#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "state.h"
#include "../tecnicofs-api-constants.h"

#define MUTEX_C 'm'
#define RWLOCK_C 'r'

inode_t inode_table[INODE_TABLE_SIZE];

//variavel que controla qual o sistema de trinco a usar
//caso a variavel nao corresponda a MUTEX ou RWLOCK as funcoes 
//correspondentes aos trincos nao fazem nada
char mode;

//trincos que protegem o filesystem
pthread_mutex_t mutex;
pthread_rwlock_t rwlock;

//inicializa o trinco escolhido
void init(){
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
void destroy(){
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

/*
 * Sleeps for synchronization testing.
 */
void insert_delay(int cycles) {
    for (int i = 0; i < cycles; i++) {}
}

/*
 * Initializes the i-nodes table.
 */
void inode_table_init(char _mode) {
    mode = _mode;
    init();
    
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        inode_table[i].nodeType = T_NONE;
        inode_table[i].data.dirEntries = NULL;
        inode_table[i].data.fileContents = NULL;
    }
}

/*
 * Releases the allocated memory for the i-nodes tables.
 */

void inode_table_destroy() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        if (inode_table[i].nodeType != T_NONE) {
            /* as data is an union, the same pointer is used for both dirEntries and fileContents */
            /* just release one of them */
	  if (inode_table[i].data.dirEntries)
            free(inode_table[i].data.dirEntries);
        }
    }

    destroy();
}

/*
 * Creates a new i-node in the table with the given information.
 * Input:
 *  - nType: the type of the node (file or directory)
 * Returns:
 *  inumber: identifier of the new i-node, if successfully created
 *     FAIL: if an error occurs
 */
int inode_create(type nType) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    lock_read();
    for (int inumber = 0; inumber < INODE_TABLE_SIZE; inumber++) {
        if (inode_table[inumber].nodeType == T_NONE) {
            unlock();

            lock_write();
            inode_table[inumber].nodeType = nType;

            if (nType == T_DIRECTORY) {
                /* Initializes entry table */
                inode_table[inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);
                
                for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                    inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
                }
            }
            else {
                inode_table[inumber].data.fileContents = NULL;
            }

            unlock();
            return inumber;
        }
    }
    
    unlock();
    return FAIL;
}

/*
 * Deletes the i-node.
 * Input:
 *  - inumber: identifier of the i-node
 * Returns: SUCCESS or FAIL
 */
int inode_delete(int inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);
    
    lock_read();
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_delete: invalid inumber\n");
        unlock();
        return FAIL;
    } 
    unlock();

    lock_write();
    inode_table[inumber].nodeType = T_NONE;
    unlock();

    lock_read();
    /* see inode_table_destroy function */
    if (inode_table[inumber].data.dirEntries){
        unlock();
        lock_write();
        free(inode_table[inumber].data.dirEntries);
    }

    unlock();
    return SUCCESS;
}

/*
 * Copies the contents of the i-node into the arguments.
 * Only the fields referenced by non-null arguments are copied.
 * Input:
 *  - inumber: identifier of the i-node
 *  - nType: pointer to type
 *  - data: pointer to data
 * Returns: SUCCESS or FAIL
 */
int inode_get(int inumber, type *nType, union Data *data) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    lock_read();
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_get: invalid inumber %d\n", inumber);
        unlock();
        return FAIL;
    }

    if (nType)
        *nType = inode_table[inumber].nodeType;

    if (data)
        *data = inode_table[inumber].data;
    
    unlock();
    return SUCCESS;
}


/*
 * Resets an entry for a directory.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 * Returns: SUCCESS or FAIL
 */
int dir_reset_entry(int inumber, int sub_inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);
    
    lock_read();
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid inumber\n");
        unlock();
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_reset_entry: can only reset entry to directories\n");
        unlock();
        return FAIL;
    }

    if ((sub_inumber < FREE_INODE) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid entry inumber\n");
        unlock();
        return FAIL;
    }

    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == sub_inumber) {
            unlock();
            lock_write();
            inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
            inode_table[inumber].data.dirEntries[i].name[0] = '\0';
            unlock();
            return SUCCESS;
        }
    }
    
    unlock();
    return FAIL;
}


/*
 * Adds an entry to the i-node directory data.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 *  - sub_name: name of the sub i-node entry 
 * Returns: SUCCESS or FAIL
 */
int dir_add_entry(int inumber, int sub_inumber, char *sub_name) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    lock_read();
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid inumber\n");
        unlock();
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_add_entry: can only add entry to directories\n");
        unlock();
        return FAIL;
    }

    if ((sub_inumber < 0) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid entry inumber\n");
        unlock();
        return FAIL;
    }

    if (strlen(sub_name) == 0 ) {
        printf("inode_add_entry: \
               entry name must be non-empty\n");
        unlock();
        return FAIL;
    }
    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == FREE_INODE) {
            unlock();
            lock_write();
            inode_table[inumber].data.dirEntries[i].inumber = sub_inumber;
            strcpy(inode_table[inumber].data.dirEntries[i].name, sub_name);
            unlock();
            return SUCCESS;
        }
    }
    
    unlock();
    return FAIL;
}


/*
 * Prints the i-nodes table.
 * Input:
 *  - inumber: identifier of the i-node
 *  - name: pointer to the name of current file/dir
 */
void inode_print_tree(FILE *fp, int inumber, char *name) {
    if (inode_table[inumber].nodeType == T_FILE) {
        fprintf(fp, "%s\n", name);
        return;
    }

    if (inode_table[inumber].nodeType == T_DIRECTORY) {
        fprintf(fp, "%s\n", name);
        for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
            if (inode_table[inumber].data.dirEntries[i].inumber != FREE_INODE) {
                char path[MAX_FILE_NAME];
                if (snprintf(path, sizeof(path), "%s/%s", name, inode_table[inumber].data.dirEntries[i].name) > sizeof(path)) {
                    fprintf(stderr, "truncation when building full path\n");
                }
                inode_print_tree(fp, inode_table[inumber].data.dirEntries[i].inumber, path);
            }
        }
    }
}
