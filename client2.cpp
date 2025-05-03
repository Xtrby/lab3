#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace boost::asio;
using ip::tcp;

int main() {
    try {
        io_context io;
        tcp::socket socket(io);
        tcp::resolver resolver(io);

        boost::asio::connect(socket, resolver.resolve("127.0.0.1", "12345"));

        std::cout << "Введите числа через пробел: ";
        std::string input;
        std::getline(std::cin, input);

        std::istringstream iss(input);
        std::vector<int> numbers;
        int num;
        while (iss >> num) {
            numbers.push_back(num);
        }

        if (numbers.empty()) {
            std::cerr << "Ошибка: введены некорректные данные" << std::endl;
            return 1;
        }

        std::string message = input + "\n";
        write(socket, buffer(message));

        streambuf response;
        read_until(socket, response, '\n');
        std::istream stream(&response);
        std::string server_response;
        std::getline(stream, server_response);

        std::cout << "Результат: " << server_response << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
    }
    return 0;
}