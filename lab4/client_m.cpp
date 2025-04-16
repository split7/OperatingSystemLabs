#include <windows.h>
#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[]) {
    std::wcout << argv[1] << L" client" << std::endl;

    // Ожидание доступности канала
    if (!WaitNamedPipeW(argv[1], 10000)) {
        std::wcerr << L"WaitNamedPipe failed: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    HANDLE hPipe = CreateFileW( //Создает или открывает файл или устройство ввода-вывода
        argv[1], //Имя файла или устройства, которое нужно создать или открыть
        GENERIC_READ | GENERIC_WRITE, //Запрошенный доступ к файлу или устройству, который можно суммировать как чтение, запись или ни один из них
        0, //Если этот параметр равен нулю и CreateFile выполнен успешно, файл или устройство невозможно открыть повторно, пока не будет закрыт дескриптор файла или устройства
        NULL, //дескриптор не может наследоваться любым дочерним процессом, который может создать приложение, и файл или устройство, связанное с возвращенным дескриптором безопасности, возвращает дескриптор безопасности по умолчанию.
        OPEN_EXISTING, //Для устройств, отличных от файлов, Открыть существующий
        0, //Атрибуты файла
        NULL); //файл шаблона

    if (hPipe == INVALID_HANDLE_VALUE) { //Если функция завершается неудачно, возвращается значение INVALID_HANDLE_VALUE
        std::wcerr << L"CreateFile failed: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(//Задает режим чтения и режим блокировки указанного именованного канала.
        hPipe, //Дескриптор экземпляра именованного канала
        &mode, //Данные считываются из канала в виде потока сообщений. Функция завершается ошибкой, если этот флаг указан для канала байтового типа.
        NULL, //Максимальное число байтов, собранных на клиентском компьютере перед передачей на сервер. Этот параметр должен иметь значение NULL , если указанный дескриптор канала находится на серверной стороне именованного канала или если клиентский и серверный процессы находятся на одном компьютере.
        NULL)) { //Максимальное время (в миллисекундах), которое может пройти до передачи данных по сети удаленным именованным каналом. Этот параметр должен иметь значение NULL , если указанный дескриптор канала находится на сервере именованного канала или если клиентские и серверные процессы находятся на одном компьютере.
        std::wcerr << L"SetNamedPipeHandleState failed: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        system("pause");
        return 1;
    }

    // Чтение времени жизни
    DWORD lifetime;
    DWORD bytesRead;
    if (!ReadFile(hPipe, //Дескриптор устройства
        &lifetime, //Указатель на буфер, который получает данные, считываемые из файла или устройства.
        sizeof(lifetime), //Максимальное число байтов для чтения.
        &bytesRead, //Указатель на переменную, которая получает число байтов, считываемых при использовании синхронного параметра hFile.
        NULL)) {
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