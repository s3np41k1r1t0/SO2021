#ifndef FS_H
#define FS_H
#include "state.h"

#define READ  	    1
#define WRITE 	    2
#define TRY_WRITE   3
#define LOCK_BUSY   -23

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int move(char *name, char *destination);
int lookup_read_handler(char *name);
void print_tecnicofs_tree(FILE *fp);
int lookup(char *name, char flag, int *locks, int * size);
void undo_locks(int *locks, int locks_size);

#endif /* FS_H */
