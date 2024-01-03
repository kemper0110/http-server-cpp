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
    //std::cout << "new connection #" << id << '\n';
    auto response_count = 0;

    //auto logger = std::ofstream(std::string("errors") + std::to_string(id) + ".txt");

    Controller controller;
    StaticMiddleware staticMiddleware;
    while (socket.is_open()) {
        //std::cout << "serving connection #" << id << '\t' << "already served requests: " << (response_count++) << "\n";

        asio::streambuf request_;
        {
            const auto [ec, read_n] = co_await asio::async_read_until(socket, request_, "\r\n\r\n",
                                                                      asio::as_tuple(asio::use_awaitable));
            if (ec) {
                //logger << ec.message() << '\n';
                //std::cout << id << "# read err: " << ec.message() << '\n';
                break;
            }
        }
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
        //std::cout << id << '#' << request.method << ' ' << request.path << ' ' << request.http_version << '\n';

        asio::streambuf response_;
        std::ostream response_stream(&response_);

        response_stream << "HTTP/1.1 200 OK\r\n";
        response_stream << "Content-Length: " << length << "\r\n";
        response_stream << "\r\n";

        {
            const auto [ec, write_n] = co_await asio::async_write(socket, response_,
                                                                  asio::as_tuple(asio::deferred));
            if (ec) {
                std::cout << id << "# write err: " << ec.message() << '\n';
                break;
            }
        }

        co_await async_sendfile(socket, file, 0, length);

        const auto connection = request.headers.find("Connection");
        if (
                request.http_version == "HTTP/1.0" and
                (connection == request.headers.end() or connection->second != "keep-alive")
                or
                request.http_version == "HTTP/1.1" and connection != request.headers.end() and
                connection->second == "close"
                ) {
            //std::cout << id << "# not keep alive, closing\n";
            break;
        }
    }
    socket.shutdown(tcp::socket::shutdown_both);

    //std::cout << "connection #" << id << " closed\n";
}

inline asio::awaitable<void> Server::listen_and_serve() {
    try {
        const auto ioc = co_await asio::this_coro::executor;

        tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 2085));
        for (;;) {
            auto socket = co_await acceptor.async_accept(asio::use_awaitable);
            asio::co_spawn(ioc, serve(std::move(socket)), asio::detached);
        }
    } catch (const boost::system::system_error &err) {
        std::cout << err.what();
    }
}
