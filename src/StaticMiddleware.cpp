#include "StaticMiddleware.h"
#include <iostream>
#include <filesystem>
#include "utils.h"


//        response_stream << "HTTP/1.1 200 OK\r\n";
//        response_stream << "Content-Length: " << length << "\r\n";
//        response_stream << "\r\n";
//
//        {
//            const auto [ec, write_n] = co_await asio::async_write(socket, response_,
//                                                                  asio::as_tuple(asio::deferred));
//            if (ec) {
//                std::cout << id << "# write err: " << ec.message() << '\n';
//                break;
//            }
//        }
//
//        co_await async_sendfile(socket, file, 0, length);

std::string StaticMiddleware::handle(const Request& request) {
	if (request.method != "GET") 
		return "";

	std::string path = "../";
	if (request.path == "/")
		path += "assets/index.html";
	else
		path += "assets" + request.path;

//	std::cout << "trying find static path: " << path << '\n';
	const auto static_exists = std::filesystem::exists(path);

	if (!static_exists) {
//		std::cout << "file not found\n";
		return "";
	}

//	std::cout << "file found\n";
    return read_file_content(path);
}
