#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup_read_handler(char *name);
int move(char *name, char *destination);
void print_tecnicofs_tree(FILE *fp);
int lookup(char *name, char flag, int *locks, int * size);
void undo_locks(int *locks, int locks_size);
int lookup_dst(char *name, char flag, int *locks, int * size, int *d, int ds);

#endif /* FS_H */
