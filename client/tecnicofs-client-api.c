#include "tecnicofs-client-api.h"
#include "tecnicofs-api-constants.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define SOCKET_NAME "Soquetxi"
#define MAX_PATH_LEN 30

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

int tfsCreate(char *filename, char nodeType) {
    char out_buffer[MAX_INPUT_SIZE+6];
    char in_buffer[MAX_INPUT_SIZE];
    
    sprintf(out_buffer,"c %s %c\n",filename,nodeType);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
	perror("client: sendto error");
	exit(EXIT_FAILURE);
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
	perror("client: recvfrom error");
	exit(EXIT_FAILURE);
    }

    return atoi(in_buffer);
}

int tfsDelete(char *path) {
    char out_buffer[MAX_INPUT_SIZE+4];
    char in_buffer[MAX_INPUT_SIZE];
    
    sprintf(out_buffer,"d %s\n",path);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
	perror("client: sendto error");
	exit(EXIT_FAILURE);
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
	perror("client: recvfrom error");
	exit(EXIT_FAILURE);
    }

    return atoi(in_buffer);
}

int tfsMove(char *from, char *to) {
    char out_buffer[MAX_INPUT_SIZE+4];
    char in_buffer[MAX_INPUT_SIZE];
    
    sprintf(out_buffer,"d %s %s\n",from,to);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
	perror("client: sendto error");
	exit(EXIT_FAILURE);
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
	perror("client: recvfrom error");
	exit(EXIT_FAILURE);
    }

    return atoi(in_buffer);
}

int tfsLookup(char *path) {
    char out_buffer[MAX_INPUT_SIZE+4];
    char in_buffer[MAX_INPUT_SIZE];

    sprintf(out_buffer,"l %s\n",path);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
	perror("client: sendto error");
	exit(EXIT_FAILURE);
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
	perror("client: recvfrom error");
	exit(EXIT_FAILURE);
    }
    
    return atoi(in_buffer);
}

int tfsPrint(char *path) {
    return -1;

    char out_buffer[MAX_INPUT_SIZE+4];
    char in_buffer[MAX_INPUT_SIZE];

    sprintf(out_buffer,"d %s\n",path);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
	perror("client: sendto error");
	exit(EXIT_FAILURE);
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
	perror("client: recvfrom error");
	exit(EXIT_FAILURE);
    }
    
    return atoi(in_buffer);
}

//TODO check errors
int tfsMount(char * sockPath) {
    struct sockaddr_un client_addr;
    socklen_t clilen;
    char path[MAX_PATH_LEN];

    sprintf(path, "/tmp/%s%d.sock", SOCKET_NAME, getpid());

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
    
    servlen = setSockAddrUn(sockPath, &serv_addr);

    return 0;
}

int tfsUnmount() {
    close(sockfd);
    unlink(SOCKET_NAME);
    return 0;
}
