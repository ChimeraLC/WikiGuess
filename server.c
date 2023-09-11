#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int
main(int argc, char **argv)
{
        (void) argc;
        (void) argv;
        int iRet;       // Used to track error codes
	WSADATA wsa;    // Windows socket information
        SOCKET ListenSocket = INVALID_SOCKET;   // Listening socket
        SOCKET ClientSocket = INVALID_SOCKET;   // Client socket
	
        // Initializing WSA
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		return 1;
	}

        // Initialize the hints that should be passed to getaddrinfo()
        struct addrinfo *result = NULL;
        struct addrinfo hints;
	
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        
        // Resolve the server address and port
        iRet = getaddrinfo(NULL, DEFAULT_PORT, &hints, &iRet);
        if (iRet != 0 ) {
                printf("getaddrinfo failed with error: %d\n", iRet);
                WSACleanup();
                return 1;
        }

        // Create server's listen socket
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
                printf("socket failed with error: %d\n", WSAGetLastError());
                freeaddrinfo(result);
                WSACleanup();
                return 1;
        }

        // Setup TCP listening socket
        iRet = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iRet == SOCKET_ERROR) {
                printf("bind failed with error: %d\n", WSAGetLastError());
                freeaddrinfo(result);
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
        }

        // Free addr info
        freeaddrinfo(result);

        // Listen
        iRet = listen(ListenSocket, SOMAXCONN);
        if (iRet == SOCKET_ERROR) {
                printf("listen failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
        }

        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
        }
        
        // Close server socket
        closesocket(ListenSocket);

        // MAIN LOOP HERE

        // Cleaning up connections
        iRet = shutdown(ClientSocket, SD_SEND);
        if (iRet == SOCKET_ERROR) {
                printf("shutdown failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
        }

        // cleanup
        closesocket(ClientSocket);
        WSACleanup();

        return 0;
}