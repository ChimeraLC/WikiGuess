#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reader.h"
#include "bloom.h"

#define DEFAULT_BUFLEN 512
char *port_num = "27015";

void usage();

int
main(int argc, char **argv)
{
        // Parse flags
        char c;
        while ((c = getopt (argc, argv, "p:")) != -1) {
                switch (c)
                {
                case 'p':       // Start point of debug
                        port_num = optarg;
                        break;
                case '?':
                        if (optopt == 'p')
                        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                        else
                        usage();
                        return -1;
                default:
                        abort ();
                }
        }
        

        // Reading wikipedia page
	if (optind == argc) {         // Checking input arguments
		printf("Missing page title");
		return -1;
	}
        char *wiki_page = argv[optind];
	for (int i = optind + 1; i < argc; i++) {                               // TODO: currently doesn't work if flag at end
		strcat(wiki_page, " ");
		strcat(wiki_page, argv[i]);
	}

        int iRet;       // Used to track error codes

        // Call reader that will internally initialize the bloom filter
        iRet = parse_page(wiki_page);
        if (iRet != 0) {
                // End on error
                printf("Wikipedia page was not able to be gotten\n");
                return 1;
        }

        int iSent = 0;  // Used to track sent bits
	WSADATA wsa;    // Windows socket information
        SOCKET ListenSocket = INVALID_SOCKET;   // Listening socket
        SOCKET ClientSocket = INVALID_SOCKET;   // Client socket
        char recvbuf[DEFAULT_BUFLEN];           // Data buffer
        int recvbuflen = DEFAULT_BUFLEN;        // Data buffer size

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
        iRet = getaddrinfo(NULL, port_num, &hints, &result);
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
        do {

                iRet = recv(ClientSocket, recvbuf, recvbuflen, 0);
                if (iRet > 0) {
                printf("Bytes received: %d\n", iRet);
                printf("Message: %s\n", recvbuf);
                if (test(recvbuf)) {
                        printf("Message contained in filter\n");
                }
                // Echo the buffer back to the sender
                iSent = send( ClientSocket, recvbuf, iRet, 0 );
                if (send( ClientSocket, recvbuf, iRet, 0 ) == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                }
                printf("Bytes sent: %d\n", iSent);

                // Clear the buffer
                memset(recvbuf, 0, strlen(recvbuf));
                }
                else if (iSent == 0)
                        printf("Connection closing...\n");
                else  {
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                }

        } while (iRet > 0);

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


// Usage function, displays inputs
void
usage()
{
    fprintf(stderr, "Usage: main <server address> [-p <port number>]\n");
    fprintf(stderr, "Options\n");\
    fprintf(stderr, "\t-p         Port number (default 27015).\n");
}