#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>


#define MAX_CLIENTS 10
#define MAX_ROOMS 5
#define BUFFER_SIZE 2048
#define ROOM_NAME_LEN 50

struct client {
    SOCKET socket;
    char username[50];
    char room[ROOM_NAME_LEN];
    int active;
};

struct room {
    char name[ROOM_NAME_LEN];
    int active;
};

struct client clients[MAX_CLIENTS];
struct room rooms[MAX_ROOMS] = {
    {"General", 1},
    {"Gaming", 1},
    {"Tech", 1}
};
int client_count = 0;

void format_and_send(SOCKET sock, const char* prefix, const char* message) {
    char formatted_msg[BUFFER_SIZE];
    snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %s\n", prefix, message);
    send(sock, formatted_msg, strlen(formatted_msg), 0);
}

void room_broadcast(const char *message, const char *room, int sender_index) {
    char formatted_msg[BUFFER_SIZE];
    snprintf(formatted_msg, sizeof(formatted_msg), "[Room: %s] %s: %s", room, clients[sender_index].username, message);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && i != sender_index && strcmp(clients[i].room, room) == 0) {
            send(clients[i].socket, formatted_msg, strlen(formatted_msg), 0);
        }
    }
}

void send_room_list(int client_index) {
    char room_list[BUFFER_SIZE] = "Available rooms:\n";
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active) {
            strcat(room_list, "- ");
            strcat(room_list, rooms[i].name);
            strcat(room_list, "\n");
        }
    }
    format_and_send(clients[client_index].socket, "System", room_list);
}

void join_room(int client_index, const char *room_name) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && strcmp(rooms[i].name, room_name) == 0) {
            char notification[BUFFER_SIZE];
            snprintf(notification, sizeof(notification), "%s has left the room %s", clients[client_index].username, clients[client_index].room);
            room_broadcast(notification, clients[client_index].room, client_index);

            strcpy(clients[client_index].room, room_name);
            snprintf(notification, sizeof(notification), "%s has joined the room %s", clients[client_index].username, room_name);
            room_broadcast(notification, room_name, client_index);
            format_and_send(clients[client_index].socket, "System", "Room joined successfully");
            return;
        }
    }
    format_and_send(clients[client_index].socket, "Error", "Room not found");
}

void private_message(const char *username, const char *message, int sender_index) {
    char formatted_msg[BUFFER_SIZE];
    snprintf(formatted_msg, sizeof(formatted_msg), "[Private] %s: %s", clients[sender_index].username, message);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            send(clients[i].socket, formatted_msg, strlen(formatted_msg), 0);
            snprintf(formatted_msg, sizeof(formatted_msg), "[Private to %s] %s", username, message);
            send(clients[sender_index].socket, formatted_msg, strlen(formatted_msg), 0);
            return;
        }
    }
    snprintf(formatted_msg, sizeof(formatted_msg), "User %s not found", username);
    format_and_send(clients[sender_index].socket, "Error", formatted_msg);
}

void handle_client(int index) {
    char buffer[BUFFER_SIZE];
    char notification[BUFFER_SIZE];

    strcpy(clients[index].room, "General");
    snprintf(notification, sizeof(notification), "%s has joined room General", clients[index].username);
    room_broadcast(notification, "General", index);
    send_room_list(index);

    while (1) {
        int bytes_received = recv(clients[index].socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            snprintf(notification, sizeof(notification), "%s has left the chat", clients[index].username);
            printf("%s\n", notification);
            room_broadcast(notification, clients[index].room, index);
            clients[index].active = 0;
            closesocket(clients[index].socket);
            break;
        }

        buffer[bytes_received] = '\0';
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "/rooms") == 0) {
            send_room_list(index);
        } else if (strncmp(buffer, "/join ", 6) == 0) {
            join_room(index, buffer + 6);
        } else if (buffer[0] == '@') {
            char *username = strtok(buffer + 1, " ");
            char *msg = strtok(NULL, "");
            if (username && msg) {
                private_message(username, msg, index);
            }
        } else {
            printf("[Room: %s] %s: %s\n", clients[index].room, clients[index].username, buffer);
            room_broadcast(buffer, clients[index].room, index);
        }
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    int c;

    printf("Initializing Chat Server...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSA Initialization failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    listen(server_socket, 3);
    printf("Server started. Waiting for clients...\n");

    c = sizeof(struct sockaddr_in);
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, &c)) != INVALID_SOCKET) {
        char username[50];
        recv(client_socket, username, 50, 0);
        username[strcspn(username, "\n")] = '\0';

        int index = client_count++;
        clients[index].socket = client_socket;
        strcpy(clients[index].username, username);
        clients[index].active = 1;

        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_client, (LPVOID)index, 0, NULL);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
