#include <stdio.h>
#include <stdbool.h>

//#include "winsock.h"

//extern int winsock(void);

#define _WIN32_WINNT 0x501

#include <ws2tcpip.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define CRLF "\r\n"
#define SP " "
#define ROOT_PATH "C:\\Users\\guizo\\CLionProjects\\server\\content"
#define STATUS404 "Not found"


struct http_status_code {
    int code;
    char *text;
}typedef http_status_code;

struct http_request_line {
    char *method;
    char *pathUri;
    char *payload;
    char *version;
}typedef http_request_line;

struct http_response_line {
    http_status_code statusCode;
    char *payload;
    char *version;
}typedef http_response_line;

http_status_code status_codes[] = {
    {200, " OK"},
    {400, " Bad Request"},
    {404, " Not Found"}
};


char *str_replace(char *testInput, char *replaceChar, char *replaceWith) {
    char* temp;
    char* insertPoint;
    char* result;
    int replaceCount;
    int lenReplace = strlen(replaceChar);
    int lenWith = strlen(replaceWith);
    int lenFront;

    insertPoint = testInput;
    for (replaceCount = 0; (temp = strstr(insertPoint, "/")); replaceCount++) {
        printf("replace count: %d\n", replaceCount);
        insertPoint = temp + lenReplace;
    }

    int diffCharSize = lenWith - lenReplace;
    temp = result = malloc(strlen(testInput) + 1 + diffCharSize * replaceCount);

    while (replaceCount--) {
        insertPoint = strstr(testInput, replaceChar);
        lenFront = insertPoint - testInput;
        temp = strncpy(temp, testInput, lenFront) + lenFront;
        temp = strcpy(temp, replaceWith) + lenWith;
        testInput += lenFront + lenReplace;
    }
    strcpy(temp, testInput);

    for (int i = 0; result[i] != '\0'; i++) {
        printf("char[%d] = '%c' (ASCII %d)\n", i, result[i], result[i]);
    }

    return result;
}

char *getFilePath(const http_request_line *req) {
    char fileBuffer[1024];
    char buffer[1024] = "";
    char filePath[256] = ROOT_PATH;

    char* fixedPath = str_replace(req->pathUri, "/", "\\");
    strcat(filePath, fixedPath);
    printf("replaced path: %s\n", filePath);

    FILE *filePtr = fopen(filePath, "r");
    if (!filePtr) {
        perror("fopen failed");
        free(fixedPath);
        return NULL;
    }

    while (fgets(fileBuffer, 1024 - strlen(buffer), filePtr)) {
        strcat(buffer, fileBuffer);
    }
    free(fixedPath);

    return strdup(buffer);
}

char *getFile(const http_request_line *req) {
    char filePath[256] = ROOT_PATH;

    char* fixedPath = str_replace(req->pathUri, "/", "\\");
    strcat(filePath, fixedPath);
    printf("replaced path: %s\n", filePath);

    FILE *filePtr = fopen(filePath, "r");
    if (!filePtr) {
        perror("fopen failed");
        free(fixedPath);
        return NULL;
    }

    size_t bufferSize = 4096;
    size_t len = 0;
    char* buffer = malloc(bufferSize);
    if (!buffer) {
        fclose(filePtr);
        free(fixedPath);
        return NULL;
    }
    buffer[0] = '\0';
    char tempBuffer[1024];

    while (fgets(tempBuffer, sizeof(tempBuffer), filePtr)) {
        size_t tempLength = strlen(tempBuffer);
        if (len + tempLength + 1 > bufferSize) {
            bufferSize *= 2;
            char* newBuffer = realloc(buffer, bufferSize);
            if (!newBuffer) {
                free(fixedPath);
                free(buffer);
                fclose(filePtr);
                return NULL;
            }
            buffer = newBuffer;
        }
        strcat(buffer, tempBuffer);
        len += tempLength;
    }

    fclose(filePtr);
    free(fixedPath);
    return buffer;
}

http_request_line *parseRequestLine(const char *buff, const size_t bufSize, char *delimiter) {
    char *copyBuffer = malloc(bufSize);
    if (!copyBuffer) return NULL;

    http_request_line *req = malloc(sizeof(http_request_line));
    if (!req) {
        free(copyBuffer);
        return NULL;
    }

    char *saveptr1;
    char *saveptr2;

    memcpy(copyBuffer, buff, bufSize);
    char *reqToken = strtok_r(copyBuffer, CRLF, &saveptr1);

    req->method = strdup(strtok_r(reqToken, SP, &saveptr2));
    req->pathUri = strdup(strtok_r(NULL, SP, &saveptr2));
    printf("===>Method: %s\n", req->method);
    printf("===>pathUri: %s\n", req->pathUri);

    for (int i = 0; reqToken != NULL; i++) {
        printf("=>token: %s\n", reqToken);
        reqToken = strtok_r(NULL, CRLF, &saveptr1);
    }

    free(copyBuffer);
    return req;
}

void setStatusCode(http_response_line* resp, int statusCode) {
    for (int i = 0; i < sizeof(status_codes)/sizeof(http_status_code); i++) {
        http_status_code curr = status_codes[i];
        if (curr.code == statusCode) {
            resp->statusCode = curr;
            return;
        }
    }
}

int buildResponsePayload(http_response_line* resp, char* fileBuffer) {
    size_t fileLength = strlen(fileBuffer);

    int codeDigits = snprintf(NULL, 0, "%d", resp->statusCode.code);
    size_t headersLength = strlen(resp->version) + codeDigits + strlen(resp->statusCode.text);

    //3 spaces + "\r\n\r\n" (4 chars) + null terminator
    resp->payload = malloc(headersLength + fileLength + 3 + 4 + 1);
    if (!resp->payload) {
        perror("malloc failed");
        return 1;
    }
    sprintf(resp->payload, "%s %d %s\r\n\r\n%s",
            resp->version,
            resp->statusCode.code,
            resp->statusCode.text,
            fileBuffer);

    return 0;
}

int handleClient(int client_socket) {
    ssize_t socketRecv = 0;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        socketRecv = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (socketRecv < 0) {
            perror("recv(client)");
            return -1;
        }
        if (socketRecv == 0) {
            printf("connection closed gracefully\n");
            break;
        }

        printf("REQUEST=> \n%s\n", buffer);
        http_request_line *parsedReq = parseRequestLine(buffer, sizeof(buffer), CRLF);
        http_response_line *response = malloc(sizeof(http_response_line));

        char* httpHeader = "HTTP/1.0";
        const size_t initialLen = strlen(httpHeader);
        char* responseBuffer = malloc(1024);

        response->version = httpHeader;
        setStatusCode(response, 200);

        char* fileBuffer = getFile(parsedReq);
        if (!fileBuffer) {
            printf("getPathFile failed\n");

            setStatusCode(response, 404);
            buildResponsePayload(response, "");
        }
        else {
            buildResponsePayload(response, fileBuffer);
        }

        (void) send(client_socket, response->payload, strlen(response->payload), 0);
        closesocket(client_socket);

        if (fileBuffer)
            free(fileBuffer);

        free(parsedReq->pathUri);
        free(parsedReq->method);
        free(parsedReq);
        free(responseBuffer);
        free(response);
        break;
    }
    printf("####################################################\n");

    // ReSharper disable once CppDFAMemoryLeak
    return 0;
}

int main(void) {
    printf("test");

    struct addrinfo *addr_result = NULL, *ptr = NULL, hints;
    int tcpSocket;
    WSADATA wsaData;
    int enabled = TRUE;

    // Initialize Winsock
    tcpSocket = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (tcpSocket != 0) {
        printf("WSAStartup failed with error: %d\n", tcpSocket);
        return 1;
    }

    (void) setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    tcpSocket = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &addr_result);
    if (tcpSocket != 0) {
        printf("getaddrinfo failed: %f\n");
        WSACleanup();
        return 1;
    }

    //creates the socket to listen for the client connection
    SOCKET listenSocket = INVALID_SOCKET;
    listenSocket = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        printf("error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }
    printf("socket created\n");

    /*
     *      SOCKET BINDING
     */
    tcpSocket = bind(listenSocket, addr_result->ai_addr, (int) addr_result->ai_addrlen);
    if (tcpSocket == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }

    printf("socket bind\n");
    //result variable is no longer needed after the binding, so memory is freed
    freeaddrinfo(addr_result);

    /*
     *      SOCKET LISTENING
     */

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    printf("socket listening for connections\n");

    while (1) {
        SOCKET clientSocket = INVALID_SOCKET;
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        printf("connection accepted \n");

        handleClient(clientSocket);
        // TODO: handle errors from the handle_client function
    }


    //winsock();
    return 0;
}
