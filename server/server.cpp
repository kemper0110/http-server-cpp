#include "Server.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <istream>
#include "Request.h"
#include "Controller.h"
#include "StaticMiddleware.h"
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

namespace asio = boost::asio;
using namespace boost::system;
using tcp = asio::ip::tcp;

Server::Server() {

}

void Server::run() {
	asio::io_context ioc;
	asio::co_spawn(ioc, listen_and_serve(), asio::detached);
	ioc.run();
}

/*
GET / HTTP/1.1
Host: 127.0.0.1
User-Agent: curl/8.0.1
Accept: ..

*/

inline asio::awaitable<void> Server::serve(tcp::socket socket) {
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
			const auto [ec, read_n] = co_await asio::async_read_until(socket, request_, "\r\n\r\n", asio::as_tuple(asio::use_awaitable));
			if (ec) {
				//logger << ec.message() << '\n';
				//std::cout << id << "# read err: " << ec.message() << '\n';
				break;
			}
		}
		std::istream request_stream(&request_);


		Request request;
		request_stream >> request.method >> request.path >> request.http_version;

		request.headers.reserve(50);
		std::string header;
		std::getline(request_stream, header); // read last "\r"
		while (std::getline(request_stream, header) && header != "\r") {
			const auto delim = header.find(": ");
			auto value = std::string(header.cbegin() + delim + 2, header.cend() - 1);
			header.resize(delim);
			request.headers.emplace(std::move(header), std::move(value));
		}
		//std::cout << id << '#' << request.method << ' ' << request.path << ' ' << request.http_version << '\n';

		asio::streambuf response_;
		std::ostream response_stream(&response_);

		//if (staticMiddleware.handle(request, response_stream)) {
		controller.handle(request, response_stream);
		//}

		{
			//std::cout << id << '#' << " responsing\n";
			const auto [ec, write_n] = co_await asio::async_write(socket, response_, asio::as_tuple(asio::use_awaitable));
			if (ec) {
				//logger << ec.message() << '\n';
				//std::cout << id << "# write err: " << ec.message() << '\n';
				break;
			}
			//std::cout << id << '#' << " responsed: " << write_n << '\n';
		}
		if (
			request.http_version == "HTTP/1.0" and request.headers["Connection"] != "keep-alive"
			or
			request.http_version == "HTTP/1.1" and request.headers["Connection"] == "close"
			)
		{
			//std::cout << id << "# not keep alive, closing\n";
			break;
		}
	}
	//const auto [ec] = co_await socket.async_wait(tcp::socket::wait_error, asio::as_tuple(asio::use_awaitable));
	socket.shutdown(tcp::socket::shutdown_both);

	//std::cout << "connection #" << id << " closed\n";
}

inline asio::awaitable<void> Server::listen_and_serve() {
	const auto ioc = co_await asio::this_coro::executor;

	tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 80));
	for (;;) {
		auto socket = co_await acceptor.async_accept(asio::use_awaitable);
		asio::co_spawn(ioc, serve(std::move(socket)), asio::detached);
	}
}
