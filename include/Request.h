#pragma once
#include <string>
#include <unordered_map>

struct Request
{
	std::string method, path, http_version;
	std::unordered_map<std::string, std::string> headers;
//    boost::container::small_vector<std::pair<std::string, std::string>, 50> headers;
	std::string body;
};

