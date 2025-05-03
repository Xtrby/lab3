#include <boost/asio.hpp>
#include <iostream>
#include <sstream>

using namespace boost::asio;
using ip::tcp;

int main() {
    try {
        io_context io;
        tcp::socket socket(io);
        tcp::resolver resolver(io);

        boost::asio::connect(socket, resolver.resolve("127.0.0.1", "12345"));
        std::cout << "Введите сообщение: ";

        std::string message;
        std::getline(std::cin, message);
        message += "\n";
        write(socket, buffer(message));

        boost::asio::streambuf response;
        read_until(socket, response, '\n');
        
        std::istream stream(&response);
        std::string server_response;
        std::getline(stream, server_response);
        std::cout << "Ответ сервера: " << server_response << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
    }
    return 0;
}