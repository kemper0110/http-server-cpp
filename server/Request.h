#pragma once
#include <string>
#include <unordered_map>

struct Request
{
	std::string method, path, http_version;
	std::unordered_map<std::string, std::string> headers;
	std::string body;
};

