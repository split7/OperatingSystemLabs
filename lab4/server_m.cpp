#include <windows.h>
#include <iostream>
#include <vector>
#include <memory>

struct ClientData {
    std::wstring pipeName;
    DWORD lifetime;
    HANDLE hPipe;
    HANDLE hEvent;
    HANDLE hPipeReadyEvent;
};

DWORD WINAPI ClientThread(LPVOID lpParameter) {
    std::unique_ptr<ClientData> clientData(static_cast<ClientData*>(lpParameter));

    // Создание канала
    clientData->hPipe = CreateNamedPipeW(
        clientData->pipeName.c_str(),
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        1024,
        1024,
        0,
        NULL);

    if (clientData->hPipe == INVALID_HANDLE_VALUE) {
        std::wcerr << "[ERROR] CreateNamedPipe failed with " << GetLastError() << std::endl;
        SetEvent(clientData->hEvent);
        return 1;
    }

    // Уведомляем, что канал готов
    SetEvent(clientData->hPipeReadyEvent);

    // Ожидание подключения клиента
    if (!ConnectNamedPipe(clientData->hPipe, NULL)) {
        std::wcerr << "[ERROR] Connection failed with " << GetLastError() << std::endl;
        CloseHandle(clientData->hPipe);
        SetEvent(clientData->hEvent);
        return 1;
    }

    // Отправка времени клиенту
    DWORD bytesWritten;
    if (!WriteFile(clientData->hPipe, &clientData->lifetime, sizeof(clientData->lifetime), &bytesWritten, NULL)) {
        std::wcerr << "[ERROR] WriteFile failed with " << GetLastError() << std::endl;
    } else {
        std::wcout << "To client " << clientData->pipeName << " sent time: " << clientData->lifetime << " seconds" << std::endl;
    }

    // Завершение работы
    FlushFileBuffers(clientData->hPipe);
    DisconnectNamedPipe(clientData->hPipe);
    CloseHandle(clientData->hPipe);
    SetEvent(clientData->hEvent);
    return 0;
}

int main() {
    int clientCount;
    std::cout << "Enter the number of client processes: ";
    std::cin >> clientCount;

    std::vector<HANDLE> events;

    for (int i = 0; i < clientCount; ++i) {
        DWORD lifetime;
        std::cout << "Enter the lifetime of client " << i + 1 << " (0 - infinite): ";
        std::cin >> lifetime;

        auto client = std::make_unique<ClientData>();
        client->lifetime = lifetime;
        client->pipeName = L"\\\\.\\pipe\\mynamedpipe_" + std::to_wstring(i);
        client->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        client->hPipeReadyEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        // Сохраняем дескрипторы событий ДО передачи владения
        HANDLE hEvent = client->hEvent;
        HANDLE hPipeReadyEvent = client->hPipeReadyEvent;
        events.push_back(hEvent);

        std::wstring clientPipeName = client->pipeName;

        // Запуск потока
        HANDLE hThread = CreateThread(
            NULL,
            0,
            ClientThread,
            client.get(),
            0,
            NULL);

        if (hThread) {
            client.release(); // Владение передано потоку
            CloseHandle(hThread);

            // Ждем готовности канала через локальный дескриптор
            WaitForSingleObject(hPipeReadyEvent, INFINITE);

            // Запуск клиента
            std::wstring cmd = L"client.exe \"" + clientPipeName + L"\"";
            STARTUPINFO startupInfo = { sizeof(startupInfo) };
            PROCESS_INFORMATION processInformation;
            std::wcout << cmd << std::endl;
            if (!CreateProcessW(
                NULL,
                const_cast<LPWSTR>(cmd.data()),
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                reinterpret_cast<LPSTARTUPINFOW>(&startupInfo),
                &processInformation)) {
                std::wcerr << "[ERROR] CreateProcess failed with " << GetLastError() << std::endl;
            } else {
                CloseHandle(processInformation.hThread);
                CloseHandle(processInformation.hProcess);
            }
        } else {
            std::wcerr << "[ERROR] CreateThread failed with " << GetLastError() << std::endl;
            CloseHandle(client->hEvent);
            CloseHandle(client->hPipeReadyEvent);
            delete client.release();
        }
    }

    // Ожидание завершения всех клиентов
    WaitForMultipleObjects(events.size(), events.data(), TRUE, INFINITE);

    // Очистка ресурсов
    for (HANDLE event : events) {
        CloseHandle(event);
    }

    return 0;
}