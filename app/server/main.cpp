#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#define PORT 8080

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    listen(server_fd, 3);
    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        std::cout << "Client connected!" << std::endl;

        std::thread([client_socket]() {
            char buffer[1024] = {0};

            while (true) {
                int valread = read(client_socket, buffer, 1024);
                if (valread <= 0) break;

                std::cout << "Client: " << buffer << std::endl;

                std::string reply = "Server received: ";
                reply += buffer;
                send(client_socket, reply.c_str(), reply.size(), 0);

                memset(buffer, 0, sizeof(buffer));
            }

            close(client_socket);
            std::cout << "Client disconnected!" << std::endl;
        }).detach();
    }

    close(server_fd);
}