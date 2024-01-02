#pragma once
#include "Request.h"
#include <ostream>


class StaticMiddleware
{
public:
	bool handle(const Request& request, std::ostream& response);
};

