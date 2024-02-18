#pragma once
#include "Request.h"
#include <array>

class Controller
{
public:
	std::string handle(const Request& request);

private:
	std::string get(const Request& request);
	std::string post(const Request& request);
//	std::string put(const Request& request);
//	std::string patch(const Request& request);
//	std::string del(const Request& request);
//	std::string head(const Request& request);
//	std::string options(const Request& request);

    using handler_t = std::string (Controller::*)(const Request &);

    struct variant {
        const char *method_name;
        handler_t method_ptr;
    };

    inline static const std::array<variant, 2> variants = std::array{
            variant{"GET", &Controller::get},
            variant{"POST", &Controller::post},
//            variant{"PUT", &Controller::put},
//            variant{"PATCH", &Controller::patch},
//            variant{"DELETE", &Controller::del},
//            variant{"HEAD", &Controller::head},
//            variant{"OPTIONS", &Controller::options},
    };
};

