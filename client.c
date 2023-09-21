// This


#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_BUFLEN 512
char *port_num = "27015";

void usage();

int main(int argc, char **argv) 
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

        WSADATA wsaData;
        SOCKET ConnectSocket = INVALID_SOCKET;
        struct addrinfo *result = NULL,
                        *ptr = NULL,
                        hints;
        char recvbuf[DEFAULT_BUFLEN];
        int iRet;
        int recvbuflen = DEFAULT_BUFLEN;

        // Ask for username
        char sendbuf[100] = "Connection recieved from: ";
        char name[100];
        fgets(name, 100, stdin); 
        int i = 0;
        // Cut out initial spaces
        while (name[i] == ' ') {
                i++;
        }
        int lead = i;
        // Cut out newline elements
        while (name[i] != '\0' && name[i] != '\n') {
                name[i-lead] = name[i];
                i++;
        }
        // Set end of string
        name[i] = '\0';
        scanf("%s", name);                                                      // TODO: check length
        strcat(sendbuf, name);
        
        // Display message
        printf("Now awaiting inputs\n");
        

        // Reading wikipedia page
	if (optind == argc) {         // Checking input arguments
                printf("usage: %s server-name\n", argv[0]);
		return -1;
	}
        char *server_name = argv[optind];
	
        /*
        // Validate the parameters
        if (argc != 2) {
                printf("usage: %s server-name\n", argv[0]);
                return 1;
        }
        */

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
        iRet = getaddrinfo(server_name, port_num, &hints, &result);
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

        // Temporary setup
        char data[100];
        while(true) {
                printf("Enter data: ");
                fgets(data, 100, stdin); 
                int i = 0;
                // Cut out initial spaces
                while (data[i] == ' ') {
                        i++;
                }
                int lead = i;
                // Cut out newline elements
                while (data[i] != '\0' && data[i] != '\n') {
                        data[i-lead] = data[i];
                        i++;
                }
                // Set end of string
                data[i] = '\0';

                if (strcmp(data, "quit") == 0) {
                        break;
                }

                iRet = send( ConnectSocket, data, (int)strlen(data), 0 );
                if (iRet == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ConnectSocket);
                        WSACleanup();
                        return 1;
                }
        }

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

// Usage function, displays inputs
void
usage()
{
    fprintf(stderr, "Usage: main <Wikipedia Page Title> [-p <port number>]\n");
    fprintf(stderr, "Options\n");\
    fprintf(stderr, "\t-p         Port number (default 27015).\n");
}