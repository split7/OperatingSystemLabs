#include <windows.h>
#include <iostream>
#include <string>

#define PIPE_NAME L"\\\\.\\pipe\\mychat"
#define BUFFER_SIZE 512

DWORD WINAPI ReadMessages(LPVOID param) {
    HANDLE hPipe = (HANDLE)param;
    wchar_t buffer[BUFFER_SIZE];
    DWORD bytesRead;

    while (true) {
        if (!ReadFile(hPipe,
            buffer,
            BUFFER_SIZE * sizeof(wchar_t),
            &bytesRead,
            NULL)) {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE)
                std::wcout << L"Disconnected from server." << std::endl;
            else
                std::wcerr << L"Read error: " << err << std::endl;
            break;
        }

        if (bytesRead == 0)
            break;

        buffer[bytesRead / sizeof(wchar_t)] = L'\0';
        std::wcout << buffer << std::endl;
    }
    return 0;
}

int wmain() {
    if (!WaitNamedPipeW(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        std::wcerr << L"Server unavailable. Error: " << GetLastError() << std::endl;
        return 1;
    }

    HANDLE hPipe = CreateFileW(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to connect. Error: " << GetLastError() << std::endl;
        return 1;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(hPipe, &mode, NULL, NULL)) {
        std::wcerr << L"SetNamedPipeHandleState failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return 1;
    }

    HANDLE hThread = CreateThread(NULL, 0, ReadMessages, hPipe, 0, NULL);
    if (!hThread) {
        std::wcerr << L"CreateThread failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return 1;
    }

    std::wstring message;
    while (std::getline(std::wcin, message)) {
        if (message == L"exit")
            break;

        DWORD bytesWritten;
        if (!WriteFile(hPipe,
            message.c_str(),
            (message.size() + 1) * sizeof(wchar_t),
            &bytesWritten,
            NULL)) {
            std::wcerr << L"Write failed. Error: " << GetLastError() << std::endl;
            break;
        }
    }

    CancelIo(hPipe);
    CloseHandle(hPipe);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    return 0;
}