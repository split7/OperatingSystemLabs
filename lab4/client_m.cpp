#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        std::wcerr << L"Using: client.exe <namedPipe>" << std::endl;
        return 1;
    }

    HANDLE hPipe; //Подключение к каналу
    while (true) {
        hPipe = CreateFileW(
            argv[1],
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (hPipe == INVALID_HANDLE_VALUE) break;

        if(GetLastError() == ERROR_PIPE_BUSY) {
            std::wcerr << "Cant connect with " << GetLastError() << std::endl;
            return 1;
        }

        WaitNamedPipeW(argv[1], NMPWAIT_WAIT_FOREVER);
    }

    DWORD lifetime;
    DWORD bytesRead;

    if (!ReadFile(hPipe, &lifetime, sizeof(lifetime), &bytesRead, NULL)) {
        std::wcerr << "ReadFile failed with " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return 1;
    }

    CloseHandle(hPipe);

    if(lifetime == 0) {
        std::wcout << "Infinite lifetime" << std::endl;
        while (true) {
            Sleep(1000);
            std::wcout << "Working..." << std::endl;
        }
    }
    for (DWORD i = lifetime;i > 0 ; --i) {
        std::wcout << i << " seconds left" << std::endl;
        Sleep(1000);
    }
    std::wcout << "Time's up"<<std::endl;
    return 0;
}