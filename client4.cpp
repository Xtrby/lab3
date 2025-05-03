#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio;
using ip::tcp;

int main() {
    try {
        io_context io;
        tcp::socket socket(io);
        tcp::resolver resolver(io);

        connect(socket, resolver.resolve("127.0.0.1", "12345"));
        std::cout << "Введите число: ";
        
        std::string message;
        std::getline(std::cin, message);
        message += "\n";
        write(socket, buffer(message));

        streambuf response;
        read_until(socket, response, '\n');
        std::istream stream(&response);
        std::string server_response;
        std::getline(stream, server_response);
        std::cout << "Ответ: " << server_response << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}