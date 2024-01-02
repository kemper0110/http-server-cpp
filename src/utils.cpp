#include "utils.h"
#include <sstream>
#include <fstream>


std::string read_file_content(const std::string& name) {
	return (std::ostringstream() << std::ifstream(name, std::ios::binary).rdbuf()).str();
}