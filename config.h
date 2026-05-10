#ifndef CONFIG_H
#define CONFIG_H
int readconfig(const char* filename, char* server_addr, int* port);

int readconfig_server(const char* filename, char* access_token);
#endif