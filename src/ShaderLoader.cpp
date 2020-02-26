#include <ShaderLoader.h>
#include <fstream>
#include <stdexcept>
#include <iostream>

using namespace std::string_literals;

std::vector<char> CShaderLoader::readFile(const std::string& filename) {
	auto file = std::ifstream{ filename, std::ios::ate | std::ios::binary };
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: "s + filename);
	}
	const size_t fileSize = static_cast<size_t>(file.tellg());
	std::cout << "[Shader Loader] Opened file: "s + filename << " of size: " << fileSize << std::endl;

	auto buffer = std::vector<char>(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}
