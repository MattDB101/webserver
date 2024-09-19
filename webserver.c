#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Link the Winsock library
#define PORT 8080

int main() {

    // Initialize Winsock
    WSADATA wsaData; 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }


    // Create an unbound (addressless) socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0); //AF_INET = IPv4, SOCK_STREAM = TCP
    if (server_socket == -1) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }


    // Enable recent address re-use
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) { // Cast with (const char*) is needed because setsockopt() expects a pointer to a value in the form of a char array.
        printf("SO_REUSEADDR failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }


    // Initialise serv_addr
    struct sockaddr_in serv_addr = { .sin_family = AF_INET , // IPv4 address
                                     .sin_port = htons(PORT), // htons() converts int to big-endian.
                                     .sin_addr = { htonl(INADDR_ANY) }, // INADDR_ANY is a special constant that tells the system to bind the socket to all available network interfaces.
                                    };          // htonl() converts the IP address into big-endian.
    
    
    // Bind the socket to an address and chacek for failure
    if (bind(server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }


    // Listen for up to 3 concurrent connections
    int connection_backlog = 3;
    if (listen(server_socket, connection_backlog) != 0) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Waiting for client to connect\n");


    // Accept incoming connections and create a socket to communicate back to that invidiual client
    struct sockaddr_in client_addr;
    int client_addr_len;
    client_addr_len = sizeof(client_addr);
    // accept() is a system call that extracts the first connection request from the queue of pending connections for server_socket and creates a new socket specifically for that connection.
    // accept() is a blocking call
    SOCKET client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

    if (client_socket == -1) {
        printf("Accept failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Client connected\n");


    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
