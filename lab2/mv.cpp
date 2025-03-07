#include <string>
#include <iostream>
#include <filesystem>

void mv(const std::string& input_file,const std::string& output_file) {
    try {
        const std::filesystem::path source = input_file;
        std::filesystem::path target = output_file;
        if (!exists(source)) {
            throw std::runtime_error("Исходный файл не существует: " + input_file);
        }
        if (exists(target)) {
            remove(target);
        }
        if (is_directory(target)) {
            target /= source.filename();
        } else {
            if (target.parent_path() != source.parent_path()) {
                create_directories(target.parent_path());
            }
        }
        rename(source, target);
        std::cout << "Файл успешно переименован/перемещён" << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Ошибка файловой системы: " + std::string(e.what()));
    }
}