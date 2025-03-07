#include <cstring>
#include <iostream>

/*
g++-14 --std=c++26 -Wall -Wpedantic -Wextra -Werror main.cpp chmod.cpp cp.cpp info.cpp mv.cpp read_n.cpp -o lab2
Необходимо написать программу для работы с файлами, по-
лучающую информацию из командной строки или из консоли вво-
да.
Программа должна корректно обрабатывать ключи и аргумен-
ты в случае ввода из командной строки, либо программа должна
показывать список действий в случае ввода из консоли ввода.
Программа должна иметь возможность осуществлять:
- копирование файлов,
- перемещение файлов,
- получение информации о файле (права, размер, время изме-
нения),
- изменение прав на выбранный файл.
Просто вызвать функцию для копирования файла нельзя.
Также программа должна иметь help для работы с ней, он должен
вызываться при запуске программы с ключом --help.
Копирование файла должно производиться при помощи ко-
манд блочного чтения и записи в файл. Размер буфера для чтения
и записи должен быть больше единицы. Не допускается так же
производить копирование файла при помощи однократной коман-
ды чтения и записи, так как при таком подходе предполагается,
что оперативной памяти достаточно, чтобы прочитать весь файл
одной командой. Это неверно, так как в общем случае размер фай-
ла может существенно превышать оперативную память и файл
подкачки.
 */
void help() {
    std::cout <<
        " ./lab2 cp <input_file> <output_file> для копирования файлов\n"
        " <input_file> - имя файла, который копируем\n"
        " <output_file> - имя файла, в который копируем, если его не существует, то он создаётся "<< std::endl;
    std::cout <<
        "mv <input_file> <output_file> для перемещения файла\n"
        "<input_file> - имя файла, который перемещаем\n"
        "<output_file> - имя файла, в который перемещаем, если это каталог, то перемещаем в каталог" << std::endl;
    std::cout <<
        "info <input_file>"
        "<input_file> - имя файла, о котором получаем информацию (права, размер, время изменения)" << std::endl;
    std::cout <<
        "chmod [ugoa]*([-+=]([rwxXst]*|[ugo]))+|[-+=][0-7]+ <input_file>\n"
        "<input_file> - имя файла, права которого изменяем\n"
        "+ - добавление полномочий\n"
        "- - отобрать полномочия\n"
        "= - установить полномочия"<< std::endl;
}

extern void cp(const std::string& input_file, const std::string& output_file);
extern void mv(const std::string& input_file, const std::string& output_file);
extern void info(const std::string& input_file);
extern void chmod(const std::string& mode, const std::string& input_file);
extern void read_n(const std::string& input_file);


int main(const int argc, char* argv[]) {
    if (argc >= 5 || argc == 1) {
        throw std::invalid_argument("Wrong number of arguments");
    }
    if (strcmp(argv[1], "--help") == 0) {
        if (argc != 2) throw std::invalid_argument("Wrong number of arguments");
        help();
    } else if (strcmp(argv[1], "cp") == 0) {
        if (argc != 4) {
            throw std::invalid_argument("Wrong number of arguments");
        }
        cp(argv[2], argv[3]);
    } else if (strcmp(argv[1], "mv") == 0) {
        if (argc != 4) {
            throw std::invalid_argument("Wrong number of arguments");
        }
        mv(argv[2], argv[3]);
    } else if  (strcmp(argv[1], "info") == 0) {
        if (argc != 3) {
            throw std::invalid_argument("Wrong number of arguments");
        }
        info(argv[2]);
    } else if (strcmp(argv[1], "chmod") == 0) {
        if (argc != 4) {
            throw std::invalid_argument("Wrong number of arguments");
        }
        chmod(argv[2], argv[3]);
    } else if (strcmp(argv[1], "read_n") == 0) { //Доп функция
        if (argc != 3)
            throw std::invalid_argument("Wrong number of arguments");
        read_n(argv[2]);
    }
    else
        throw std::invalid_argument("Wrong arguments");
}