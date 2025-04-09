#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        std::wcerr << L"Using: client.exe <namedPipe>" << std::endl;
        return 1;
    }

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    const DWORD maxRetries = 10;
    const DWORD retryDelay = 500;
    DWORD retryCount = 0;

    while (retryCount < maxRetries) {
        hPipe = CreateFileW(
            argv[1],
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hPipe != INVALID_HANDLE_VALUE) break;

        DWORD lastError = GetLastError();
        if (lastError == ERROR_PIPE_BUSY || lastError == ERROR_FILE_NOT_FOUND) {
            Sleep(retryDelay);
            retryCount++;
        } else {
            std::wcerr << L"CreateFile failed with error " << lastError << std::endl;
            return 1;
        }
    }

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to connect after " << maxRetries << L" attempts" << std::endl;
        return 1;
    }

    DWORD lifetime;
    DWORD bytesRead;
    if (!ReadFile(hPipe, &lifetime, sizeof(lifetime), &bytesRead, NULL)) {
        std::wcerr << L"ReadFile failed with " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return 1;
    }

    CloseHandle(hPipe);

    if (lifetime == 0) {
        std::wcout << L"Infinite lifetime" << std::endl;
        while (true) {
            Sleep(1000);
            std::wcout << L"Working..." << std::endl;
        }
    }
    for (DWORD i = lifetime; i > 0; --i) {
        std::wcout << i << L" seconds left" << std::endl;
        Sleep(1000);
    }
    std::wcout << L"Time's up" << std::endl;

    return 0;
}