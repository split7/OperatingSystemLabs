#include <windows.h>
#include <iostream>
#include <vector>
#include <memory>
#include <string>

struct ClientData {
    std::wstring pipeName;
    DWORD lifetime;
    HANDLE hPipe;
    HANDLE hEvent;
    HANDLE hPipeReadyEvent;
};

DWORD WINAPI ClientThread(LPVOID lpParameter) {
    std::unique_ptr<ClientData> clientData(static_cast<ClientData*>(lpParameter));

    clientData->hPipe = CreateNamedPipeW(
        clientData->pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        1024,
        1024,
        0,
        NULL);

    if (clientData->hPipe == INVALID_HANDLE_VALUE) {
        std::wcerr << L"[ERROR] CreateNamedPipe failed with " << GetLastError() << std::endl;
        SetEvent(clientData->hEvent);
        return 1;
    }

    SetEvent(clientData->hPipeReadyEvent);

    if (!ConnectNamedPipe(clientData->hPipe, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            std::wcerr << L"[ERROR] Connection failed with " << err << std::endl;
            CloseHandle(clientData->hPipe);
            SetEvent(clientData->hEvent);
            return 1;
        }
    }

    DWORD bytesWritten;
    if (!WriteFile(clientData->hPipe, &clientData->lifetime, sizeof(clientData->lifetime), &bytesWritten, NULL)) {
        std::wcerr << L"[ERROR] WriteFile failed with " << GetLastError() << std::endl;
    }

    FlushFileBuffers(clientData->hPipe);
    DisconnectNamedPipe(clientData->hPipe);
    CloseHandle(clientData->hPipe);
    SetEvent(clientData->hEvent);
    return 0;
}

int wmain() {
    int clientCount;
    do {
        std::wcout << L"Enter the number of client processes (at least 3): ";
        std::cin >> clientCount;
    } while (clientCount < 3);

    std::vector<HANDLE> events;

    for (int i = 0; i < clientCount; ++i) {
        DWORD lifetime;
        std::wcout << L"Enter the lifetime of client " << i + 1 << L" (0 - infinite): ";
        std::cin >> lifetime;

        auto client = std::make_unique<ClientData>();
        client->lifetime = lifetime;
        client->pipeName = L"\\\\.\\pipe\\mynamedpipe_" + std::to_wstring(i);
        client->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        client->hPipeReadyEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        std::wstring pipeName = client->pipeName; // Сохраняем имя канала до передачи владения
        HANDLE hEvent = client->hEvent;
        HANDLE hPipeReadyEvent = client->hPipeReadyEvent;
        events.push_back(hEvent);

        HANDLE hThread = CreateThread(NULL, 0, ClientThread, client.get(), 0, NULL);
        if (!hThread) {
            std::wcerr << L"[ERROR] CreateThread failed: " << GetLastError() << std::endl;
            CloseHandle(client->hEvent);
            CloseHandle(client->hPipeReadyEvent);
            continue;
        }
        client.release();

        WaitForSingleObject(hPipeReadyEvent, INFINITE);

        std::wstring cmd = L"client.exe " + pipeName;
        std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
        cmdLine.push_back(L'\0');

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcessW(
            NULL,
            cmdLine.data(),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi)) {
            std::wcerr << L"[ERROR] CreateProcess failed: " << GetLastError() << std::endl;
        } else {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        CloseHandle(hThread);
    }

    WaitForMultipleObjects(events.size(), events.data(), TRUE, INFINITE);

    for (HANDLE event : events) {
        CloseHandle(event);
    }

    return 0;
}