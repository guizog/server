#include "server.h"
#include "job_queue.h"
#include "clogger/logger.h"

char WORKING_DIR[1024];
char CONTENT_DIR[1024];

int initializeDirectory() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char cwd_copy[1024];
        strcpy(cwd_copy, cwd);

        char *dir = dirname(cwd_copy);
        strcpy(WORKING_DIR, dir);
        snprintf(CONTENT_DIR, sizeof(CONTENT_DIR), "%s\\wwwcontent", WORKING_DIR);

        writeLog(LOG_INFO, "Root path: %s\nContent path: %s\n", WORKING_DIR, CONTENT_DIR);
        return 0;
    }
    writeLog(LOG_ERROR, "Error while initializing server directories");
    return 1;
}

char *getFile(const http_request_line *req, size_t *outSize, char **outType) {
    if (!req || !req->pathUri)
        return NULL;

    char tempPath[PATH_MAX];
    char resolvedPath[PATH_MAX];

    snprintf(tempPath, sizeof(tempPath), "%s%s", CONTENT_DIR, req->pathUri);

    if (!_fullpath(resolvedPath, tempPath,  PATH_MAX)) {
        writeLog(LOG_ERROR, "fullpath failed to process the generated file path");
        return NULL;
    }

    char rootResolved[PATH_MAX];
    if (!_fullpath(rootResolved, CONTENT_DIR, PATH_MAX)){
        return NULL;
    }

    size_t rootLength = strlen(rootResolved);

    if (strncmp(resolvedPath, rootResolved, rootLength) != 0 ||
        (resolvedPath[rootLength] != '/' &&
         resolvedPath[rootLength] != '\\' &&
         resolvedPath[rootLength] != '\0')) {
        fprintf(stderr, "Directory traversal attempt blocked");
        return NULL;
     }

    FILE * filePtr = fopen(resolvedPath, "rb");
    if (!filePtr) {
        writeLog(LOG_ERROR, "Failed to open file");
        return NULL;
    }

    if (fseek(filePtr, 0, SEEK_END) != 0) {
        fclose(filePtr);
        return NULL;
    }

    long fileSize = ftell(filePtr);
    if (fileSize < 0) {
        fclose(filePtr);
        return NULL;
    }

    rewind(filePtr);

    char *buffer = malloc(fileSize);
    if (!buffer) {
        writeLog(LOG_ERROR, "Failed to allocate buffer for file");
        return NULL;
    }

    size_t readSize = fread(buffer, 1, fileSize, filePtr);
    fclose(filePtr);

    if (readSize != (size_t)fileSize) {
        free(buffer);
        return NULL;
    }

    char *extensionDot = strrchr(resolvedPath, '.');
    if (!extensionDot || extensionDot == resolvedPath) {
        *outType = "bin";
    }
    else {
        *outType = extensionDot + 1;
    }
    *outSize = fileSize;
    return buffer;
}

int startServer() {
    initializeDirectory();
    startLogger(WORKING_DIR);


    struct addrinfo *addr_result = NULL, *ptr = NULL, hints;
    int tcpSocket;
    WSADATA wsaData;
    int enabled = TRUE;

    // Initialize Winsock
    tcpSocket = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (tcpSocket != 0) {
        writeLog(LOG_ERROR, "WSAStartup failed with error: %d\n", tcpSocket);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    tcpSocket = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &addr_result);
    if (tcpSocket != 0) {
        writeLog(LOG_ERROR, "getaddrinfo failed: %d", tcpSocket);
        WSACleanup();
        return 1;
    }

    //creates the socket to listen for the client connection
    SOCKET listenSocket = INVALID_SOCKET;
    listenSocket = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        writeLog(LOG_ERROR, "error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }
    writeLog(LOG_INFO,"socket created");
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enabled, sizeof(enabled));

    tcpSocket = bind(listenSocket, addr_result->ai_addr, (int) addr_result->ai_addrlen);
    if (tcpSocket == SOCKET_ERROR) {
        writeLog(LOG_ERROR, "bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }

    writeLog(LOG_INFO, "socket bind");

    //result variable is no longer needed after the binding, so memory is freed
    freeaddrinfo(addr_result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        writeLog(LOG_ERROR, "Listen failed with error: %ld", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    writeLog(LOG_INFO, "socket listening for connections");

    queue_init(&queue);

    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    while (1) {
        SOCKET clientSocket = INVALID_SOCKET;
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            writeLog(LOG_ERROR, "accept failed with error: %d", WSAGetLastError());
            continue;
        }

        writeLog(LOG_INFO, "connection accepted");
        enqueue(&queue, clientSocket);
    }
}

void *worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue.mutex);

        while (queue.head == NULL) {
            pthread_cond_wait(&queue.cond, &queue.mutex);
        }

        job_t *job = queue.head;
        queue.head = job->next;
        if (!queue.head) queue.tail = NULL;
        queue.size--;

        pthread_mutex_unlock(&queue.mutex);

        SOCKET client = job->client_fd;
        free(job);

        handleClient(client);
        closesocket(client);
    }
}

int isRequestComplete(char *buffer, size_t len) {
    if (len < 4)
        return 0;

    char *headerEnd = strstr(buffer, "\r\n\r\n");
    if (headerEnd == NULL)
        return 0;

    size_t headerSize = (size_t) (headerEnd - buffer) + 4;

    char saved = *headerEnd;
    *headerEnd = '\0';
    char *cl = strstr(buffer, "Content-Length:");
    *headerEnd = saved;

    if (cl == NULL)
        return 1;

    unsigned long contentLength = strtoul(cl + strlen("Content-Length:"), NULL, 10);
    size_t bodyReceived = len - headerSize;

    return bodyReceived >= contentLength;
}

static int sendAll(SOCKET sock, const char *data, size_t len) {
    size_t sent = 0;

    while (sent < len) {
        int bytes = send(sock, data + sent, (int) (len - sent), 0);
        if (bytes <= 0)
            return -1;
        sent += (size_t) bytes;
    }

    return 0;
}

int handleClient(int client_socket) {
    size_t capacity = 4096;
    size_t length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) {
        writeLog(LOG_ERROR, "Failed to allocate request buffer");
        return -1;
    }

    while (1) {
        if (length + 1 >= capacity) {
            if (capacity >= MAX_REQUEST_SIZE) {
                writeLog(LOG_ERROR, "Request exceeds maximum size");
                free(buffer);
                return -1;
            }

            size_t newCapacity = capacity * 2;
            if (newCapacity > MAX_REQUEST_SIZE)
                newCapacity = MAX_REQUEST_SIZE;

            char *resized = realloc(buffer, newCapacity);
            if (!resized) {
                writeLog(LOG_ERROR, "Failed to grow request buffer");
                free(buffer);
                return -1;
            }
            buffer = resized;
            capacity = newCapacity;
        }

        int bytes = recv(client_socket, buffer + length, (int) (capacity - length - 1), 0);
        if (bytes < 0) {
            writeLog(LOG_ERROR, "recv(client)");
            free(buffer);
            return -1;
        }
        if (bytes == 0) {
            writeLog(LOG_INFO, "connection closed gracefully");
            break;
        }

        length += (size_t) bytes;
        buffer[length] = '\0';

        if (isRequestComplete(buffer, length))
            break;
    }

    if (length == 0) {
        free(buffer);
        return 0;
    }

    writeLog(LOG_DEBUG, "REQUEST=> %s", buffer);

    http_request_line *parsedReq = parseRequestLine(buffer, length, CRLF);
    http_response_line *response = calloc(1, sizeof(http_response_line));
    if (!response) {
        freeRequestLine(parsedReq);
        free(buffer);
        return -1;
    }

    response->version = "HTTP/1.0";

    size_t fileLength = 0;
    char *fileType = NULL;
    char *fileBuffer = NULL;

    if (!parsedReq) {
        writeLog(LOG_ERROR, "Failed to parse request");
        setStatusCode(response, 400);
        buildResponsePayload(response, "", 0, "html", NULL, 0);
    }
    else {
        setStatusCode(response, 200);
        fileBuffer = getFile(parsedReq, &fileLength, &fileType);
        if (!fileBuffer) {
            writeLog(LOG_ERROR, "getPathFile failed");
            setStatusCode(response, 404);
            buildResponsePayload(response, "", 0, "html", NULL, 0);
        }
        else {
            writeLog(LOG_DEBUG, "file type being read: %s", fileType);
            buildResponsePayload(response, fileBuffer, fileLength, fileType, NULL, 0);
        }
    }

    if (response->payloadBody && response->payloadLength > 0) {
        if (sendAll((SOCKET) client_socket, response->payloadBody, response->payloadLength) != 0) {
            writeLog(LOG_ERROR, "send(client) failed");
        }
    }

    free(fileBuffer);
    freeRequestLine(parsedReq);
    freeResponseLine(response);
    free(response);
    free(buffer);

    return 0;
}
