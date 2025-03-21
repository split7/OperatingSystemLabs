#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>

struct CalcData {
    long double x;
    int N;
    long double epsilon;
};

// Неименнованые каналы связи in и out
int pipe_in[2];
int pipe_out[2];
pid_t pid;
void help() {
    std::cout << "Справка:\n"
              << "Чтобы посчитать приближённое значение функции, запустите программу без ключей\n";
}

long double read_double(const std::string &msg, bool positive = false) {
    long double result;
    bool flag = true;
    while (flag) {
        std::cout << msg;
        std::cin >> result;
        if (std::cin.fail() || std::cin.peek() != '\n' || (positive && result <= 0.0)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "> Ошибка ввода, попробуйте еще раз\n";
        } else {
            flag = false;
        }
    }
    return result;
}
int read_int(const std::string &msg) {
    int result;
    bool flag = true;
    while (flag) {
        std::cout << msg;
        std::cin >> result;
        if (result <= 0 || std::cin.fail() || std::cin.peek() != '\n') {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "> Ошибка ввода, попробуйте еще раз\n";
        } else {
            flag = false;
        }
    }
    return result;
}

long double calculate(long double x, int N, long double epsilon) {
    long double result = sin(1); // sin(x^0)/0!
    long double power = 1;
    long double factorial = 1;
    for (int n = 1; n <= N; n++) {
        power *= x;
        factorial *= n+1;
        if (long double prec = 1.0 / factorial; result + prec == result || prec < epsilon) {
            break;
        }
        long double cur = std::sin(power) / factorial;
        result += cur;
    }
    return result;
}

/*
//Клиентская часть программы
//Принимает данные: левую границу, правую границу и количество прямо-
угольников
//Записывает и читает в/из неименованных каналов
//Выводит результат вычислений
*/

void frontend() {
    CalcData data{};
    data.x = read_double("Введите значение переменной x: ");
    data.N = read_int("Введите число слагаемых: ");
    data.epsilon = read_double("Введите значение погрешности, до которой будет проводиться суммирование: ", true);
    write(pipe_in[1], &data, sizeof(CalcData));
    long double result;
    read(pipe_out[0], &result, sizeof(long double));
    std::cout << "Результат: " << std::setprecision(10) << result << std::endl;
    exit(0);
}
void frontend(const std::string &input_file, const std::string &output_file) {
    CalcData data{};
    std::ifstream input(input_file, std::ios::binary);
    if (!input.is_open()) {
        throw std::invalid_argument("frontend: could not open input file");
    }
    std::ofstream output(output_file, std::ios::binary);
    if (!output.is_open()) {
        input.close();
        throw std::invalid_argument("frontend: could not open output file");
    }
    input >> data.x;
    if (input.fail() || input.peek() != '\n') {
        output.close();
        input.close();
        throw std::invalid_argument("frontend: could not parse input file");
    }
    input >> data.N;
    if (input.fail() || input.peek() != '\n') {
        output.close();
        input.close();
        throw std::invalid_argument("frontend: could not parse input file");
    }
    input >> data.epsilon;
    if (input.fail() || input.peek() != EOF) {
        output.close();
        input.close();
        throw std::invalid_argument("frontend: could not parse input file");
    }
    input.close();
    write(pipe_in[1], &data, sizeof(CalcData));
    long double result;
    read(pipe_out[0], &result, sizeof(long double));
    output << std::setprecision(10) << result << std::endl;
    output.close();
    std::cout << "Результат: "<< std::setprecision(10) << result << std::endl;
    exit(0);
}

/*
//Серверная часть программы
//Производит вычисления по данным, переданным клиентской частью по не-
именованному каналу
//Записывает результат вычисления в неименованный канал
*/
void backend() {
    CalcData data;
    read(pipe_in[0], &data, sizeof(CalcData));
    long double result = calculate(data.x, data.N, data.epsilon);
    write(pipe_out[1], &result, sizeof(long double));
}
int main(int argc, char const *argv[]) {
    setlocale(LC_ALL, 0);
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        help();
        exit(0);
    }
    std::cout << "Программа для нахождения приближённого значения функции одной переменной\n";
    pipe(pipe_in);
    pipe(pipe_out);
    pid = fork();
    if (pid < 0) {
        std::cerr << "Критическая ошибка! Форк не создан" << std::endl;
        exit(1);
    }
    if (argc == 4 && !strcmp(argv[1], "--file")) {
        if (pid > 0) {
            frontend(argv[2], argv[3]);
        } else {
            backend();
        }
    } else if (argc != 1) {
        std::cerr << "Неверные аргументы." << std::endl;
        exit(1);
    } else {
        if (pid > 0) {
            frontend();
        } else {
            backend();
        }
    }
    for (int i = 0; i < 2; ++i) {
        close(pipe_in[i]);
        close(pipe_out[i]);
    }
    return 0;
}
