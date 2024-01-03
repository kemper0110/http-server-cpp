#pragma once
#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>

class Server {
public:
    Server();

    void run();

private:
    boost::asio::awaitable<void> serve(boost::asio::ip::tcp::socket socket);

    boost::asio::awaitable<void> listen_and_serve();

    size_t connection_id = 0;
    int file;
    size_t length;
};

