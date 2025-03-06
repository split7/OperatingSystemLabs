#include <iostream>
#include <string>
#include <sys/stat.h>
#include <iomanip>

void info(const std::string& str1) {
    struct stat64 file_stat;
    if (stat64(str1.c_str(), &file_stat)) {
        throw std::runtime_error("info: stat failed");
    }
    std::cout << std::oct << (file_stat.st_mode & 0777) << " " << file_stat.st_size << " " << asctime(localtime(&file_stat.st_mtim.tv_sec)) << std::endl;
}