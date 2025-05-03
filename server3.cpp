#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <memory>

using namespace boost::asio;
using ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)), timer_(socket_.get_executor()) {}

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
                read_data(); 
            }
        );
    }

    void process_request(const std::string& data) {
        if (data.find("timer ") == 0) { 
            try {
                int delay = std::stoi(data.substr(6));
                if (delay < 0) throw std::invalid_argument("Отрицательное время");
                
                send_response("Ready in " + std::to_string(delay) + " sec");
                
                timer_.expires_after(boost::asio::chrono::seconds(delay));
                timer_.async_wait(
                    [this, self = shared_from_this()](const boost::system::error_code& ec) {
                        if (!ec) send_response("Done!");
                    }
                );
            } catch (...) {
                send_response("Ошибка: некорректное время");
            }
        } else {
            send_response("Неизвестная команда");
        }
    }

    void send_response(const std::string& response) {
        auto self(shared_from_this());
        async_write(
            socket_, 
            buffer(response + "\n"),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) std::cerr << "Ошибка отправки: " << ec.message() << std::endl;
            }
        );
    }

    tcp::socket socket_;
    streambuf buffer_;
    steady_timer timer_;
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
        std::cout << "Сервер запущен.\n";
        io.run();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}