#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define WRITE 0
#define READ  1

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {	
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}

	unlock(root);
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	int locks[INODE_TABLE_SIZE] = {0};
	int locks_size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name,WRITE,locks,&locks_size);
	
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	//inode create already locks the child
	
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	unlock(child_inumber);
	undo_locks(locks,locks_size);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	int locks[INODE_TABLE_SIZE] = {0};
	int locks_size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name,WRITE,locks,&locks_size);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		undo_locks(locks,locks_size);
		return FAIL;
	}
	
	lock_write(child_inumber);

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);	
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}
	
	unlock(child_inumber);
	undo_locks(locks,locks_size);
	return SUCCESS;
}

int isSubpath(char *src, char *dst){
    return !strncmp(src,dst,strlen(src));
}

int move(char *name, char *destination){
	int parent_inumber, child_inumber, dest_parent_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	char *dest_parent_name, *dest_child_name, dest_name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, nType;
	union Data pdata, ndata;

	int locks[INODE_TABLE_SIZE] = {0};
	int locks_size = 0;
	
	
	if (isSubpath(name,destination)){
		printf("could not move: destination is a subpath of source\n");
		return FAIL;
	} 

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	//                             a/v           a            v
	strcpy(dest_name_copy, destination);
	split_parent_child_from_path(dest_name_copy, &dest_parent_name, &dest_child_name);
	//                             z                   root                z          

	while((parent_inumber = lookup(parent_name,WRITE,locks,&locks_size)) == -1){
		undo_locks(locks,locks_size); 
		locks_size = 0;
	}

	//							a

	if (parent_inumber == FAIL) {
		printf("could not move: file/directory %s doesn't exist\n", name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);
	
	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	//									v

	if (child_inumber == FAIL){
		printf("could not move: file/directory %s doesn't exist\n", name);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	lock_write(child_inumber);

	//dest_parent_inumber = lookup(dest_parent_name,WRITE,locks,&locks_size);
	//									root
	
	while((dest_parent_inumber = lookup(dest_parent_name,WRITE,locks,&locks_size)) == -1){
		undo_locks(locks,locks_size); 
		locks_size = 0;
	}
	
	if (dest_parent_inumber == FAIL){
		printf("could not move: destination %s doesn't exist\n", dest_parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	} 
	
	inode_get(dest_parent_inumber, &nType, &ndata);

	if (lookup_sub_node(child_name, ndata.dirEntries) != FAIL){
		printf("could not move: file/directory %s already exists\n", name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	if (dir_add_entry(dest_parent_inumber, child_inumber, dest_child_name) == FAIL) {
		printf("could not move %s to dir %s\n",
		       child_name, parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to move %s from dir %s\n",
		       child_name, parent_name);
		unlock(child_inumber);
		undo_locks(locks,locks_size);
		return FAIL;
	}

	unlock(child_inumber);
	undo_locks(locks,locks_size);
	return SUCCESS;
}

// searches for a inumber in a array of inumbers
// n time and 0 space is a not that bad algorithmical time and space complexity
// given that the array of locks might not be sorted...
int inLock(int inumber, int* locks, int size){
	if(locks == NULL) return -1;

	for(int i=0; i<size; i++)
		if(locks[i] == inumber) return 1;

	return 0;
} 

/*
 * Lookup for a given path. Inserts all locked node number in locks.
 * Input:
 *  - name: path of node
 *  - flag: enables WRITE mode
 *  - locks: array of inumbers that are locked
 *  - size: pointer to size of locks
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, char flag, int *locks, int * size) {
	char full_path[MAX_FILE_NAME];
	char * saveptr;
	char delim[] = "/";
	
	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;
	
	char *path = strtok_r(full_path, delim, &saveptr);

	int il = inLock(current_inumber,locks,*size);

	if(!il){
		if(path == NULL && flag == WRITE) {
			if(try_lock_write(current_inumber) == EBUSY) 
				return -1;
		}
		
		else lock_read(current_inumber);

		locks[(*size)++] = current_inumber;
	}

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);

		il = inLock(current_inumber,locks,*size);

		if(!il){
			if(path == NULL && flag == WRITE) {
				if(try_lock_write(current_inumber) == EBUSY) 
					return -1;
			}
			
			else lock_read(current_inumber);

			locks[(*size)++] = current_inumber;
		}

		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}

int lookup_read_handler(char *name){
	int locks[INODE_TABLE_SIZE] = {0}, size = 0;
	int search = lookup(name,READ,locks,&size);
	undo_locks(locks,size);
	return search;
}

void undo_locks(int *locks, int size) {
	for(int i=0; i<size; i++) {
		unlock(locks[i]);
		locks[i] = 0;
	}
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
