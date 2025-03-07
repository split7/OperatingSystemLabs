#include <iostream>
#include <string>
#include <fstream>
//Вывести нечётные строки файла(Доп функция)
void read_n(const std::string& input_file) {

    std::ifstream ifs(input_file, std::ios::binary);
    if (!ifs.is_open())
        throw std::invalid_argument("read_n: could not open input file");
    std::string line;
    while (!ifs.eof()) {
        std::getline(ifs, line);
        std::cout << line << std::endl;
        if (!ifs.eof()) {
            std::getline(ifs, line);
        }
    }
    ifs.close();
}
