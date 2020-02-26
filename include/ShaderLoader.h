#pragma once
#include <vector>
#include <string>
class CShaderLoader {
public:
	static std::vector<char> readFile(const std::string& filename);
};
