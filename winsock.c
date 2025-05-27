#include <stdio.h>
#include <winsock.h>

#define _WIN32_WINNT 0x501

#include <ws2tcpip.h>

#define DEFAULT_PORT "27015"

#define DEFAULT_BUFLEN 512

/*
 *
 *      TESTING WINSOCK LIBRARY
 *
 */

int winsock(void) {
    struct addrinfo *addr_result = NULL, *ptr = NULL, hints;
    int socketResult;
    WSADATA wsaData;

    // Initialize Winsock
    socketResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (socketResult != 0) {
        printf("WSAStartup failed with error: %d\n", socketResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    socketResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &addr_result);
    if (socketResult != 0) {
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

    /*
     *      SOCKET BINDING
     */
    socketResult = bind(listenSocket, addr_result->ai_addr, (int) addr_result->ai_addrlen);
    if (socketResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(addr_result);
        WSACleanup();
        return 1;
    }

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


    /*
     *      ACCEPTING CONNECTIONS
     */
    while (1) {
        printf("waiting for connections... \n");
        SOCKET clientSocket = INVALID_SOCKET;
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        printf("connection accepted \n");
        //listenSocket is not longer needed after the connection is accepted
        closesocket(listenSocket);


        char recvbuf[DEFAULT_BUFLEN];
        int socketSendResult;
        socketResult = NULL;
        int recvbuflen = DEFAULT_BUFLEN;

        do {
            socketResult = recv(clientSocket, recvbuf, recvbuflen, 0);
            if (socketResult > 0) {
                printf("bytes received: %d\n", socketResult);
                printf("data received: %s\n", recvbuf);

                socketSendResult = send(clientSocket, recvbuf, socketResult, 0);
                if (socketSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(clientSocket);
                    WSACleanup();
                    return 1;
                }

                printf("bytes sent: %d\n", socketSendResult);
            } else if (socketResult == 0) {
                printf("connection closing...\n");
            } else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                WSACleanup();
                return 1;
            }
        } while (socketResult > 0);

        /*
         *      CLOSE CONNECTION
         */

        socketResult = shutdown(clientSocket, SD_SEND);
        if (socketResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        closesocket(clientSocket);
        WSACleanup();
    }


    printf("Hello, World!\n");
    return 0;
}
