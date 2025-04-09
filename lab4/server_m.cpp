/*
С процесса-сервера запускается n процессов клиентов. Для каждого из созданных клиентов указывается
время жизни(в секундах). Клиент запускается, существует заданное время и завершает работу.
Также следует предусмотреть значение для бесконечного времени. Требуется не менее трёх одновременно
запускаемых процессов-клиентов.
 */
#include <windows.h>
#include <iostream>
#include <vector>
#include <memory>

struct ClientData {
    std::wstring pipeName; //Имя канала
    DWORD lifetime; //Время жизни
    HANDLE hPipe; //Дескриптор канала
    HANDLE hEvent;
};

DWORD WINAPI ClientThread(LPVOID lpParameter) {
    std::unique_ptr<ClientData> clientData(static_cast<ClientData*>(lpParameter));

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
        std::wcerr << "[ERROR] CreateNamedPipe failed with "<< GetLastError() << std::endl;
        SetEvent(clientData->hEvent);
        return 1;
    }

    if (!ConnectNamedPipe(clientData->hPipe, NULL)) {
        std::wcerr << "[ERROR] Connection failed with " << GetLastError() << std::endl;
        CloseHandle(clientData->hPipe);
        SetEvent(clientData->hEvent);
        return 1;
    }

    DWORD bytesWritten;
    if (!WriteFile(clientData->hPipe, &clientData->lifetime, sizeof(clientData->lifetime), &bytesWritten, NULL)) {
        std::wcerr << "[ERROR] WriteFile failed with " << GetLastError() << std::endl;
    } else {
        std::wcout << "To client " << clientData->pipeName << " send time: " << clientData->lifetime << " seconds"<< std::endl;
    }

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
        std::cout << "Enter the lifetime of client " << i + 1 << "(0) - infinite ";
        std::cin >> lifetime;

        auto client = std::make_unique<ClientData>(); //Информация для клиента
        client->lifetime = lifetime;
        client->pipeName = L"\\\\.\\pipe\\mynamedpipe" + std::to_wstring(i);
        client->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        std::wstring clientPipeName = client->pipeName;

        HANDLE hThread = CreateThread(
            NULL,
            0,
            ClientThread,
            client.get(),
            0,
            NULL);

        if (hThread) {
            client.release();
            events.push_back(client->hEvent);

            std::wstring cmd = L"client.exe " + clientPipeName;
            STARTUPINFO startupInfo = {sizeof(startupInfo)};
            PROCESS_INFORMATION processInformation;
            if (!CreateProcessW(
            NULL,
            &cmd[0],
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            reinterpret_cast<LPSTARTUPINFOW>(&startupInfo),
            &processInformation)) {
                std::wcerr << "[ERROR] CreateProcess failed with " << GetLastError() << std::endl;
            }else {
                CloseHandle(processInformation.hThread);
                CloseHandle(processInformation.hProcess);
            }
        } else {
            std::wcerr << "[ERROR] CreateThread failed with " << GetLastError() << std::endl;
            CloseHandle(client->hEvent);
        }
    }

    WaitForMultipleObjects(events.size(), events.data(), TRUE, INFINITE);

    for (HANDLE thread : events) {
        CloseHandle(thread);
    }

    return 0;
}
