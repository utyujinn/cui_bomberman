#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int readconfig(const char* filename, char* server_addr, int* port) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }

    char key[256];
    char value[256];
    while (fscanf(fp, "%255[^=]=%255s\n", key, value) != EOF) {
        if (strcmp(key, "host") == 0) {
            strncpy(server_addr, value, 255);
        } else if (strcmp(key, "port") == 0) {
            *port = atoi(value);
        }
    }

    if(server_addr[0] == '\0' || *port == 0) {
        strncpy(server_addr, "localhost", 255);
        *port = 8000;
    }

    fclose(fp);
    return 0;
}

int readconfig_server(const char* filename, char* access_token) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }

    char key[256];
    char value[256];
    while (fscanf(fp, "%255[^=]=%255s\n", key, value) != EOF) {
        if(strcmp(key, "ACCESS_TOKEN") == 0) {
            strncpy(access_token, value, 511);
        }
    }

    fclose(fp);
    return 0;
}