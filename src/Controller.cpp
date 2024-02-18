#include "Controller.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>

std::string Controller::get(const Request &request) {
//    std::cout << "requested: " << request.path << '\n';
    std::ostringstream oss;
    oss << '{';
    for (const auto &[key, value]: request.headers)
        oss << std::quoted(key) << ':' << std::quoted(value) << ',';
    oss << R"("cringe": "value"})";
    return oss.str();
}

std::string Controller::handle(const Request &request) {
    //std::string body = "Hello world";
//	response << "HTTP/1.0 200 OK\r\n";
    //response << "Content-Length: " << body.length() << "\r\n";
//	response << "\r\n";
    //response << body;

    size_t predicted_id = -1;
    switch (request.method[0]) {
        case 'G':
            predicted_id = 0;
            break;
        case 'P':
            switch (request.method[1]) {
                case 'O':
                    predicted_id = 1;
                    break;
                case 'U':
                    predicted_id = 2;
                    break;
                case 'A':
                    predicted_id = 3;
                    break;
            }
            break;
        case 'D':
            predicted_id = 4;
            break;
        case 'H':
            predicted_id = 5;
            break;
        case 'O':
            predicted_id = 6;
            break;
    }

    const auto &variant = variants.at(predicted_id);
//    std::cout << variant.method_name << '\n';
    if (variant.method_name == request.method) {
        return std::invoke(variant.method_ptr, this, request);
    }
    return "";
}

std::string Controller::post(const Request &request) {
    return get(request);
}
//
//void Controller::put(const Request &request, std::ostream &response) {
//    get(request,response);
//}
//
//void Controller::patch(const Request &request, std::ostream &response) {
//    get(request,response);
//}
//
//void Controller::del(const Request &request, std::ostream &response) {
//    get(request,response);
//}
//
//void Controller::head(const Request &request, std::ostream &response) {
//    get(request,response);
//}
//
//void Controller::options(const Request &request, std::ostream &response) {
//    get(request,response);
//}
