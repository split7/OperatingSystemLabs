#include <windows.h>
#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[]) {

    // Ожидание доступности канала
    if (!WaitNamedPipeW(argv[1], 10000)) {
        std::wcerr << L"WaitNamedPipe failed: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    // Подключение к каналу
    HANDLE hPipe = CreateFileW(
        argv[1],
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::wcerr << L"CreateFile failed: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(hPipe, &mode, NULL, NULL)) {
        std::wcerr << L"SetNamedPipeHandleState failed: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        system("pause");
        return 1;
    }

    // Чтение времени жизни
    DWORD lifetime;
    DWORD bytesRead;
    if (!ReadFile(hPipe, &lifetime, sizeof(lifetime), &bytesRead, NULL)) {
        std::wcerr << L"ReadFile failed: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        system("pause");
        return 1;
    }

    CloseHandle(hPipe);

    // Логика работы
    if (lifetime == 0) {
        std::wcout << L"Running indefinitely..." << std::endl;
        while (true)
            Sleep(1000);
    }
    for (DWORD i = lifetime; i > 0; --i) {
        std::wcout << i << L" seconds remaining" << std::endl;
        Sleep(1000);
    }
    std::wcout << L"Client exiting." << std::endl;
    system("pause");
    return 0;
}