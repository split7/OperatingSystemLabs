#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <atomic>
#include <csignal>

#pragma comment(lib, "ws2_32.lib")
// g++ server_m.cpp -o server.exe -lws2_32


const int MAX_CLIENTS = 3;
const int BUFFER_SIZE = 1024;

std::atomic_bool serverRunning(true);
HANDLE hSemaphore;
HANDLE hMutex;
SOCKET serverSocket;

struct ClientInfo {
    SOCKET socket;
    int id;
};
std::vector<ClientInfo> clients;
std::vector<std::string> chatHistory;
int clientCounter = 0;

void ShutdownServer() {
    serverRunning = false;// Остановка основного цикла сервера
    WaitForSingleObject(hMutex, INFINITE); // Блокировка мьютекса для безопасной работы с клиентами
    for (const auto& client : clients) { // Закрытие всех клиентских сокетов
        shutdown(client.socket, SD_BOTH);
        closesocket(client.socket);
    }
    clients.clear();
    ReleaseMutex(hMutex);// Разблокировка мьютекса
    closesocket(serverSocket);
    WSACleanup();
    CloseHandle(hSemaphore);
    CloseHandle(hMutex);
    exit(0);
}

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        ShutdownServer(); // Корректное завершение при нажатии Ctrl+C
        return TRUE;
    }
    return FALSE;
}

DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET clientSocket = reinterpret_cast<SOCKET>(lpParam);

    // Ожидаем свободный слот
    WaitForSingleObject(hSemaphore, INFINITE);

    // Генерируем уникальный ID
    int clientId;
    WaitForSingleObject(hMutex, INFINITE);
    clientId = ++clientCounter;
    clients.push_back({clientSocket, clientId});
    std::cout << "[SERVER] Client " << clientId << " connected. Online: " << clients.size() << std::endl;
    ReleaseMutex(hMutex);

    // Отправляем клиенту его ID
    std::string idMsg = "YourID:" + std::to_string(clientId);
    send(clientSocket, idMsg.c_str(), idMsg.size() + 1, 0);

    // Уведомляем всех о новом клиенте
    std::string connectMsg = "Client " + std::to_string(clientId) + " joined. Online: " + std::to_string(clients.size());
    WaitForSingleObject(hMutex, INFINITE);
    chatHistory.push_back(connectMsg);
    for (const auto& client : clients) {
        if (client.socket != clientSocket) {
            send(client.socket, connectMsg.c_str(), connectMsg.size() + 1, 0);
        }
    }
    ReleaseMutex(hMutex);

    // Приём сообщений
    char buffer[BUFFER_SIZE];
    while (serverRunning) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0';
        std::string message = "Client " + std::to_string(clientId) + ": " + std::string(buffer);
        std::cout << "[SERVER] " << message << std::endl;

        WaitForSingleObject(hMutex, INFINITE);
        chatHistory.push_back(message);
        for (const auto& client : clients) {
            if (client.socket != clientSocket) {
                send(client.socket, message.c_str(), message.size() + 1, 0);
            }
        }
        ReleaseMutex(hMutex);
    }

    // Отключение клиента
    WaitForSingleObject(hMutex, INFINITE);
    auto it = std::find_if(clients.begin(), clients.end(), [clientSocket](const ClientInfo& ci) {
        return ci.socket == clientSocket;
    });
    if (it != clients.end()) {
        closesocket(it->socket);
        clients.erase(it);
        ReleaseSemaphore(hSemaphore, 1, NULL);
        std::string disconnectMsg = "Client " + std::to_string(clientId) + " left. Online: " + std::to_string(clients.size());
        chatHistory.push_back(disconnectMsg);
        std::cout << "[SERVER] " << disconnectMsg << std::endl;
        for (const auto& client : clients) {
            send(client.socket, disconnectMsg.c_str(), disconnectMsg.size() + 1, 0);
        }
    }
    ReleaseMutex(hMutex);

    return 0;
}

int main() {
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    hSemaphore = CreateSemaphore(NULL, MAX_CLIENTS, MAX_CLIENTS, NULL);
    hMutex = CreateMutex(NULL, FALSE, NULL);

    std::cout << "[SERVER] Server started on port 12345" << std::endl;

    while (serverRunning) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;

        CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket, 0, NULL);
    }

    ShutdownServer();
    return 0;
}