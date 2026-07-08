#include "server.h"
#include "job_queue.h"

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

        printf("Root path: %s\nContent path: %s\n", WORKING_DIR, CONTENT_DIR);
        return 0;
    }
    perror("Error while initializing server directories");
    return 1;
}

char *getFile(const http_request_line *req, size_t *outSize) {
    if (!req || !req->pathUri)
        return NULL;


    char tempPath[PATH_MAX];
    char resolvedPath[PATH_MAX];

    snprintf(tempPath, sizeof(tempPath), "%s%s", CONTENT_DIR, req->pathUri);

    if (!_fullpath(resolvedPath, tempPath,  PATH_MAX)) {
        perror("fullpath failed to process the generated file path");
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
        perror("Failed to open file");
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
        perror("Failed to allocate buffer for file");
        return NULL;
    }

    size_t readSize = fread(buffer, 1, fileSize, filePtr);
    fclose(filePtr);

    if (readSize != (size_t)fileSize) {
        free(buffer);
        return NULL;
    }

    *outSize = fileSize;
    return buffer;
}

int startServer() {
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

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    printf("socket listening for connections\n");
    initializeDirectory();

    queue_init(&queue);

    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

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

        //handleClient(clientSocket);
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
    char *bodyStart = strstr(buffer, CRLF);
    if (bodyStart == NULL) return 0;

    char *cl = strstr(buffer, "Content-Length:");
    if (cl == NULL) return 1; // no body present

    int contentLength = atoi(cl + strlen("Content-Length:"));

    size_t headerSize = bodyStart - buffer;
    size_t bodyReceived = len - headerSize;

    return bodyReceived >= (size_t)contentLength;
}

int handleClient(int client_socket) {
    ssize_t socketRecv = 0;
    size_t capacity = 4096;
    size_t length = 0;
    char* buffer = malloc(capacity);
    int receiveComplete = 0;
    memset(buffer, 0, sizeof(buffer));


    while (1) {
        if (length >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }

        int bytes = recv(client_socket, buffer + length, capacity - length, 0);
        if (bytes < 0) {
            perror("recv(client)");
            free(buffer);
            return -1;
        }
        if (bytes == 0) {
            printf("connection closed gracefully\n");
            break;
        }

        length += bytes;

        if (isRequestComplete(buffer, length))
            break;
    }

    printf("REQUEST=> \n%s\n", buffer);
    http_request_line *parsedReq = parseRequestLine(buffer, socketRecv, CRLF);
    http_response_line *response = malloc(sizeof(http_response_line));

    char* httpHeader = "HTTP/1.0";

    response->version = httpHeader;
    setStatusCode(response, 200);

    size_t  fileLength = 0;
    char* fileBuffer = getFile(parsedReq, &fileLength);
    if (!fileBuffer) {
        printf("getPathFile failed\n");

        setStatusCode(response, 404);
        buildResponsePayload(response, "", 0, NULL, NULL, 0);
    }
    else {
        buildResponsePayload(response, fileBuffer, fileLength, "text/html", NULL, 0);
    }

    (void) send(client_socket, response->payloadBody, response->payloadLength, 0);
    closesocket(client_socket);

    if (fileBuffer)
        free(fileBuffer);

    freeRequestLine(parsedReq);
    free(response);

    // ReSharper disable once CppDFAMemoryLeak
    return 0;
}
