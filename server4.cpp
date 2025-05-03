#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>

using namespace boost::asio;
using ip::tcp;

class ThreadSafeLog {
public:
    void add(const std::string& entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_.push_back(entry);
        std::cout << "LOG: " << entry << std::endl;
    }

private:
    std::vector<std::string> log_;
    std::mutex mutex_;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, io_context::strand& strand, ThreadSafeLog& log)
        : socket_(std::move(socket)), strand_(strand), log_(log) {}

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
            boost::asio::bind_executor(
                strand_,
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (ec) {
                        log_.add("Ошибка чтения: " + ec.message());
                        return;
                    }
                    
                    std::istream stream(&buffer_);
                    std::string data;
                    std::getline(stream, data);
                    process_request(data);
                }
            )
        );
    }

    void process_request(const std::string& data) {
        auto self(shared_from_this());
        
        std::thread([this, self, data]() {
            int number = 0;
            try {
                number = std::stoi(data);
            } catch (...) {
                send_response("Ошибка: некорректный ввод");
                return;
            }

            uint64_t result = 1;
            for (int i = 2; i <= number; ++i) result *= i;

            boost::asio::post(
                strand_,
                [this, self, result]() {
                    log_.add("Вычислен факториал " + std::to_string(result));
                    send_response("Факториал: " + std::to_string(result));
                }
            );
        }).detach();
    }

    void send_response(const std::string& response) {
        auto self(shared_from_this());
        async_write(
            socket_, 
            buffer(response + "\n"),
            boost::asio::bind_executor(
                strand_,
                [this, self](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        log_.add("Ошибка отправки: " + ec.message());
                    } else {
                        read_data();
                    }
                }
            )
        );
    }

    tcp::socket socket_;
    io_context::strand& strand_;
    ThreadSafeLog& log_;
    streambuf buffer_;
};

class Server {
public:
    Server(io_context& io, short port, int threads)
        : io_(io),
          acceptor_(io, tcp::endpoint(tcp::v4(), port)),
          strand_(io),
          log_(),
          threads_count_(threads) 
    {
        accept();
    }

    void run() {
        for (int i = 0; i < threads_count_; ++i) {
            threads_.emplace_back([this]() { io_.run(); });
        }
        for (auto& t : threads_) t.join();
    }

private:
    void accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket), strand_, log_)->start();
                }
                accept();
            }
        );
    }

    io_context& io_;
    tcp::acceptor acceptor_;
    io_context::strand strand_;
    ThreadSafeLog log_;
    std::vector<std::thread> threads_;
    int threads_count_;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <потоки>\n";
        return 1;
    }

    try {
        io_context io;
        Server server(io, 12345, std::stoi(argv[1]));
        std::cout << "Сервер запущен. Потоков: " << argv[1] << "\n";
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}