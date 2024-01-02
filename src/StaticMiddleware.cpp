#include "StaticMiddleware.h"
#include <iostream>
#include <filesystem>
#include "utils.h"


bool StaticMiddleware::handle(const Request& request, std::ostream& response) {
	if (request.method != "GET") 
		return true;

	std::string path = "../";
	if (request.path == "/")
		path += "assets/index.html";
	else
		path += "assets" + request.path;

//	std::cout << "trying find static path: " << path << '\n';
	const auto static_exists = std::filesystem::exists(path);

	if (!static_exists) {
//		std::cout << "file not found\n";
		return true;
	}

//	std::cout << "file found\n";
	std::string body = read_file_content(path);

	response << "HTTP/1.1 200 OK\r\n";
	response << "Content-Length: " << body.length() << "\r\n";
	response << "\r\n";
	response << body;

	return false;
}
