// Сервер (server.cpp)
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

    // Создание канала с двунаправленным доступом
    clientData->hPipe = CreateNamedPipeW(
        clientData->pipeName.c_str(),
        PIPE_ACCESS_OUTBOUND,  // Изменено на DUPLEX для двусторонней связи
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

    // Уведомляем, что канал готов к подключению
    SetEvent(clientData->hPipeReadyEvent);

    // Ожидание подключения клиента
    if (!ConnectNamedPipe(clientData->hPipe, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            std::wcerr << L"[ERROR] Connection failed with " << err << std::endl;
            CloseHandle(clientData->hPipe);
            SetEvent(clientData->hEvent);
            return 1;
        }
    }

    // Отправка времени жизни
    DWORD bytesWritten;
    if (!WriteFile(clientData->hPipe, &clientData->lifetime, sizeof(clientData->lifetime), &bytesWritten, NULL)) {
        std::wcerr << L"[ERROR] WriteFile failed with " << GetLastError() << std::endl;
    }

    // Закрытие канала
    FlushFileBuffers(clientData->hPipe);
    DisconnectNamedPipe(clientData->hPipe);
    CloseHandle(clientData->hPipe);
    SetEvent(clientData->hEvent);
    return 0;
}

int wmain() {
    int clientCount;
    std::wcout << L"Enter the number of client processes: ";
    std::cin >> clientCount;

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

        HANDLE hEvent = client->hEvent;
        HANDLE hPipeReadyEvent = client->hPipeReadyEvent;
        events.push_back(hEvent);

        // Запуск потока
        HANDLE hThread = CreateThread(NULL, 0, ClientThread, client.get(), 0, NULL);
        if (!hThread) {
            std::wcerr << L"[ERROR] CreateThread failed: " << GetLastError() << std::endl;
            CloseHandle(client->hEvent);
            CloseHandle(client->hPipeReadyEvent);
            continue;
        }
        client.release();
        CloseHandle(hThread);

        // Ожидание готовности канала
        WaitForSingleObject(hPipeReadyEvent, INFINITE);

        // Запуск клиента с правильными аргументами
        std::wstring cmd = L"client.exe " + client->pipeName;
        std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
        cmdLine.push_back(L'\0'); // Обязательный нулевой терминатор

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (!CreateProcessW(
            L"client.exe",
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
    }

    WaitForMultipleObjects(events.size(), events.data(), TRUE, INFINITE);

    for (HANDLE event : events) {
        CloseHandle(event);
    }

    return 0;
}
