#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>

// g++ client_m.cpp -o client.exe -lws2_32

#pragma comment(lib, "ws2_32.lib") //Сетевая библиотека

const int BUFFER_SIZE = 1024;
std::atomic_bool clientRunning(true);
int clientId = -1;

void ReceiveMessages(SOCKET sock) {
    char buffer[BUFFER_SIZE]; // Буфер для приема данных
    while (clientRunning) {  // Бесконечный цикл пока работает клиент
        int bytesReceived = recv(sock, //Дескриптор, идентифицирующий подключенный сокет.
            buffer,//Указатель на буфер для получения входящих данных.
            BUFFER_SIZE,//Длина (в байтах) буфера, на который указывает параметр buf .
            0);  // Прием данных от сервера
        if (bytesReceived <= 0) {
            std::cout << "Connection closed by server." << std::endl;
            clientRunning = false;
            break;
        }
        // Обработка сообщения
        std::string msg(buffer, bytesReceived);
        if (msg.find("YourID:") == 0) {// Если сервер прислал ID
            clientId = std::stoi(msg.substr(7));// Извлечение ID
            std::cout << "Your ID: " << clientId << std::endl;
        } else {
            std::cout << msg << std::endl;// Обычное сообщение
        }

    }
}

int main() {
    WSADATA wsaData; // Инициализация Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Создание сокета
    SOCKET sock = socket(AF_INET, //Семейство адресов IPv4.
        SOCK_STREAM, 0);// TCP-сокет

    // Настройка адреса сервера
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
    // Запуск потока для приема сообщений
    std::thread receiver(ReceiveMessages, sock);
    receiver.detach();// Отделение потока от основного

    // Основной цикл отправки сообщений
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

    shutdown(sock,
        SD_BOTH);//Завершите операции отправки и получения.
    closesocket(sock);
    WSACleanup();
    return 0;
}