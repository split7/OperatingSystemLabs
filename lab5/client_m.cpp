#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 1024;
std::atomic_bool clientRunning(true);
int clientId = -1;

void ReceiveMessages(SOCKET sock) {
    char buffer[BUFFER_SIZE];
    while (clientRunning) {
        int bytesReceived = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cout << "Connection closed by server." << std::endl;
            clientRunning = false;
            break;
        }

        std::string msg(buffer, bytesReceived);
        if (msg.find("YourID:") == 0) {
            clientId = std::stoi(msg.substr(7));
            std::cout << "Your ID: " << clientId << std::endl;
        } else {
            std::cout << msg << std::endl;
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread receiver(ReceiveMessages, sock);
    receiver.detach();

    std::string message;
    while (clientRunning) {
        std::getline(std::cin, message);
        if (!clientRunning) break;
        if (message == "exit") {
            clientRunning = false;
            break;
        }
        send(sock, message.c_str(), message.size() + 1, 0);
    }

    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();
    return 0;
}