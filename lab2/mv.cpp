#include <string>
#include <cstdio>
#include <iostream>

void mv(const std::string& str1,const std::string& str2) {
    if (rename(str1.c_str(),str2.c_str()) == 0) {
        std::cout << "Файл успешно переименован/перемещён" << std::endl;
        return;
    }
    throw std::runtime_error("Ошибка переименования");
}