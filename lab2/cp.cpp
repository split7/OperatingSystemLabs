#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

/*
Функция копирования файлов
- название копируемого файла
- название копии файла
- копирует содержимое
 */
void cp(const std::string& input_file,const std::string& output_file) {
    if (input_file == output_file) {
        throw std::invalid_argument("cp: input_file == output_file");
    }
    std::ifstream ifs(input_file, std::ios::binary);
    if (!ifs.is_open())
        throw std::invalid_argument("cp: could not open input file");
    if (std::filesystem::path target = output_file, source = input_file; !exists(target) && target.parent_path() != source.parent_path()) {
        create_directories(target.parent_path());
    }
    std::ofstream ofs(output_file, std::ios::binary);
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
