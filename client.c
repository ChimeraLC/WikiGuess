
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

int main(int argc, char **argv) 
{
        WSADATA wsaData;
        SOCKET ConnectSocket = INVALID_SOCKET;
        struct addrinfo *result = NULL,
                        *ptr = NULL,
                        hints;
        const char *sendbuf = "this is a test";
        char recvbuf[DEFAULT_BUFLEN];
        int iRet;
        int recvbuflen = DEFAULT_BUFLEN;

        // Validate the parameters
        if (argc != 2) {
                printf("usage: %s server-name\n", argv[0]);
                return 1;
        }

        // Initialize Winsock
        iRet = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iRet != 0) {
                printf("WSAStartup failed with error: %d\n", iRet);
                return 1;
        }

        ZeroMemory( &hints, sizeof(hints) );
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port
        iRet = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
        if ( iRet != 0 ) {
                printf("getaddrinfo failed with error: %d\n", iRet);
                WSACleanup();
                return 1;
        }

        // Attempt to connect to an address until one succeeds
        for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

                // Create a SOCKET for connecting to server
                ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
                        ptr->ai_protocol);
                if (ConnectSocket == INVALID_SOCKET) {
                        printf("socket failed with error: %d\n", WSAGetLastError());
                        WSACleanup();
                        return 1;
                }

                // Connect to server.
                iRet = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iRet == SOCKET_ERROR) {
                        closesocket(ConnectSocket);
                        ConnectSocket = INVALID_SOCKET;
                        continue;
                }
                break;
        }

        freeaddrinfo(result);

        if (ConnectSocket == INVALID_SOCKET) {
                printf("Unable to connect to server!\n");
                WSACleanup();
                return 1;
        }

        // Send an initial buffer
        iRet = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
        if (iRet == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
        }

        printf("Bytes Sent: %d\n", iRet);

        // shutdown the connection since no more data will be sent
        iRet = shutdown(ConnectSocket, SD_SEND);
        if (iRet == SOCKET_ERROR) {
                printf("shutdown failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
        }

        // Receive until the peer closes the connection
        do {
                iRet = recv(ConnectSocket, recvbuf, recvbuflen, 0);
                if ( iRet > 0 )
                        printf("Bytes received: %d\n", iRet);
                else if ( iRet == 0 )
                        printf("Connection closed\n");
                else
                        printf("recv failed with error: %d\n", WSAGetLastError());

        } while( iRet > 0 );

        // cleanup
        closesocket(ConnectSocket);
        WSACleanup();

        return 0;
}