#include "tecnicofs-client-api.h"
#include "tecnicofs-api-constants.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd, active = 0;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;

//if path is a nullptr this function creates a 
//pseudo-random-enough path for the socket
int setSockAddrUn(char * path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    
    if(path == NULL) sprintf(addr->sun_path,"/tmp/socket%d",getpid());
    else strcpy(addr->sun_path,path);

    return SUN_LEN(addr);
}

//sends command to the server 
void send_server(char * command){ 
    if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(TECNICOFS_ERROR_CONNECTION_ERROR);
    }
}

//receives command result from the server and writes it in 
//the integer pointed by ret
void recv_server(int * ret){
    if(recvfrom(sockfd, ret, sizeof(int), 0,0,0) < 0){
        perror("client: recvfrom error");
        exit(TECNICOFS_ERROR_CONNECTION_ERROR);
    }
}

//creates a node in the server's filesystem
int tfsCreate(char *filename, char nodeType) {
    char buffer[MAX_INPUT_SIZE];
    int ret;

    if(!active){
        perror("tfsCreate: client: doesn't have an active session.");
        exit(TECNICOFS_ERROR_NO_OPEN_SESSION);
    }
    
    sprintf(buffer,"c %s %c\n",filename,nodeType);

    send_server(buffer);
    recv_server(&ret);
    
    return ret;
}

//deletes a node in the server's filesystem
int tfsDelete(char *path) {
    char buffer[MAX_INPUT_SIZE];
    int ret;

    if(!active){
        perror("tfsDelete: client: doesn't have an active session.");
        exit(TECNICOFS_ERROR_NO_OPEN_SESSION);
    }
    
    sprintf(buffer,"d %s\n",path);

    send_server(buffer);
    recv_server(&ret);

    return ret;
}

//moves a node in the server's filesystem
int tfsMove(char *from, char *to) {
    char buffer[MAX_INPUT_SIZE];
    int ret;

    if(!active){
        perror("tfsMove: client: doesn't have an active session.");
        exit(TECNICOFS_ERROR_NO_OPEN_SESSION);
    }
    
    sprintf(buffer,"m %s %s\n",from,to);

    send_server(buffer);
    recv_server(&ret);

    return ret;
}

//looks for a node in the server's filesystem
int tfsLookup(char *path) {
    char buffer[MAX_INPUT_SIZE];
    int ret;
    
    if(!active){
        perror("tfsLookup: client: doesn't have an active session.");
        exit(TECNICOFS_ERROR_NO_OPEN_SESSION);
    }

    sprintf(buffer,"l %s\n",path);

    send_server(buffer);
    recv_server(&ret);
    
    return ret;
}

//prints the filesystem tree to a given path (server side)
int tfsPrint(char *path) {
    char buffer[MAX_INPUT_SIZE];
    int ret;

    if(!active){
        perror("tfsPrint: client: doesn't have an active session.");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    sprintf(buffer,"p %s\n",path);

    send_server(buffer);
    recv_server(&ret);
    
    return ret;
}

//sets up the socket and the client and server addresses 
int tfsMount(char * sockPath) {
    if(active){
        perror("tfsMount: session is already active");
        return TECNICOFS_ERROR_OPEN_SESSION;
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("tfsMount: can't open socket");
        return TECNICOFS_ERROR_OTHER;
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("tfsMount: can't open socket");
        return TECNICOFS_ERROR_OTHER;
    }

    clilen = setSockAddrUn(NULL, &client_addr);

    if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
        perror("tfsMount: bind error");
        return TECNICOFS_ERROR_OTHER;
    }
    
    servlen = setSockAddrUn(sockPath, &serv_addr);

    active = ACTIVE;

    return SUCCESS;
}

//destroys socket 
int tfsUnmount() {
    if(!active){
        perror("tfsUnmount: doesn't have an active session");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    if(close(sockfd) < 0) {
        perror("tfsUnmount: can't close socket file descriptor");
        return TECNICOFS_ERROR_OTHER;
    }

    if(unlink(client_addr.sun_path) != 0){
        perror("tfsUnmount: unlink error");
        return TECNICOFS_ERROR_OTHER;
    }

    active = !ACTIVE;
    
    return SUCCESS;
}
