#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

void testTimeout(const char* host, int port, int delay)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        return;
    }
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock);
        return;
    }
    
    std::cout << "Connected to server. Sending slow request..." << std::endl;
    
    const char* line1 = "POST /test HTTP/1.1\r\n";
    send(sock, line1, strlen(line1), 0);
    const char* line2 = "Host: localhost\r\n";
    send(sock, line2, strlen(line2), 0);
    std::cout << "Sent: Host header" << std::endl;
    const char* line3 = "Content-Length: 10\r\n\r\n";
    send(sock, line3, strlen(line3), 0);
    std::cout << "Sent: Content-Length" << std::endl;
    sleep(delay);
    
    // Send body
    const char* body = "1234567890";
    send(sock, body, strlen(body), 0);
    std::cout << "Sent: Body" << std::endl;
    
    // Try to receive response
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0)
    {
        std::cout << "\nResponse received:\n" << buffer << std::endl;
    }
    else if (bytes_received == 0)
    {
        std::cout << "\n✅ Connection closed by server (timeout worked!)" << std::endl;
    }
    else
    {
        perror("recv");
    }
    
    close(sock);
}

int main(int argc, char* argv[])
{
    const char* host = "127.0.0.1";
    int port = 8080;
    int delay = 10;
    
    if (argc > 1)
        delay = atoi(argv[1]);
    if (argc > 2)
        port = atoi(argv[2]);
    
    std::cout << "Testing timeout with " << delay << " second delays..." << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "(Each part will be sent with " << delay << "s delay)\n" << std::endl;
    
    testTimeout(host, port, delay);
    
    return 0;
}