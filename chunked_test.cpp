#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1337);
    inet_pton(AF_INET, "127.1.0.1", &serv_addr.sin_addr);
    
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    // Send chunked request
    std::string request = 
        "POST /test HTTP/1.1\r\n"
        "Host: localhost:1337\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "3\r\n"
        "Web\r\n"
        "0\r\n"
        "\r\n";
    
    // Send in parts to test your buffer
    send(sock, request.substr(0, 50).c_str(), 50, 0);
    sleep(1);
    send(sock, request.substr(50).c_str(), request.length() - 50, 0);
    
    char buffer[1024];
    int bytes = recv(sock, buffer, sizeof(buffer), 0);
    std::cout << "Response: " << std::string(buffer, bytes) << std::endl;
    close(sock);
    
    return 0;
}