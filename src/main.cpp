#include <VulkanApplication.h>
#include <iostream>

int main() {
	auto app = CVulkanApplication{};
	try {
		app.run();
	}
	catch (std::exception const& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
