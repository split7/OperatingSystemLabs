#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
#include <atomic>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

//g++ client_m.cpp -o client.exe -lws2_32 -lgdi32 -luser32 -mwindows

const int BUFFER_SIZE = 1024;
const COLORREF BG_COLOR = RGB(30, 30, 45);
const COLORREF TEXT_COLOR = RGB(220, 220, 240);
const COLORREF INPUT_BG_COLOR = RGB(50, 50, 70);
const COLORREF BUTTON_BG_COLOR = RGB(70, 70, 90);

std::atomic_bool isFullyConnected(false);
std::atomic_bool clientRunning(true);
std::atomic_bool isConnected(false);
int clientId = -1;
SOCKET clientSocket = INVALID_SOCKET;

HWND hWndMain, hChatList, hInputEdit, hSendBtn, hExitBtn;
HBRUSH hBackgroundBrush;
HFONT hFont;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RecvThread();
void AddChatMessage(const std::string& msg);
void SafeDisconnect();
void InitControls(HWND hWnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ChatClient";
    wc.hbrBackground = CreateSolidBrush(BG_COLOR);
    RegisterClassW(&wc);

    hWndMain = CreateWindowW(L"ChatClient", L"Chat Client", WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600, NULL, NULL, hInstance, NULL);

    InitControls(hWndMain);
    hBackgroundBrush = CreateSolidBrush(BG_COLOR);
    hFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBoxW(hWndMain, L"Network initialization failed", L"Error", MB_ICONERROR);
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        MessageBoxW(hWndMain, L"Socket creation failed", L"Error", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBoxW(hWndMain, L"Connection failed", L"Error", MB_ICONERROR);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    isConnected = true;
    std::thread(RecvThread).detach();

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(hFont);
    DeleteObject(hBackgroundBrush);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, TEXT_COLOR);
            SetBkColor(hdc, INPUT_BG_COLOR);
            return (LRESULT)CreateSolidBrush(INPUT_BG_COLOR);
        }

        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, TEXT_COLOR);
            SetBkColor(hdc, BG_COLOR);
            return (LRESULT)hBackgroundBrush;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case 3: // Send
                    if (!isConnected || !isFullyConnected) {
                        MessageBoxW(hWnd, L"Connection lost", L"Error", MB_ICONWARNING);
                        SafeDisconnect();
                        break;
                    }

                    wchar_t buffer[BUFFER_SIZE];
                    GetWindowTextW(hInputEdit, buffer, BUFFER_SIZE);
                    if (wcslen(buffer) > 0) {
                        char narrowBuffer[BUFFER_SIZE];
                        WideCharToMultiByte(CP_UTF8, 0, buffer, -1, narrowBuffer, BUFFER_SIZE, NULL, NULL);

                        if (send(clientSocket, narrowBuffer, strlen(narrowBuffer), 0) == SOCKET_ERROR) {
                            MessageBoxW(hWnd, L"Failed to send message", L"Error", MB_ICONERROR);
                            SafeDisconnect();
                            break;
                        }

                        std::wstring selfMsg = L"You: " + std::wstring(buffer);
                        AddChatMessage(std::string(selfMsg.begin(), selfMsg.end()));
                        SetWindowTextW(hInputEdit, L"");
                    }
                    break;

                case 4: // Exit
                    PostMessageW(hWnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
        }

        case WM_CLOSE:
            SafeDisconnect();
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_USER: {
            wchar_t* msg = (wchar_t*)lParam;
            SendMessageW(hChatList, LB_ADDSTRING, 0, (LPARAM)msg);
            delete[] msg;
            break;
        }
        case WM_USER+1: {
            EnableWindow(hSendBtn, TRUE); // Разблокируем кнопку
            break;
        }

        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlType != ODT_BUTTON) return FALSE;

            SetTextColor(dis->hDC, TEXT_COLOR);
            SetBkColor(dis->hDC, BUTTON_BG_COLOR);

            FillRect(dis->hDC, &dis->rcItem, CreateSolidBrush(BUTTON_BG_COLOR));

            wchar_t text[128];
            GetWindowTextW(dis->hwndItem, text, 128);

            DrawTextW(dis->hDC, text, -1, &dis->rcItem,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            FrameRect(dis->hDC, &dis->rcItem, CreateSolidBrush(
                (dis->itemState & ODS_SELECTED) ? RGB(120, 120, 140) : RGB(90, 90, 110)
            ));
            return TRUE;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void InitControls(HWND hWnd) {
    hChatList = CreateWindowW(L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        10, 10, 760, 480, hWnd, (HMENU)1, NULL, NULL);

    hInputEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        10, 500, 600, 30, hWnd, (HMENU)2, NULL, NULL);

    hSendBtn = CreateWindowW(L"BUTTON", L"Send", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        620, 500, 80, 30, hWnd, (HMENU)3, NULL, NULL);

    hExitBtn = CreateWindowW(L"BUTTON", L"Exit", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        710, 500, 80, 30, hWnd, (HMENU)4, NULL, NULL);

    for (HWND hCtrl : {hChatList, hInputEdit, hSendBtn, hExitBtn}) {
        SendMessageW(hCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    EnableWindow(hSendBtn, FALSE); // Начальное состояние заблокировано
}

void RecvThread() {
    char buffer[BUFFER_SIZE];
    while (clientRunning && isConnected) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0';
        std::string msg(buffer);

        if (msg == "SERVER_SHUTDOWN") {
            AddChatMessage("[System] Server has been shut down");
            SafeDisconnect();
            break;
        }
        else if (msg.find("YourID:") == 0) {
            clientId = std::stoi(msg.substr(7));
            isFullyConnected=true;
            PostMessageW(hWndMain, WM_USER+1, 0, 0); // Уведомление GUI
            AddChatMessage("[System] Connected! Your ID: " + std::to_string(clientId));
        }
        else {
            AddChatMessage(msg);
        }
    }
    SafeDisconnect();
}

void AddChatMessage(const std::string& msg) {
    wchar_t* buf = new wchar_t[msg.size() * 2 + 1];
    MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, buf, msg.size() * 2 + 1);
    PostMessageW(hWndMain, WM_USER, 0, (LPARAM)buf);
}

void SafeDisconnect() {
    if (!isConnected) return;

    isConnected = false;
    clientRunning = false;
    isFullyConnected = false;
    EnableWindow(hSendBtn, FALSE);

    if (clientSocket != INVALID_SOCKET) {
        shutdown(clientSocket, SD_BOTH);
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    WSACleanup();

    AddChatMessage("[System] Disconnected from server");
}