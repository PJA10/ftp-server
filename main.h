
#include <stdio.h>
#include <winsock2.h>
#include <pthread.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <ws2tcpip.h>
#include <dirent.h>
#pragma comment(lib,"ws2_NUM_WORDS.lib") //Winsock Library

#define PORT 21
#define NUM_WORDS 32
#define NUM_CHARS 32
#define IP "0.0.0.0"

int ftpServer();
void str_split(char* a_str, const char a_delim, char [NUM_CHARS][NUM_WORDS]);
DWORD WINAPI connection_handler(void* socket_param);
void send220(SOCKET socket);
void send530(SOCKET socket);
void send331(SOCKET socket, char *userName);
char *handleUSER(SOCKET socket, char tokens[NUM_CHARS][NUM_WORDS]);
boolean handlePASS(SOCKET socket, char *userName, char tokens[NUM_CHARS][NUM_WORDS]);
void send221(SOCKET socket);
struct sockaddr_in handlePORT(SOCKET socket, char tokens[NUM_CHARS][NUM_WORDS]);
int sendDataViaNewConnection(struct sockaddr_in, char *);
void handleNLST(SOCKET socket, struct sockaddr_in dataAddress, DIR *dr);
void send150(SOCKET socket);
void send226(SOCKET socket);
