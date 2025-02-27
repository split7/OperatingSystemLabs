#include <iostream>
#include <algorithm>

// --std=c++26 -Wall -Wextra -Wpedantic -Werror
/*
    Переписать введенную пользователем строку, заменив пробелы, на запятые.
 */

const std::string ABOUT_MESSAGE = "This program replaces spaces with commas.\n";
const std::string CONTINUE_MESSAGE = "Do you want to continue? (y/n) >> ";
const std::string INCORRECT_MESSAGE = "I don't understand you, sorry. Please, try again.\n";
const std::string INPUT_MESSAGE = "Enter a string: \n";

void uncorrect() {
    std::cin.clear();
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << INCORRECT_MESSAGE;
}

char valid_continue() {
    std::cout << CONTINUE_MESSAGE;
    std::string answer;
    std::getline(std::cin, answer);
    while (answer != "y" && answer != "n" && answer != "Y" && answer != "N") {
        uncorrect();
        std::cout << CONTINUE_MESSAGE;
        std::getline(std::cin, answer);
    }
    return static_cast<char>(std::tolower(answer[0]));
}

int main() {
    char answer;
    std::cout << ABOUT_MESSAGE;
    do {
        std::cout << INPUT_MESSAGE;
        std::string str;
        std::getline(std::cin, str);
        /*
        size_t count = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == ' ') {
                str[i] = '.';
                ++count;
            }
        }
        */
        std::ranges::replace(str, ' ', '.');
        std::cout << str << std::endl;
        //std::cout << "Число заменённых элементов: " << count << std::endl;
        answer = valid_continue();
    } while (answer == 'y');
    return 0;
}
