#include "Server.h"
#include <iostream>
#include <unordered_map>
#include <istream>
#include "Request.h"
#include "Controller.h"
#include "StaticMiddleware.h"
#include "sendfile.h"
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

struct count_matcher {
    const long long max_count;
    long long count = 0;

    template<typename Iterator>
    std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) {
        Iterator i = begin;
        while (i != end) {
            ++i;
            if (++count == max_count) {
                return std::make_pair(i, true);
            }
        }
        return std::make_pair(i, false);
    }
};

namespace boost::asio {
    template<>
    struct is_match_condition<count_matcher>
            : public boost::true_type {
    };
} // namespace asio


namespace asio = boost::asio;
using namespace boost::system;
using tcp = asio::ip::tcp;

Server::Server() {

    const auto filename = "../assets/img/toro.jpg";
//        const auto filename = "../assets/index.html";
//    const auto filename = "../assets/vkvia.html";

    struct stat st;
    stat(filename, &st);
    this->length = st.st_size;

    this->file = open(filename, O_RDONLY);
}

void Server::run() {
    const auto thread_count = 4;

    asio::io_context ioc(thread_count);

    asio::co_spawn(ioc, listen_and_serve(), asio::detached);

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&ioc] {
            ioc.run();
        });
    }
    for (auto &th: threads)
        th.join();
}

/*
GET / HTTP/1.1
Host: 127.0.0.1
User-Agent: curl/8.0.1
Accept: ..

*/

inline asio::awaitable<void> Server::serve(tcp::socket socket) {
    const auto ioc = co_await asio::this_coro::executor;
    const auto id = connection_id++;
//    std::cout << "new connection #" << id << '\n';
    auto response_count = 0;

    //auto logger = std::ofstream(std::string("errors") + std::to_string(id) + ".txt");

    Controller controller;
    StaticMiddleware staticMiddleware;

    using namespace asio::experimental::awaitable_operators;
    while (socket.is_open()) {
//        std::cout << "serving connection #" << id << '\t' << "already served requests: " << (response_count++) << "\n";
//        boost::asio::deadline_timer wait_timer(ioc, boost::posix_time::seconds(30));

        asio::streambuf request_;
//        std::cout << "start wait " << socket.is_open() << '\n';

//        const auto [ec, n] = co_await asio::async_read(socket, request_, asio::transfer_at_least(1),asio::as_tuple(asio::use_awaitable));
//        std::cout << "end wait, start read " << socket.is_open() << '\n';
//
//        if(ec) {
//            std::cout << "wait with n=" << n << ' ' << ec.what() << '\n';
//            break;
//        }
        read_again:
        const auto [header_ec, header_size] = co_await asio::async_read_until(socket, request_, "\r\n\r\n",asio::as_tuple(asio::use_awaitable));
        if (header_ec) {
            if (header_ec == asio::error::eof) {
                goto read_again;
            } else {
                std::cout << "header_ec: " << header_ec.what() << '\n';
                break;
            }
        }
//        if (ec) {
//            if (ec == asio::error::eof) {
//                goto wait_input;
//            } else {
//                //            logger << ec.message() << '\n';
//                std::cout << id << "# read err: " << ec.message() << '\n';
//                break;
//            }
//        }
        std::istream request_stream(&request_);


        Request request;
        request_stream >> request.method >> request.path >> request.http_version;

        request.headers.reserve(24);
        std::string header;
        header.reserve(128);
        std::getline(request_stream, header); // read last "\r"
        while (true) {
            header.reserve(128);
            if (!std::getline(request_stream, header) || header == "\r")
                break;
            const auto delim = header.find(": ");
            auto value = std::string(header.cbegin() + delim + 2, header.cend() - 1);
            header.resize(delim);
            request.headers.emplace(std::move(header), std::move(value));
        };
        std::cout << id << '#' << request.method << ' ' << request.path << ' ' << request.http_version << '\n';

        const auto content_length = request.headers.find("Content-Length");

        if (content_length != request.headers.end()) {
            const auto body_size = std::stoll(content_length->second);
            const auto [ec, n] = co_await asio::async_read_until(socket, request_,
                                                                 count_matcher{body_size},
                                                                 asio::as_tuple(asio::deferred));
            if (ec) {
                std::cout << ec.what() << '\n';
                break;
            }
            request.body = std::string(std::istreambuf_iterator<char>{request_stream}, {});
//            std::cout << "body: " << request.body << '\n' << request.body.length() << " <=> " << content_length->second
//                      << std::endl;
        }

        asio::streambuf response_;
        std::ostream response_stream(&response_);

        const auto response_body = [&staticMiddleware, &controller, &request]() {
            auto file_string = staticMiddleware.handle(request);
            if (!file_string.empty())
                return std::move(file_string);
            else
                return controller.handle(request);
        }();

        std::ostringstream oss;

        response_stream << "HTTP/1.1 200 OK\r\n";
        response_stream << "Content-Length: " << response_body.length() << "\r\n";


        bool close_after_write = false;
        const auto connection = request.headers.find("Connection");
        if (
                request.http_version == "HTTP/1.0" and
                (connection == request.headers.end() or connection->second != "Keep-Alive")
                or
                request.http_version == "HTTP/1.1" and connection != request.headers.end() and
                connection->second == "Close"
                ) {
//            std::cout << id << "# not keep alive, close_after_write\n";
            response_stream << "Connection: Close\r\n";
            close_after_write = true;
        } else {
//            std::cout << id << "# is keep alive, reusing connection\n";
            response_stream << "Connection: Keep-Alive\r\n";
        }

        response_stream << "\r\n";
        response_stream << response_body;

        {
            const auto [ec, n] = co_await asio::async_write(socket, response_, asio::as_tuple(asio::deferred));
            if (ec) {
                std::cout << ec.what();
                break;
            }
        }

        if (close_after_write)
            break;
    }
    socket.shutdown(tcp::socket::shutdown_both);

    std::cout << "connection #" << id << " closed\n";
}

inline asio::awaitable<void> Server::listen_and_serve() {
    try {
        const auto ioc = co_await asio::this_coro::executor;

        tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 2085));
        for (;;) {
            auto socket = co_await acceptor.async_accept(asio::deferred);
            asio::co_spawn(ioc, serve(std::move(socket)), asio::detached);
        }
    } catch (const boost::system::system_error &err) {
        std::cout << err.what();
    }
}
