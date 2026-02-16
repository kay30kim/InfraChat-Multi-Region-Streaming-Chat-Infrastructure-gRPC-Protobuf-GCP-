#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. socket 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 2. bind
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    // 3. listen
    listen(server_fd, 3);
    std::cout << "Server listening on port " << PORT << std::endl;

    // 4. accept
    client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    std::cout << "Client connected!" << std::endl;

    char buffer[1024] = {0};

    while (true) {
        // 5. recv
        int valread = read(client_socket, buffer, 1024);
        if (valread <= 0) break;

        std::cout << "Client: " << buffer << std::endl;

        // 6. send reply
        std::string reply = "Server received: ";
        reply += buffer;
        send(client_socket, reply.c_str(), reply.size(), 0);

        memset(buffer, 0, sizeof(buffer));
    }

    close(client_socket);
    close(server_fd);
}
