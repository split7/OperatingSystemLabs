#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#define PIPE_NAME L"\\\\.\\pipe\\mychat"
#define BUFFER_SIZE 512

HANDLE hMutex;
std::vector<std::wstring> chatHistory;
std::vector<HANDLE> clientHandles;

DWORD WINAPI InstanceThread(LPVOID) {
    while (true) {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            3,
            BUFFER_SIZE * sizeof(wchar_t),
            BUFFER_SIZE * sizeof(wchar_t),
            0,
            NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::wcerr << L"CreateNamedPipe failed: " << GetLastError() << std::endl;
            continue;
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            std::wcerr << L"ConnectNamedPipe failed: " << GetLastError() << std::endl;
            CloseHandle(hPipe);
            continue;
        }

        WaitForSingleObject(hMutex, INFINITE);
        clientHandles.push_back(hPipe);
        std::wcout << L"New client connected. Total: " << clientHandles.size() << std::endl;
        ReleaseMutex(hMutex);

        const std::wstring connectMsg = L"[Server] New client connected";
        WaitForSingleObject(hMutex, INFINITE);
        chatHistory.push_back(connectMsg);
        for (HANDLE client : clientHandles) {
            if (client != hPipe) {
                DWORD bytesWritten;
                WriteFile(client,
                    connectMsg.c_str(),
                    (connectMsg.size() + 1) * sizeof(wchar_t),
                    &bytesWritten,
                    NULL);
            }
        }
        ReleaseMutex(hMutex);

        wchar_t buffer[BUFFER_SIZE];
        DWORD bytesRead;
        while (true) {
            if (!ReadFile(hPipe,
                buffer,
                BUFFER_SIZE * sizeof(wchar_t),
                &bytesRead,
                NULL)) {
                std::wcerr << L"ReadFile failed: " << GetLastError() << std::endl;
                break;
            }

            if (bytesRead == 0)
                break;

            buffer[bytesRead / sizeof(wchar_t)] = L'\0';
            std::wstring message(buffer);

            WaitForSingleObject(hMutex, INFINITE);
            chatHistory.push_back(message);
            for (HANDLE client : clientHandles) {
                if (client != hPipe) {
                    DWORD bytesWritten;
                    WriteFile(client,
                        message.c_str(),
                        (message.size() + 1) * sizeof(wchar_t),
                        &bytesWritten,
                        NULL);
                }
            }
            ReleaseMutex(hMutex);
        }

        WaitForSingleObject(hMutex, INFINITE);
        auto it = std::find(clientHandles.begin(), clientHandles.end(), hPipe);
        if (it != clientHandles.end()) {
            clientHandles.erase(it);
            std::wcout << L"Client disconnected. Total: " << clientHandles.size() << std::endl;
        }
        CloseHandle(hPipe);

        const std::wstring disconnectMsg = L"[Server] Client disconnected";
        chatHistory.push_back(disconnectMsg);
        for (HANDLE client : clientHandles) {
            DWORD bytesWritten;
            WriteFile(client,
                disconnectMsg.c_str(),
                (disconnectMsg.size() + 1) * sizeof(wchar_t),
                &bytesWritten,
                NULL);
        }
        ReleaseMutex(hMutex);
    }
    return 0;
}

int wmain() {
    hMutex = CreateMutexW(NULL, FALSE, NULL);
    if (!hMutex) {
        std::wcerr << L"CreateMutex error: " << GetLastError() << std::endl;
        return 1;
    }

    const int THREAD_COUNT = 3;
    HANDLE threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i] = CreateThread(NULL, 0, InstanceThread, NULL, 0, NULL);
        if (!threads[i]) {
            std::wcerr << L"CreateThread error: " << GetLastError() << std::endl;
            return 1;
        }
    }

    std::wcout << L"Server running. Press Enter to exit..." << std::endl;
    std::wcin.get();

    WaitForSingleObject(hMutex, INFINITE);
    for (HANDLE client : clientHandles) {
        CloseHandle(client);
    }
    clientHandles.clear();
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return 0;
}