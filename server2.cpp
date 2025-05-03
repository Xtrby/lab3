#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <memory>

using namespace boost::asio;
using ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        read_data();
    }

private:
    void read_data() {
        auto self(shared_from_this());
        async_read_until(
            socket_, 
            buffer_, 
            '\n',
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (ec) return;
                
                std::istream stream(&buffer_);
                std::string data;
                std::getline(stream, data);
                
                process_request(data);
            }
        );
    }

    void process_request(const std::string& data) {
        std::istringstream iss(data);
        std::vector<int> numbers;
        int num;
        while (iss >> num) numbers.push_back(num);

        std::string response;
        if (numbers.empty()) {
            response = "Ошибка: нет чисел\n";
        } else {
            int max = *std::max_element(numbers.begin(), numbers.end());
            response = "Максимум: " + std::to_string(max) + "\n";
        }

        async_write(
            socket_, 
            buffer(response),
            [this](boost::system::error_code ec, std::size_t) {
                if (ec) std::cerr << "Ошибка отправки: " << ec.message() << std::endl;
            }
        );
    }

    tcp::socket socket_;
    streambuf buffer_;
};

class Server {
public:
    Server(io_context& io, short port) 
        : acceptor_(io, tcp::endpoint(tcp::v4(), port)) 
    {
        accept();
    }

private:
    void accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                accept(); 
            }
        );
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        io_context io;
        Server server(io, 12345);
        std::cout << "Сервер запущен. Ожидание ввода...\n";
        io.run();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}