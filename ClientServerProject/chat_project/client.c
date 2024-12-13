#include <stdio.h>
#include <winsock2.h>

#define BUFFER_SIZE 2048

DWORD WINAPI receive_messages(void* socket_ptr) {
    SOCKET sock = *(SOCKET*)socket_ptr;
    char buffer[BUFFER_SIZE];
    
    while (1) {
        int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("\nDisconnected from server\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);  // Messages now come formatted from the server
        printf("\nEnter message (/rooms to list, /join roomname to switch): ");
        fflush(stdout);
    }
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char username[50];
    char message[BUFFER_SIZE];
    
    printf("Enter username: ");
    fgets(username, 50, stdin);
    username[strcspn(username, "\n")] = '\0';  // Remove trailing newline

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSA Initialization failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8888);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed\n");
        return 1;
    }

    send(sock, username, strlen(username), 0);
    printf("Connected to chat server!\n");
    printf("Commands:\n/rooms - List available rooms\n/join roomname - Join a room\n@username message - Private message\n");

    CreateThread(NULL, 0, receive_messages, &sock, 0, NULL);

    while (1) {
        printf("\nEnter message (/rooms to list, /join roomname to switch): ");
        fgets(message, BUFFER_SIZE, stdin);

        if (strcmp(message, "quit\n") == 0) {
            break;
        }

        message[strcspn(message, "\n")] = '\0';  // Remove trailing newline
        send(sock, message, strlen(message), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
