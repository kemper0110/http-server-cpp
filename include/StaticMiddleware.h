#pragma once
#include "Request.h"
#include <ostream>


class StaticMiddleware
{
public:
	std::string handle(const Request& request);
};

