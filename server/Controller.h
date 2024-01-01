#pragma once
#include "Request.h"

class Controller
{
public:
	void handle(const Request& request, std::ostream& response);

private:
	void get(const Request& request, std::ostream& response);
};

