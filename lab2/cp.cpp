#include <fstream>
#include <iostream>
#include <memory>
#include <string>

/*
Функция копирования файлов
- название копируемого файла
- название копии файла
- копирует содержимое
 */
void cp(const std::string& str1,const std::string& str2) {
    if (str1 == str2) {
        throw std::invalid_argument("cp: str1 == str2");
    }
    std::ifstream ifs(str1, std::ios::binary);
    if (!ifs.is_open())
        throw std::invalid_argument("cp: could not open input file");
    std::ofstream ofs(str2, std::ios::binary);
    if (!ofs.is_open()) {
        ifs.close();
        throw std::invalid_argument("cp: could not open output file");
    }
    size_t bufsize = 256;
    auto buf = std::make_unique<char[]>(bufsize);
    while (ifs.read(buf.get(), static_cast<std::streamsize>(bufsize)))
        ofs.write(buf.get(), ifs.gcount());
    if (ifs.gcount() > 0)
        ofs.write(buf.get(), ifs.gcount());
    ifs.close();
    ofs.close();
    std::cout << "Копирование завершено" << std::endl;
}
