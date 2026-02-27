#include <iostream>
#include <cstring>
#include <unistd.h>     // read(), close()
#include <arpa/inet.h>  // socket structures & inet_pton()

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create socket
    // TCP 소켓 생성 (IPv4 + TCP stream)
    // Returns a file descriptor used for network communication
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // Set server address family (IPv4)
    // 서버 주소 타입을 IPv4로 설정
    serv_addr.sin_family = AF_INET;

    // Convert port number to network byte order
    // 포트 번호를 네트워크 바이트 순서(Big-endian)로 변환
    serv_addr.sin_port = htons(PORT);

    // Convert IP string to binary form
    // 문자열 IP → 바이너리 주소로 변환
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 2. Connect to server
    // 서버와 TCP 연결 시도 (3-way handshake 발생)
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    std::cout << "Connected to server\n";

    char buffer[1024] = {0}; // buffer to store server response
                              // 서버 응답 저장 버퍼

    while (true) {
        std::string msg;

        // Read input from user
        // 사용자 입력 받기
        std::getline(std::cin, msg);

        // 3. Send message to server
        // 입력한 메시지를 서버로 전송 (TCP stream)
        send(sock, msg.c_str(), msg.size(), 0);

        // 4. Receive response from server
        // 서버로부터 데이터 수신 (blocking call)
        int valread = read(sock, buffer, 1024);

        // Print server response
        // 서버 응답 출력
        std::cout << buffer << std::endl;

        // Clear buffer for next message
        // 다음 메시지를 위해 버퍼 초기화
        memset(buffer, 0, sizeof(buffer));
    }

    // Close socket connection
    // 소켓 닫기 (TCP 연결 종료)
    close(sock);
}