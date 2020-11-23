#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define SOCKET_NAME "Soquetxi"

char* serverName;
int sockfd;
socklen_t servlen;
struct sockaddr_un serv_addr;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}


//int tfsCreate(char *filename, char nodeType) {
//  	return -1;
//}

int tfsCreate() {

	servlen = setSockAddrUn(serverName, &serv_addr);

	if (sendto(sockfd, "Hello world", strlen("Hello world")+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
		perror("client: sendto error");
		exit(EXIT_FAILURE);
  	}
  	return -1;
}

int tfsDelete(char *path) {
  	return -1;
}

int tfsMove(char *from, char *to) {
  	return -1;
}

int tfsLookup(char *path) {
  	return -1;
}

int tfsPrint(char *path) {
  	return -1;
}

//TODO check errors
int tfsMount(char * sockPath) {
    struct sockaddr_un client_addr;
    socklen_t clilen;
    char *path;

	serverName = sockPath;

	path = SOCKET_NAME;

	if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        return TECNICOFS_ERROR_OPEN_SESSION;
    }

	unlink(path);

    clilen = setSockAddrUn (path, &client_addr);

	if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
        perror("server: bind error");
        return TECNICOFS_ERROR_OTHER;
    }

  	return 0;
}

int tfsUnmount() {
  	return -1;
}
