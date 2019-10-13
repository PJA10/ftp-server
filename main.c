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
 * This will handle connection for each sendRecvDataViaNewConnection
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
    char path[PATH_MAX];
    if (getcwd(path, sizeof(path)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    send220(socket);
    printf("%s\n", IP);
    //Receive a message from sendRecvDataViaNewConnection
    while( (read_size = recv(socket , client_message , 2000 , 0)) > 0 )
    {
        //Send the message back to sendRecvDataViaNewConnection
        //send(socket , client_message , strlen(client_message), 0);
        client_message[read_size] = '\0';
        puts(client_message);
        strcpy(tokens[1], "");
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
                handleNLST(socket, tokens, dataAddress, path);
            }
            else if (strcmp(command, "XPWD") == 0 || strcmp(command, "PWD") == 0) {
                send257(socket, path);
            }
            else if (strcmp(command, "RETR") == 0) {
                handleRETR(socket, tokens, dataAddress, path);
            }
            else if (strcmp(command, "CWD") == 0) {
                handleCWD(socket, tokens, path);
            }
            else if (strcmp(command, "LIST") == 0) {
                handleLIST(socket, tokens, dataAddress, path);
            }
            else if (strcmp(command, "DELE") == 0) {
                handleDELE(socket, tokens, path);
            }
            else if (strcmp(command, "STOR") == 0) {
                handleSTOR(socket, tokens, dataAddress, path);
            }
            else {
                send502(socket);
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

void handleSTOR(SOCKET socket, char tokens[NUM_WORDS][NUM_CHARS], struct sockaddr_in dataAddress, char *path) {
    char data[BUFFER_SIZE];
    FILE *newFilePtr;
    char newFilePath[PATH_MAX];

    if (tokens[1][strlen(tokens[1]) -2] == '\r')
    {
        tokens[1][strlen(tokens[1]) -2] = '\0';
    }

    send150(socket);
    sendRecvDataViaNewConnection(dataAddress, data, TRUE);
    send226(socket);

    sprintf(newFilePath, "%s\\%s", path, tokens[1]);
    newFilePtr = fopen(newFilePath, "w");
    if (newFilePtr == NULL) {
        printf("error in fopen in handleSTOR\n");
        return;
    }
    fputs(data, newFilePtr);
    fclose(newFilePtr);
}

void handleDELE(SOCKET socket, char tokens[NUM_WORDS][NUM_CHARS], char *path) {
    char command[MAX_COMMAND];
    FILE *outputFile;
    char filePath[MAX_PATH];
    int size;

    if (tokens[1][strlen(tokens[1]) -2] == '\r')
    {
        tokens[1][strlen(tokens[1]) -2] = '\0';
    }

    sprintf(command, "cd %s && del %s 2> output.txt", path, tokens[1]);
    system(command);

    sprintf(filePath, "%s\\output.txt", path);
    outputFile = fopen(filePath, "r");
    if (outputFile == NULL) {
        printf("error in gnalleDele\n");
        return;
    }
    fseek (outputFile, 0, SEEK_END);
    size = ftell(outputFile);

    if (0 == size) {
        // file is empty there was no error
        send250(socket);
    }
    else {
        send550(socket);
    }
    fclose(outputFile);

    sprintf(command, "del \"%s\\output.txt\"", path);
    system(command);
}

void handleCWD(SOCKET socket, char tokens[NUM_WORDS][NUM_CHARS], char *path) {
    char command[MAX_COMMAND];
    int retValue;
    if (tokens[1][strlen(tokens[1]) -2] == '\r')
    {
        tokens[1][strlen(tokens[1]) -2] = '\0';
    }

    sprintf(command, "cd %s && cd %s", path, tokens[1]);
    retValue = system(command);
    if (retValue == 0) {// command succsed
        if (strcmp(tokens[1], "..") == 0) {
            if (strlen(path) < 4) {
                send550(socket);
            }
            else {
                char *lastSlash = strrchr(path, '\\');
                lastSlash[0] = '\0';
                send250(socket);
            }
        }
        else {
            sprintf(path, "%s\\%s", path, tokens[1]);
            send250(socket);
        }
    }
    else {
        send550(socket);
    }
}

void handleRETR(SOCKET socket, char tokens[NUM_WORDS][NUM_CHARS], struct sockaddr_in dataAddress, char *path) {
    FILE *fp;
    int fileSize;
    char *data;
    char filePath[100];

    if (dataAddress.sin_port == 0) {
        return;
    }

    if (tokens[1][strlen(tokens[1]) -2] == '\r')
    {
        tokens[1][strlen(tokens[1]) -2] = '\0';
    }

    sprintf(filePath, "%s\\%s", path, tokens[1]);

    fp = fopen(filePath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open input file\n");
        send550(socket);
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
    sendRecvDataViaNewConnection(dataAddress, data, 0);
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

void handleLIST(SOCKET socket, char tokens[NUM_WORDS][NUM_CHARS], struct sockaddr_in dataAddress, char *path) {
    char command[MAX_COMMAND];

    if (strlen(tokens[1]) > 2 && tokens[1][strlen(tokens[1]) - 2] == '\r') {
        tokens[1][strlen(tokens[1]) - 2] = '\0';
    }

    sprintf(command, "cd %s && dir %s> output.txt", path, tokens[1]);
    system(command);

    FILE *fileptr1, *fileptr2;
    char filePath[MAX_PATH];
    int ch;
    int delete_line = 1;
    int temp = 1;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    sprintf(filePath, "%s//output.txt", path);
    //open file in read mode
    fileptr1 = fopen(filePath, "r");

    while ((read = getline(&line, &len, fileptr1)) != -1) {
        if(strstr(line, "output.txt") != NULL) {
            break;
        }
        delete_line++;
    }

    rewind(fileptr1);

    //open new file in write mode
    sprintf(filePath, "%s//replica.txt", path);
    fileptr2 = fopen(filePath, "w");
    ch = 0;
    while ((ch = getc(fileptr1)) != EOF)
    {
        //except the line to be deleted
        if (temp != delete_line)
        {
            //copy all lines in file replica.c
            putc(ch, fileptr2);
        }
        if (ch == '\n')
        {
            temp++;
        }
    }
    fclose(fileptr1);
    fclose(fileptr2);
    sprintf(filePath, "%s//output.txt", path);
    remove(filePath);


    strcpy(tokens[1], "replica.txt");

    handleRETR(socket, tokens, dataAddress, path);

    sprintf(command, "del \"%s\\replica.txt\"", path);
    system(command);
}

void handleNLST(SOCKET socket, char tokens[NUM_WORDS][NUM_CHARS], struct sockaddr_in dataAddress, char *path) {
    sprintf(tokens[1], "/b %s", tokens[1]);
    handleLIST(socket, tokens, dataAddress, path);
}

void send220(SOCKET socket) {
    int code = 220;
    char message[100];
    sprintf(message, "%d Service ready for new user. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}

void send502(SOCKET socket) {
    int code = 502;
    char message[100];
    sprintf(message, "%d Command not implemented. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}

void send250(SOCKET socket) {
    int code = 250;
    char message[100];
    sprintf(message, "%d Requested action okay, completed. Aviv is the best\r\n", code);
    send(socket , message , (int)strlen(message), 0);
}

void send550(SOCKET socket) {
    int code = 550;
    char message[100];
    sprintf(message, "%d Requested action not taken. File unavailable (e.g., file not found, no access).\r\n", code);
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

int sendRecvDataViaNewConnection(struct sockaddr_in address, char *data, boolean toRecv)
{
    SOCKET s;
    int recvSize;

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
    if (toRecv == TRUE) {
        if (recv(s, data, sizeof(data)/ sizeof(char), 0) < 0) {
            puts("Send failed");
            return 1;
        }
    }
    else {
        //Send some data
        if ((recvSize = send(s, data, strlen(data), 0)) <= 0) {
            puts("recv failed");
            return 1;
        }
        printf("data sent\n");
    }

    closesocket(s);
    printf("finished sendRecvDataViaNewConnection\n");
    return 0;
}