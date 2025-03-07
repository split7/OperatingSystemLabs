#include <sys/stat.h>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

//[ugoa]*([-+=]([rwxXst]*|[ugo]))+|[-+=][0-7]+
int chmod(const std::string& mode, const std::string& input_file) {
    bool is_octal = true;
    for (const char c : mode) { //Здесь mode_str не может быть пустым
        if (c < '0' || c > '7') {
            is_octal = false;
            break;
        }
    }
    if (is_octal && mode.length() != 3) {
        throw std::invalid_argument("Invalid octal mode");
    }
    if (is_octal && mode.length() == 3) {
        return chmod(input_file.c_str(), std::stoi(mode, nullptr, 8));
    }
    struct stat file_stat;
    if (stat(input_file.c_str(), &file_stat))
        throw std::runtime_error("chmod: stat failed");

    const mode_t current_mode = file_stat.st_mode & 0777;
    mode_t new_mode = current_mode;

    std::vector<std::string> modes; //Для хранения нескольких операций, через запятую
    size_t start = 0;
    size_t end;

    while ((end = mode.find(',', start)) != std::string::npos) {
        modes.push_back(mode.substr(start, end - start));
        start = end + 1;
    }
    modes.push_back(mode.substr(start)); // Последняя операция

    for (const auto& spec: modes) {
        const size_t operation_pos = spec.find_first_of("+-=");

        if (operation_pos == std::string::npos) {
            throw std::invalid_argument("Missing operator in: '" + spec + "'");
        }
        if (operation_pos == 0) {
            throw std::invalid_argument("Missing user class in: '" + spec + "'");
        }

        const char operation = spec[operation_pos];
        std::string accesses = spec.substr(operation_pos + 1);

        if ((operation == '+' || operation == '-') && accesses.empty()) {
            throw std::invalid_argument("Missing permissions in: '" + spec + "'");
        }

        std::vector<char> users;
        for (std::string user_classes = spec.substr(0, operation_pos); char c: user_classes) {
            if (c == 'a') {
                users.insert(users.end(), {'u', 'g', 'o'});
            } else if (c == 'u' || c == 'g' || c == 'o') {
                users.push_back(c);
            } else {
                throw std::invalid_argument("Invalid class '" + std::string(1, c) + "' in: '" + spec + "'");
            }
        }

        std::ranges::sort(users);
        users.erase(std::ranges::unique(users).begin(), users.end()); // удаление дубликатов(оставить или выбросить исключение???)

        mode_t perm_bits = 0;
        for (const char p: accesses) {
            bool valid = false;

            for (const char user: users) {
                switch (user) {
                    case 'u':
                        switch (p) {
                            case 'r':
                                perm_bits |= S_IRUSR; //https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-chmod-change-mode-file-directory
                                valid = true;
                                break;
                            case 'w':
                                perm_bits |= S_IWUSR;
                                valid = true;
                                break;
                            case 'x':
                                perm_bits |= S_IXUSR;
                                valid = true;
                                break;
                        }
                        break;
                    case 'g':
                        switch (p) {
                            case 'r':
                                perm_bits |= S_IRGRP;
                                valid = true;
                                break;
                            case 'w':
                                perm_bits |= S_IWGRP;
                                valid = true;
                                break;
                            case 'x':
                                perm_bits |= S_IXGRP;
                                valid = true;
                                break;
                        }
                        break;
                    case 'o':
                        switch (p) {
                            case 'r':
                                perm_bits |= S_IROTH;
                                valid = true;
                                break;
                            case 'w':
                                perm_bits |= S_IWOTH;
                                valid = true;
                                break;
                            case 'x':
                                perm_bits |= S_IXOTH;
                                valid = true;
                                break;
                        }
                        break;
                }
            }

            if (!valid) {
                throw std::invalid_argument("Invalid permission '" + std::string(1, p) + "' in: '" + spec + "'");
            }
        }

        mode_t clear_mask = 0; // Для сброса прав
        for (const char user: users) {
            switch (user) {
                case 'u':
                    clear_mask |= S_IRWXU;
                    break;
                case 'g':
                    clear_mask |= S_IRWXG;
                    break;
                case 'o':
                    clear_mask |= S_IRWXO;
                    break;
            }
        }

        switch (operation) {
            case '+':
                new_mode |= perm_bits;
                break;
            case '-':
                new_mode &= ~perm_bits;
                break;
            case '=':
                new_mode &= ~clear_mask;
                new_mode |= perm_bits;
                break;
            default:
                throw std::invalid_argument("Invalid operator '" + std::string(1, operation) + "'");
        }
    }
    return chmod(input_file.c_str(), new_mode);
}
