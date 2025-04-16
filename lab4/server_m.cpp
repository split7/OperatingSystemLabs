#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

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
        clientData->pipeName.c_str(), //Имя канала
        PIPE_ACCESS_DUPLEX, //Двунаправленный канал
        PIPE_TYPE_MESSAGE | //Данные записываются в канал в виде потока сообщений. Канал обрабатывает байты, записанные во время каждой операции записи как единицу сообщения.
        PIPE_READMODE_MESSAGE | //Данные считываются из канала в виде потока сообщений.
        PIPE_WAIT, //Режим блокировки включен.
        1, //Максимальное количество экземпляров, которые можно создать для этого канала.
        1024, //Количество байтов, зарезервировать для выходного буфера.
        1024, //Количество байтов для резервирования входного буфера.
        0, //Значение времени ожидания по умолчанию в миллисекундах, 0 - 50мс
        NULL); //дескриптор безопасности

    if (clientData->hPipe == INVALID_HANDLE_VALUE) { //Если функция завершается ошибкой, возвращаемое значение INVALID_HANDLE_VALUE.
        std::wcerr << L"[ERROR] CreateNamedPipe failed with " << GetLastError() << std::endl;
        SetEvent(clientData->hEvent); //Задает для указанного объекта события состояние сигнала.
        return 1;
    }

    SetEvent(clientData->hPipeReadyEvent);

    //Позволяет серверу именованного канала ожидать подключения клиентского процесса к экземпляру именованного канала.
    if (!ConnectNamedPipe(clientData->hPipe, //A handle to the server end of a named pipe instance.
        NULL)) { //Указатель на структуру OVERLAPPED.
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) { //Если клиент подключается перед вызовом функции, функция возвращает ноль и GetLastError возвращает ERROR_PIPE_CONNECTED.
            std::wcerr << L"[ERROR] Connection failed with " << err << std::endl;
            CloseHandle(clientData->hPipe);
            SetEvent(clientData->hEvent);
            return 1;
        }
    }

    DWORD bytesWritten;
    if (!WriteFile(clientData->hPipe, //Дескриптор устройства ввода-вывода или файла
        &clientData->lifetime, //Указатель на буфер, содержащий данные, записываемые в файл или устройство.
        sizeof(clientData->lifetime), //Количество байтов, записываемых в файл или устройство.
        &bytesWritten, //Указатель на переменную, получающую количество байтов, записанных при использовании синхронного параметра hFile.
        NULL)) {
        std::wcerr << L"[ERROR] WriteFile failed with " << GetLastError() << std::endl;
    }

    FlushFileBuffers(clientData->hPipe); //Очищает буферы указанного файла и приводит к записи всех буферных данных в файл.
    DisconnectNamedPipe(clientData->hPipe); //Отключает серверную часть экземпляра именованного канала от клиентского процесса.
    CloseHandle(clientData->hPipe); //Закрывает открытый дескриптор объекта
    SetEvent(clientData->hEvent);
    return 0;
}

void uncorrect() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "I don't understand you, sorry. Please, try again.\n";
}

int checkInt() {
    int res;
    std::cin >> res;
    while (std::cin.fail() || std::cin.peek() != '\n') {
        uncorrect();
        std::cout << "Give me an integer: ";
        std::cin >> res;
    }
    return res;
}

int wmain() {
    int clientCount;
    do {
        std::wcout << L"Enter the number of client processes (at least 3): ";
        clientCount = checkInt();
    } while (clientCount < 3);

    std::vector<HANDLE> events;

    for (int i = 0; i < clientCount; ++i) {
        std::wcout << L"Enter the lifetime of client " << i + 1 << L" (0 - infinite): ";
        DWORD lifetime = checkInt();

        auto client = std::make_unique<ClientData>();
        client->lifetime = lifetime;
        client->pipeName = L"\\\\.\\pipe\\mynamedpipe_" + std::to_wstring(i + 1);
        //Создает или открывает именованный или неименованный объект события.
        client->hEvent = CreateEventW(NULL, //дескриптор не может наследоваться дочерними процессами
            TRUE, //функция создает объект события ручного сброса
            FALSE, //Не назначено начальное состояние объекта
            NULL); //объект события создается без имени.
        client->hPipeReadyEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        std::wstring pipeName = client->pipeName; // Сохраняем имя канала до передачи владения
        HANDLE hEvent = client->hEvent;
        HANDLE hPipeReadyEvent = client->hPipeReadyEvent;
        events.push_back(hEvent);

        HANDLE hThread = CreateThread(NULL, //дескриптор не может быть унаследован
            0, //Начальный размер стека в байтах. Если этот параметр равен нулю, новый поток использует размер по умолчанию для исполняемого файла.
            ClientThread, //Указатель на определяемую приложением функцию, выполняемую потоком.
            client.get(), //Указатель на переменную, передаваемую потоку
            0, //Поток выполняется сразу после создания
            NULL); //идентификатор потока не возвращается
        if (!hThread) {
            std::wcerr << L"[ERROR] CreateThread failed: " << GetLastError() << std::endl;
            CloseHandle(client->hEvent);
            CloseHandle(client->hPipeReadyEvent);
            continue;
        }
        client.release(); //Освобождает владение возвращаемого сохраненного указателя в вызывающий объект и задает сохраненному указателю значение nullptr

        WaitForSingleObject(hPipeReadyEvent, INFINITE); //Ожидает, пока указанный объект находится в сигнальном состоянии или истекает интервал времени ожидания.

        std::wstring cmd = L"client.exe " + pipeName;
        std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
        cmdLine.push_back(L'\0');

        STARTUPINFOW si = { sizeof(si) }; //Указывает станцию окон, рабочий стол, стандартные дескрипторы и внешний вид основного окна для процесса во время создания.
        PROCESS_INFORMATION pi; //Содержит сведения о вновь созданном процессе и его основном потоке.
        //Создает новый процесс и его основной поток. Новый процесс выполняется в контексте безопасности вызывающего процесса.
        if (!CreateProcessW(NULL, //Параметр lpApplicationName можно NULL. В этом случае имя модуля должно быть первым маркером с разделителями пробелов в строке lpCommandLine.
            cmdLine.data(), //Командная строка, выполняемая.
            NULL, //дескриптор не может быть унаследован
            NULL, // поток получает дескриптор безопасности по умолчанию
            FALSE, //дескриптор не наследуется
            CREATE_NEW_CONSOLE,
            NULL, //новый процесс использует среду вызывающего процесса
            NULL, //новый процесс будет иметь тот же текущий диск и каталог, что и вызывающий процесс
            &si, //Указатель на структуру
            &pi)) {
            std::wcerr << L"[ERROR] CreateProcess failed: " << GetLastError() << std::endl;
        } else {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        CloseHandle(hThread);
    }

    WaitForMultipleObjects(events.size(), events.data(), TRUE, INFINITE); //Ожидает, пока один или все указанные объекты находятся в сигнальном состоянии или интервале ожидания.

    for (HANDLE event : events) {
        CloseHandle(event);
    }

    return 0;
}