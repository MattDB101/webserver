#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Link the Winsock library
#define PORT 8080
#define MAXLEN 1024

struct http {
    const char* status;
    const char* headers;
    const char* body;
};


int respond(int client_socket, struct http *response) {
    char full_response[MAXLEN];
    snprintf(full_response, sizeof(full_response), "%s\r\n%s\r\n%s", response->status, response->headers, response->body);
    int send_response = send(client_socket, full_response, strlen(full_response), 0);
    return send_response;
}


int main() {
    // Initialize Winsock
    WSADATA wsaData; 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create an unbound (addressless) socket
    SOCKET server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0); //AF_INET = IPv4, SOCK_STREAM = TCP
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Enable recent address re-use
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        printf("SO_REUSEADDR failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Initialise serv_addr
    struct sockaddr_in serv_addr = { .sin_family = AF_INET, // IPv4 address
                                     .sin_port = htons(PORT), // htons() converts int to big-endian.
                                     .sin_addr = { htonl(INADDR_ANY) }, // INADDR_ANY is a special constant that tells the system to bind the socket to all available network interfaces.
                                    }; // htonl() converts the IP address into big-endian.
    
    // Bind the socket to an address and check for failure
    if (bind(server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for up to 3 concurrent connections 
    if (listen(server_socket, 3) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Waiting for client to connect\n");

    // listen forever for new connections (or atleast until the server falls over!)
    while (1) { 
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }
        printf("Client connected\n");

        // Create an empty buffer and read from the client socket into it
        char recv_buf[MAXLEN] = {0};
        int recv_buf_len = sizeof(recv_buf);
        int recv_res;

        recv_res = recv(client_socket, recv_buf, recv_buf_len -1, 0);

        if (recv_res <= 0) {
            printf("Receive failed: %d\n", WSAGetLastError());
            closesocket(client_socket);
            continue;
        }

        recv_buf[recv_res] = '\0';

        struct http response;
        char* request = strtok(recv_buf, "\r\n"); // Slice string to first carriage return
        char* method = strtok(request, " ");
        printf("Method: %s\n", method);

        if (strcmp(method, "GET") != 0) {
            printf("Tried to use method other than GET\n");
            response.status = "HTTP/1.1 405 Method Not Allowed";
            response.headers = "Content-Type: text/plain\r\nConnection: close\r\n";
            response.body = "The requested method is not allowed.\n";

            if (respond(client_socket, &response) == -1) {
                printf("Send failed: %d\n", WSAGetLastError());
            }
            closesocket(client_socket);
            continue;
        }

        char* URL = strtok(NULL, " "); // strtok passed NULL will continue tokenizing the previously passed string
        printf("URL: %s\n", URL);

        if (strcmp(URL, "/") == 0) {
            URL = "index";
        }
       
        size_t file_name_len = strlen(URL) + 6;
        char *file_name = malloc(file_name_len);
        snprintf(file_name, file_name_len, "%s.html", URL);

        FILE *fptr = fopen(file_name, "r");
        free(file_name);
        
        if (fptr != NULL) {
            char *file_content = (char *) malloc(MAXLEN - 1); // create a buffer to store file contents
            fread(file_content, 1, MAXLEN - 1, fptr);
            file_content[MAXLEN] = '\0'; // Null-terminate the end of the buffer
            printf("File content:\n%s\n", file_content);

            response.status = "HTTP/1.1 200 OK";
            response.headers = "Content-Type: text/plain\r\nConnection: close\r\n";
            response.body = file_content;
            
            if (respond(client_socket, &response) == -1) {
                printf("Send failed: %d\n", WSAGetLastError());
            }

            free(file_content);

        } else {
            printf("Not able to open the file.");
            response.status = "HTTP/1.1 404 Not Found";
            response.headers = "Content-Type: text/plain\r\nConnection: close\r\n";
            response.body = "The requested content could not be found.\n";
            if (respond(client_socket, &response) == -1) {
                printf("Send failed: %d\n", WSAGetLastError());
            }
        }
        
        fclose(fptr); 
        closesocket(client_socket);
    }

    // Clean up
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
