#include <boost/asio.hpp>
#include <iostream>
#include <algorithm>
#include <sstream>

using namespace boost::asio;
using ip::tcp;

int main() {
    try {
        const int port = 12345;
        io_context io;

        tcp::endpoint endpoint(ip::make_address("127.0.0.1"), port);
        tcp::acceptor acceptor(io, endpoint);
        acceptor.set_option(tcp::acceptor::reuse_address(true));

        std::cout << "Сервер запущен на 127.0.0.1:" << port << "\n";
        std::cout << "Ожидание подключения\n";

        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);

            boost::asio::streambuf buffer;
            read_until(socket, buffer, '\n');
            
            std::istream stream(&buffer);
            std::string message;
            std::getline(stream, message);

            std::string upper = message;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            std::string response = std::to_string(message.size()) + ": " + upper + "\n";

            boost::asio::write(socket, boost::asio::buffer(response));
            std::cout << "Отправлен ответ: " << response;
        }

    } catch (const std::exception& e) {
        std::cerr << "Ошибка сервера: " << e.what() << std::endl;
    }
    return 0;
}