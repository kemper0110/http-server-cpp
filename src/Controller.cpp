#include "Controller.h"
#include <iostream>
#include <sstream>
#include <iomanip>

void Controller::get(const Request& request, std::ostream& response)
{
	std::cout << "requested: " << request.path << '\n';

	std::ostringstream oss;
	oss << '{';
	for (const auto& [key, value] : request.headers)
		oss << std::quoted(key) << ':' << std::quoted(value) << ',';
	oss << R"("cringe": "value"})";
	
	std::string body = oss.str();

	response << "HTTP/1.1 200 OK\r\n";
	response << "Content-Length: " << body.length() << "\r\n";
	response << "\r\n";
	response << body;
}

void Controller::handle(const Request& request, std::ostream& response)
{
	//std::string body = "Hello world";
	response << "HTTP/1.0 200 OK\r\n";
	//response << "Content-Length: " << body.length() << "\r\n";
	response << "\r\n";
	//response << body;

	//if (request.method == "GET") {
		//this->get(request, response);
	//}
}
