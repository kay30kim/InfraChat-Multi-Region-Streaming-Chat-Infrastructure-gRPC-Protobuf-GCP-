#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. socket 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 2. connect
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    std::cout << "Connected to server\n";

    char buffer[1024] = {0};

    while (true) {
        std::string msg;
        std::getline(std::cin, msg);

        // 3. send
        send(sock, msg.c_str(), msg.size(), 0);

        // 4. recv
        int valread = read(sock, buffer, 1024);
        std::cout << buffer << std::endl;

        memset(buffer, 0, sizeof(buffer));
    }

    close(sock);
    
}
