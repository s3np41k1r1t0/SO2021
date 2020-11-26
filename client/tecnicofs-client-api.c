#include "tecnicofs-client-api.h"
#include "tecnicofs-api-constants.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define MAX_PATH_LEN 30

int sockfd, active = 0;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;

int setSockAddrUn(char * path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    
    if(path == NULL) sprintf(addr->sun_path,"/tmp/socket%d",getpid());
    else strcpy(addr->sun_path,path);

    return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
    char out_buffer[MAX_INPUT_SIZE];
    char in_buffer[MAX_INPUT_SIZE];

    if(!active){
        printf("tfsCreate: client: doesn't have an active session.\n");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    
    sprintf(out_buffer,"c %s %c\n",filename,nodeType);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
        perror("client: recvfrom error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return atoi(in_buffer);
}

int tfsDelete(char *path) {
    char out_buffer[MAX_INPUT_SIZE];
    char in_buffer[MAX_INPUT_SIZE];

    if(!active){
        printf("tfsCreate: client: doesn't have an active session.\n");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    
    sprintf(out_buffer,"d %s\n",path);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
        perror("client: recvfrom error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return atoi(in_buffer);
}

int tfsMove(char *from, char *to) {
    char out_buffer[MAX_INPUT_SIZE];
    char in_buffer[MAX_INPUT_SIZE];

    if(!active){
        printf("tfsCreate: client: doesn't have an active session.\n");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    
    sprintf(out_buffer,"d %s %s\n",from,to);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
        perror("client: recvfrom error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return atoi(in_buffer);
}

int tfsLookup(char *path) {
    char out_buffer[MAX_INPUT_SIZE];
    char in_buffer[MAX_INPUT_SIZE];

    if(!active){
        printf("tfsCreate: client: doesn't have an active session.\n");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    sprintf(out_buffer,"l %s\n",path);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
        perror("client: recvfrom error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    return atoi(in_buffer);
}

int tfsPrint(char *path) {
    char out_buffer[MAX_INPUT_SIZE];
    char in_buffer[MAX_INPUT_SIZE];

    if(!active){
        printf("tfsCreate: client: doesn't have an active session.\n");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    sprintf(out_buffer,"p %s\n",path);

    if (sendto(sockfd, out_buffer, strlen(out_buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("client: sendto error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    if(recvfrom(sockfd, in_buffer, sizeof(in_buffer), 0,0,0) < 0){
        perror("client: recvfrom error");
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    
    return atoi(in_buffer);
}

//TODO check errors
int tfsMount(char * sockPath) {

    if(active){
        return TECNICOFS_ERROR_OPEN_SESSION;
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("client: can't open socket");
        return TECNICOFS_ERROR_OTHER;
    }

    clilen = setSockAddrUn(NULL, &client_addr);

    if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
        perror("client: bind error");
        return TECNICOFS_ERROR_OTHER;
    }
    
    servlen = setSockAddrUn(sockPath, &serv_addr);

    active = 1;

    return 0;
}

int tfsUnmount() {

    if(!active){
        perror("client: doesn't have an active session\n");
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    if(close(sockfd) < 0) return FAIL;

    if(unlink(client_addr.sun_path) != 0){
        perror("client: unlink error");
    }
    
    return SUCCESS;
}
