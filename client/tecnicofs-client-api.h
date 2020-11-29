#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"

#define ACTIVE 1

int tfsCreate(char *path, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsMount(char *serverName);
int tfsPrint(char *path);
int tfsUnmount();

#endif /* CLIENT_H */
