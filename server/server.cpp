#include <sdkddkver.h>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

namespace asio = boost::asio;
using namespace boost::system;
using namespace boost::asio::experimental::awaitable_operators;
using tcp = asio::ip::tcp;


asio::awaitable<void> serve(tcp::socket socket) {
	std::string read_buffer;
	auto offset = 0;
	const auto [ec, read_n]= co_await asio::async_read_until(socket, asio::dynamic_buffer(read_buffer), "\r\n", asio::as_tuple(asio::use_awaitable));
	std::string_view start_line = std::string_view(read_buffer.begin(), read_buffer.begin() + read_n);
	std::cout << start_line;
	offset += read_n;

	std::unordered_map<std::string, std::string> headers;
	while (true) {
		const auto [ec, read_n] = co_await asio::async_read_until(socket, asio::dynamic_buffer(read_buffer), "\r\n", asio::as_tuple(asio::use_awaitable));
		const auto headline = read_buffer.substr(offset, read_n);
		std::cout << headline;
		offset += read_n;
	}
	socket.close();
}

asio::awaitable<void> listen_and_serve() {
	const auto ioc = co_await asio::this_coro::executor;

	for (;;) {
		tcp::acceptor acceptor(ioc, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 80));
		auto socket = co_await acceptor.async_accept(asio::use_awaitable);
		asio::co_spawn(ioc, serve(std::move(socket)), asio::detached);
	}
}

int main()
{
	asio::io_context ioc;
	asio::co_spawn(ioc, listen_and_serve(), asio::detached);
	ioc.run();
}