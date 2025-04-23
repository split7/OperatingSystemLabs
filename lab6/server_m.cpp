#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <thread>

#pragma comment(lib, "ws2_32.lib")// Сетевая библиотека
#pragma comment(lib, "user32.lib")// GUI элементы
#pragma comment(lib, "gdi32.lib") // Графика
//g++ server_m.cpp -o server.exe -lws2_32 -lgdi32 -luser32 -mwindows

const int MAX_CLIENTS = 3;
const int BUFFER_SIZE = 1024;
const COLORREF BG_COLOR = RGB(40, 40, 60); // Цвет фона
const COLORREF TEXT_COLOR = RGB(200, 200, 220);// Цвет текста

HWND hStartBtn, hExitBtn, hLogList, hStatusLabel;// Элементы GUI
std::atomic_bool serverRunning(false);
SOCKET serverSocket = INVALID_SOCKET;
HANDLE hSemaphore, hMutex;
HBRUSH hBackgroundBrush;

struct ClientInfo {
    SOCKET socket;
    int id;
};
std::vector<ClientInfo> clients;
std::atomic_int clientCounter(0);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ServerThread();
void AddLogMessage(const std::string& msg);
DWORD WINAPI ClientHandler(LPVOID lpParam);
void ShutdownServer();
void InitControls(HWND hWnd);
void UpdateStatus();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // Регистрация класса окна
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ServerClass";
    wc.hbrBackground = CreateSolidBrush(BG_COLOR);
    RegisterClassW(&wc); //Регистрирует класс окна для последующего использования в вызовах функции CreateWindow

    HWND hWnd = CreateWindowW(L"ServerClass", L"Chat Server", WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600, NULL, NULL, hInstance, NULL);

    InitControls(hWnd); // Инициализация элементов управления
    hBackgroundBrush = CreateSolidBrush(BG_COLOR);

    ShowWindow(hWnd, nShow);
    UpdateWindow(hWnd);

    MSG msg;
    // Цикл обработки сообщений
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(hBackgroundBrush);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORLISTBOX: { // Обработка цветов элементов
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, TEXT_COLOR);
            SetBkColor(hdc, BG_COLOR);
            return (LRESULT)hBackgroundBrush;
        }

        case WM_COMMAND: {// Обработка кнопок Start/Exit
            switch (LOWORD(wParam)) {
                case 1:
                    if (!serverRunning) {
                        serverRunning = true;
                        std::thread(ServerThread).detach();
                        EnableWindow(hStartBtn, FALSE);
                    }
                    break;
                case 2:
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
        }

        case WM_CLOSE:
            ShutdownServer();
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_USER:// Добавление сообщений в ListBox
            SendMessageW(hLogList, LB_ADDSTRING, 0, lParam);
            delete[] (wchar_t*)lParam;
            break;

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void InitControls(HWND hWnd) {
    hStartBtn = CreateWindowW(L"BUTTON", L"Start Server", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 20, 150, 40, hWnd, (HMENU)1, NULL, NULL);

    hExitBtn = CreateWindowW(L"BUTTON", L"Exit", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        190, 20, 100, 40, hWnd, (HMENU)2, NULL, NULL);

    hStatusLabel = CreateWindowW(L"STATIC", L"Server Status: Stopped", WS_VISIBLE | WS_CHILD,
        20, 80, 300, 25, hWnd, NULL, NULL, NULL);

    hLogList = CreateWindowW(L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        20, 120, 740, 440, hWnd, (HMENU)3, NULL, NULL);
}

void UpdateStatus() {
    wchar_t status[50];
    swprintf_s(status, L"Server Status: %s (%d/%d)",
        serverRunning ? L"Running" : L"Stopped",
        clients.size(),
        MAX_CLIENTS
    );
    SetWindowTextW(hStatusLabel, status);
}

void ServerThread() {
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        AddLogMessage("WSAStartup failed!");
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        AddLogMessage("Socket creation failed!");
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    // Привязка и прослушивание
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        AddLogMessage("Bind failed!");
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        AddLogMessage("Listen failed!");
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    hSemaphore = CreateSemaphore(NULL, MAX_CLIENTS, MAX_CLIENTS, NULL);
    hMutex = CreateMutex(NULL, FALSE, NULL);

    AddLogMessage("Server started on port 12345");
    UpdateStatus();

    //цикл принятия подключений
    while (serverRunning) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;

        std::string statusMsg = clients.size() >= MAX_CLIENTS ?
            "SERVER_FULL:Maximum clients reached (" + std::to_string(MAX_CLIENTS) + ")" :
            "WELCOME:Connecting...";

        send(clientSocket, statusMsg.c_str(), statusMsg.size() + 1, 0);
        CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket, 0, NULL);
    }

    ShutdownServer();
}

DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    int currentId = 0;

    WaitForSingleObject(hSemaphore, INFINITE);// Ожидание свободного слота

    WaitForSingleObject(hMutex, INFINITE); // Регистрация клиента
    currentId = ++clientCounter;
    clients.push_back({clientSocket, currentId});
    ReleaseMutex(hMutex);

    UpdateStatus();

    std::string idMsg = "YourID:" + std::to_string(currentId);
    send(clientSocket, idMsg.c_str(), idMsg.size() + 1, 0);

    std::string connectMsg = "Client " + std::to_string(currentId) + " connected. Online: " + std::to_string(clients.size());
    AddLogMessage(connectMsg);

    WaitForSingleObject(hMutex, INFINITE);
    for (const auto& client : clients) {
        if (client.socket != clientSocket) {
            if (send(client.socket, connectMsg.c_str(), connectMsg.size() + 1, 0) == SOCKET_ERROR) {
                closesocket(client.socket);
                auto it = std::find_if(clients.begin(), clients.end(),
                    [&client](const ClientInfo& ci) { return ci.socket == client.socket; });
                if (it != clients.end()) {
                    clients.erase(it);
                    ReleaseSemaphore(hSemaphore, 1, NULL);
                }
            }
        }
    }
    ReleaseMutex(hMutex);

    // Цикл приема сообщений
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0 || !serverRunning) break;

        buffer[bytesReceived] = '\0';
        std::string msg = "Client " + std::to_string(currentId) + ": " + buffer;
        AddLogMessage(msg);

        WaitForSingleObject(hMutex, INFINITE);
        for (const auto& client : clients) {
            if (client.socket != clientSocket) {
                if (send(client.socket, msg.c_str(), msg.size() + 1, 0) == SOCKET_ERROR) {
                    closesocket(client.socket);
                    auto it = std::find_if(clients.begin(), clients.end(),
                        [&client](const ClientInfo& ci) { return ci.socket == client.socket; });
                    if (it != clients.end()) {
                        clients.erase(it);
                        ReleaseSemaphore(hSemaphore, 1, NULL);
                    }
                }
            }
        }
        ReleaseMutex(hMutex);
    }

    WaitForSingleObject(hMutex, INFINITE);
    auto it = std::find_if(clients.begin(), clients.end(),
        [clientSocket](const ClientInfo& ci) { return ci.socket == clientSocket; });

    if (it != clients.end()) {
        closesocket(it->socket);
        clients.erase(it);
        ReleaseSemaphore(hSemaphore, 1, NULL);
        UpdateStatus();

        std::string disconnectMsg = "Client " + std::to_string(currentId) + " disconnected. Online: " + std::to_string(clients.size());
        AddLogMessage(disconnectMsg);

        for (const auto& client : clients) {
            if (send(client.socket, disconnectMsg.c_str(), disconnectMsg.size() + 1, 0) == SOCKET_ERROR) {
                closesocket(client.socket);
                auto err_it = std::find_if(clients.begin(), clients.end(),
                    [&client](const ClientInfo& ci) { return ci.socket == client.socket; });
                if (err_it != clients.end()) {
                    clients.erase(err_it);
                    ReleaseSemaphore(hSemaphore, 1, NULL);
                }
            }
        }
    }
    ReleaseMutex(hMutex);

    return 0;
}

void ShutdownServer() {
    serverRunning = false;

    WaitForSingleObject(hMutex, INFINITE);
    for (const auto& client : clients) {
        std::string shutdownMsg = "SERVER_SHUTDOWN";
        send(client.socket, shutdownMsg.c_str(), shutdownMsg.size() + 1, 0);
        closesocket(client.socket);
    }
    clients.clear();
    ReleaseMutex(hMutex);

    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    WSACleanup();
    CloseHandle(hSemaphore);
    CloseHandle(hMutex);
    UpdateStatus();
}

void AddLogMessage(const std::string& msg) {
    wchar_t* buf = new wchar_t[msg.size() + 1];
    size_t converted;
    mbstowcs_s(&converted, buf, msg.size() + 1, msg.c_str(), _TRUNCATE);
    PostMessageW(GetParent(hLogList), WM_USER, 0, (LPARAM)buf);
}