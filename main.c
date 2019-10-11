/*
	Bind socket to port 8888 on localhost
*/

#include "main.h"

int main(int argv, char *args[]) {
    setbuf(stdout, 0);
    ftpServer();
}

int ftpServer()
{
    //pthread_t thread_id;
    WSADATA wsa;
    SOCKET s, new_socket;
    struct sockaddr_in server, client;
    int c;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }

    printf("Socket created.\n");
    printf("ftpServer.sin_addr.s_addr: %lu\n", server.sin_addr.s_addr);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);;
    server.sin_port = htons(PORT);

    //Bind
    if( bind(s,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return 1;
    }

    puts("Bind done");

    //Listen to incoming connections
    if (listen(s, 3) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return 1;
    }


    //Accept and incoming connection
    puts("Waiting for incoming connections...");

    c = sizeof(struct sockaddr_in);

    while((new_socket = accept(s, (struct sockaddr *)&client, &c)) != INVALID_SOCKET )
    {
        puts("Connection accepted");
        //connection_handler((void *)new_socket);
        HANDLE thread = CreateThread(NULL, 0, &connection_handler, (void *)new_socket, 0, NULL);
    }

    if (new_socket == INVALID_SOCKET)
    {
        printf("accept failed with error code : %d" , WSAGetLastError());
        return 1;
    }

    closesocket(s);
    WSACleanup();

    return 0;
}


/*
 * This will handle connection for each sendDataViaNewConnection
 * */
DWORD WINAPI connection_handler(void* socket_param)
{
    SOCKET socket = (SOCKET) socket_param;
    int read_size;
    char *userName , client_message[2000];
    char command[6];
    char tokens[NUM_CHARS][NUM_WORDS] = {0};
    BOOLEAN isLoggedIn = FALSE;
    BOOLEAN isPassiveMode = FALSE;
    struct sockaddr_in dataAddress;
    char* path = ".";
    //DIR *dr = opendir(".");

/*    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory" );
        return 0;
    }*/

    send220(socket);
    printf("%s\n", IP);
    //Receive a message from sendDataViaNewConnection
    while( (read_size = recv(socket , client_message , 2000 , 0)) > 0 )
    {
        //Send the message back to sendDataViaNewConnection
        //send(socket , client_message , strlen(client_message), 0);
        client_message[read_size] = '\0';
        puts(client_message);
        str_split(client_message, ' ', tokens);//(&(&tokens[0])[0]));
        strcpy(command, tokens[0]);
        if (command[strlen(command) -2] == '\r')
        {
            command[strlen(command) -2] = '\0';
        }
        if (strcmp(command, "QUIT") == 0) {
            send221(socket);
        }
        else if (isLoggedIn) {
            if (strcmp(command, "PORT") == 0) {
                dataAddress = handlePORT(socket, tokens);
            }
            else if (strcmp(command, "USER") == 0 || strcmp(command, "PASS") == 0) {
                send530(socket);
            }
            else if (strcmp(command, "NLST") == 0) {
                handleNLST(socket, dataAddress, path);
            }
            else if (strcmp(command, "XPWD") == 0 || strcmp(command, "PWD") == 0) {
                send257(socket, path);
            }
            else if (strcmp(command, "RETR") == 0) {
                handleRETR(socket, tokens, dataAddress, path);
            }
        }
        else if (strcmp(command, "USER") == 0) {
            if (userName != NULL) {
                free(userName);
            }
            userName = handleUSER(socket, tokens);
        }
        else if (strcmp(command, "PASS") == 0) {
            isLoggedIn = handlePASS(socket, userName, tokens);
        }
        else {
            send530(socket);
        }
    }
    if (userName != NULL) {
        free(userName);
    }
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
    printf("finished connection_handler\n");
    return 0;
}

void handleRETR(SOCKET socket, char tokens[32][32], struct sockaddr_in dataAddress, char *path) {
    FILE *fp;
    int fileSize;
    char *data;

    if (dataAddress.sin_port == 0) {
        return;
    }

    if (tokens[1][strlen(tokens[1]) -2] == '\r')
    {
        tokens[1][strlen(tokens[1]) -2] = '\0';
    }

    fp = fopen(tokens[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open input file\n");
        return;
    }

    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);

    data = (char *)malloc(1 + (fileSize)*sizeof(char)); // Enough memory for file + \0
    fread(data, fileSize, 1, fp); // Read in the entire file
    fclose(fp); // Close the file
    data[fileSize] = '\0';
    putc(data[fileSize], stdout);

    send150(socket);
    sendDataViaNewConnection(dataAddress, data);
    send226(socket);
    printf("before free\n");
    free(data);
    printf("after free\n");
}

void send257(SOCKET socket, char *path) {
    int code = 257;
    char message[100];
    sprintf(message, "%d curr dir is \"%s\". Aviv is the best.\r\n", code, path);
    send(socket , message , (int)strlen(message), 0);
}

void handleNLST(SOCKET socket, struct sockaddr_in dataAddress, char *path) {
    char data[2048] = {0};
    DIR *dr;
    struct dirent *de;  // Pointer for directory entry

    printf("in handleNLST\n");

    if (dataAddress.sin_port != 0) {
        if ((dr = opendir (path)) != NULL) {
            /* print all the files and directories within directory */
            while ((de = readdir (dr)) != NULL) {
                snprintf(data + strlen(data), sizeof(de->d_name), "%s\n", de->d_name);
            }
            closedir(dr);
        }
        else {
            /* could not open directory */
            perror("");
            return;
        }
        send150(socket);
        sendDataViaNewConnection(dataAddress, data);
        send226(socket);
    }
}

void send220(SOCKET socket) {
    int code = 220;
    char message[100];
    sprintf(message, "%d Service ready for new user. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}

void send226(SOCKET socket) {
    int code = 226;
    char message[100];
    sprintf(message, "%d Closing data connection. Requested file action successful.\r\n", code);
    send(socket , message , (int)strlen(message), 0);
    printf("send 226\n");
}

void send150(SOCKET socket) {
    int code = 150;
    char message[100];
    sprintf(message, "%d File status okay; about to open data connection.\r\n", code);
    send(socket , message , (int)strlen(message), 0);
    printf("send 150\n");
}


void send530(SOCKET socket) {
    int code = 520;
    char message[100];
    sprintf(message, "%d Not logged in. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}


void send331(SOCKET socket, char *userName) {
    int code = 330;
    char message[100 + strlen(userName)];
    sprintf(message, "%d Password required for %s. Aviv is the best\r\n", code, userName);
    send(socket , message , (int)strlen(message), 0);
}

void send221(SOCKET socket) {
    int code = 221;
    char message[100];
    sprintf(message, "%d Good Bye. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
    printf("end send221\n");
}

void send230(SOCKET socket) {
    int code = 230;
    char message[100];
    sprintf(message, "%d yay! Login OK. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}

void send200(SOCKET socket) {
    int code = 200;
    char message[100];
    sprintf(message, "%d PORT command Succeeded. consider using PASV.\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}

struct sockaddr_in handlePORT(SOCKET socket, char tokens[NUM_CHARS][NUM_WORDS]) {
    char clientIp[NUM_CHARS];
    int portNum = 0;
    char params[NUM_WORDS][NUM_CHARS];
    struct sockaddr_in dataAddress;
    clientIp[0] = '\0';
    str_split(*(tokens+1), ',', params);

    for (int i = 0; i < 6; ++i) {
        char * curr = *(params+i);

        if (i < 4) {
            int len = (int)strlen(clientIp);
            strcpy(clientIp+len, curr);
            clientIp[strlen(clientIp) + 1] = '\0';

            clientIp[strlen(clientIp)] = '.';
        }
        else if (i ==4) {
            clientIp[strlen(clientIp) - 1] = '\0';
            portNum = 256 * atoi(curr);
        }
        else {
            portNum += atoi(curr);
        }
    }

    dataAddress.sin_addr.s_addr = inet_addr(clientIp);
    dataAddress.sin_family = AF_INET;
    dataAddress.sin_port = htons(portNum);

    send200(socket);
    return dataAddress;
}

char *handleUSER(SOCKET socket, char tokens[NUM_CHARS][NUM_WORDS]) {
    char *userName = malloc(sizeof(*(tokens + 1))+1);
    strcpy(userName, *(tokens + 1));
    userName[strlen(userName)-2] = '\0';
    printf("user name is: %s\n", userName);
    send331(socket, userName);
    return userName;
}

boolean handlePASS(SOCKET socket, char *userName, char tokens[NUM_CHARS][NUM_WORDS]) {
    if (strcmp(userName, "anonymous") != 0) {
        send530(socket);
        return FALSE;
    }
    send230(socket);
    return TRUE;
}


void str_split(char* a_str, const char a_delim, char result[NUM_CHARS][NUM_WORDS])
{
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = '\0';

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    size_t idx  = 0;
    char* token = strtok(a_str, delim);

    while (token)
    {
        //printf("loop %ud\n", idx);
        if (idx < count == FALSE) {
            printf("error in str_split");
        }

        strcpy(*(result + idx++), token);
        //printf("loop %s\n", *(result + idx-1));
        token = strtok(NULL, delim);
    }
    if (idx == count - 1 == FALSE) {
        printf("error in str_split");
    }
}

int sendDataViaNewConnection(struct sockaddr_in address, char *data)
{
    SOCKET s;

    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }

    printf("Socket created.\n");


    //Connect to client
    if (connect(s, (struct sockaddr *)&address , sizeof(address)) < 0)
    {
        puts("connect error");
        return 1;
    }

    puts("Connected");

    //Send some data
    if (send(s, data, strlen(data), 0) < 0) {
        puts("Send failed");
        return 1;
    }
    printf("data sent\n");

    closesocket(s);
    printf("finished sendDataViaNewConnection\n");
    return 0;
}