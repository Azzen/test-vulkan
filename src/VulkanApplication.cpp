#include <VulkanApplication.h>
#include <ShaderLoader.h>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <set>
#include <algorithm>

#define std_err(str) (std::runtime_error(str))
// Fonction permettant de cr�er un VkDebugUtilsMessengerEXT
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pCallback) {
	const auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
		instance, "vkCreateDebugUtilsMessengerEXT"));
	if (fn != nullptr) { return fn(instance, pCreateInfo, pAllocator, pCallback); }
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// Fonction permettant de d�truire le messager.
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   const VkDebugUtilsMessengerEXT callback,
                                   const VkAllocationCallbacks* pAllocator) {
	const auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (fn != nullptr) { fn(instance, callback, pAllocator); }
}

void CVulkanApplication::run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void CVulkanApplication::initWindow() {
	// Initialisation de GLFW sans cr�er un contexte OpenGL
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// La fen�tre ne peut pas �tre redimensionn�e
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Cr�ation de la fen�tre
	m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
}

void CVulkanApplication::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}

void CVulkanApplication::mainLoop() {
	// Tant que l'�v�nement "fermer la fen�tre" n'est pas appel�, �couter les �v�nements
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(m_device);
}

void CVulkanApplication::cleanup() {
	cleanupSwapChain();
	// Destruction des sync objects
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHTS; i++) {
		vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
	}
	// Destruction de la commandpool
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	// Destruction du logical device
	vkDestroyDevice(m_device, nullptr);
	// Destruction du messenger si l'extension est pr�sente
	if (enableValidationLayers) { DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr); }
	// Destruction de la surface KHR
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	// Destruction de l'instance Vulkan
	vkDestroyInstance(m_instance, nullptr);
	// Destruction la fen�tre quand l'�v�nement "fermer" a �t� appel�.
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void CVulkanApplication::drawFrame() {
	vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_device, m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
	auto submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	if(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
		throw std_err("Failed to send a command buffer");
	}
	auto presentInfo = VkPresentInfoKHR{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	// Signal que la pr�sentation peut se d�rouler
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	// Swap chain qui pr�sentera les images
	VkSwapchainKHR swapChains[] = { m_swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	//presentInfo.pResults = nullptr;
	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHTS;
}

void CVulkanApplication::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Validations are enabled but not available.");
	}
	// Structure sur les informations de l'application
	auto appInfo = VkApplicationInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	/*
	* Structure permettant d'informer le drivers des extensions que l'app va utiliser
	* ainsi que des validation layers de mani�re globale.
	*/
	auto createInfo = VkInstanceCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	// R�cup�ration des extensions
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	// Validation layers
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		createInfo.ppEnabledLayerNames = validation_layers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)& debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	// Cr�ation de l'instance
	if (vkCreateInstance(&createInfo, nullptr, &m_instance)) {
		throw std::runtime_error("Failed to create VkInstance!");
	}
}

void CVulkanApplication::createSurface() {
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}

void CVulkanApplication::createSwapChain() {
	const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
	const auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	const auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	const auto extent = chooseSwapExtent(swapChainSupport.capabilities);
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	auto createInfo = VkSwapchainCreateInfoKHR{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	// Indique le nombre de couche que chaque image poss�de
	createInfo.imageArrayLayers = 1;
	// Sp�cifie le type d'op�ration appliqu� sur les images ici le rendu est directement sur les images alors on prend color attachment
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	auto indices = findQueueFamilies(m_physicalDevice);
	uint32_t queueFamilyIndices[] = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};
	/*
	* Indique comment les images seront utilis�s dans le cas ou plusieurs queue seront � l'origine des op�rations
	* VK_SHARING_MODE_EXCLUSIVE : image accessible que par une queue a la fois
	* VK_SHARING_MODE_CONCURRENT : image accessible par plusieurs queue en m�me temps
	*/
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	// Applique une transformation (lin�aire) sur une image
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	// Canal alpha doit �tre m�lang� avec les couleurs des autres fen�tre ? (non)
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	// la swapchain peut devenir invalide apr�s certains events (window resize etc.) et doit �tre recr��e pour le moment pas besoin d'handle cela.
	createInfo.oldSwapchain = nullptr;
	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

void CVulkanApplication::createLogicalDevice() {
	/*
	 * L'application a besoin de plusieurs struct vkDeviceQueueCreateInfo, une pour chaque queueFamily, on
	 * utilise alors un set contenant tous les indices des queues et un vector pour les struct
	 */
	auto indices = findQueueFamilies(m_physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};
	/*
	* Vulkan permet de cr�er les commandes buffers depuis plusieurs threads
	* et les soumettre a la queue d'un seul coup sur le main thread sans perte de performance
	* Il permet donc d'assigner des priorit�s au queue � l'aide des floats compris dans l'intervalle ]0.0;1.0[
	*/
	auto queuePriority{1.0f};
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		auto queueCreateInfo = VkDeviceQueueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	// D�finition des fonctionnalit�s du physical device qu'on souhaite utiliser
	auto deviceFeatures = VkPhysicalDeviceFeatures{};
	// Cr�ation du logical device
	auto createInfo = VkDeviceCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	// Activation des extensions
	createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	createInfo.ppEnabledExtensionNames = device_extensions.data();
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		createInfo.ppEnabledLayerNames = validation_layers.data();
	}
	else { createInfo.enabledLayerCount = 0; }
	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a logical device");
	}
	vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void CVulkanApplication::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0) { throw std::runtime_error("No graphic cards support Vulkan"); }
	auto devices = std::vector<VkPhysicalDevice>{deviceCount};
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	std::multimap<int, VkPhysicalDevice> candidates;
	// Scoring de tous les physical devices
	for (const auto& device : devices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}
	// R�cup�ration du meilleur GPU s'il existe
	if (candidates.rbegin()->first > 0) { m_physicalDevice = candidates.rbegin()->second; }
	else { throw std::runtime_error("No GPU can run this program"); }
}

void CVulkanApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void CVulkanApplication::setupDebugMessenger() {
	if constexpr (!enableValidationLayers) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger");
	}
}

void CVulkanApplication::recreateSwapChain() {
	// Attendre que plus rien ne se passe pour ne pas perturber les ressourcse en cours d'utilisation
	vkDeviceWaitIdle(m_device);
	// Cleanup de la swapchain
	cleanupSwapChain();
	// Recr�ation de la swapchain
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandBuffers();
}

void CVulkanApplication::cleanupSwapChain() {
	for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
		vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
	}
	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	for (auto& imageView : m_swapChainImagesViews) {
		vkDestroyImageView(m_device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}


void CVulkanApplication::createImageViews() {
	m_swapChainImagesViews.resize(m_swapChainImages.size());
	for (size_t i = 0; i < m_swapChainImages.size(); i++) {
		auto createInfo = VkImageViewCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;
		// Possibilit� d'alt�rer les canaux de couleurs
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// D�crit l'utilisation de l'image et quelle(s) partie(s) de l'images devraient �tre acc�d�e(s).
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImagesViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create ImageView");
		}
	}
}

void CVulkanApplication::createGraphicsPipeline() {
	auto vertShaderCode = CShaderLoader::readFile("shaders/vert.spv");
	auto fragShaderCode = CShaderLoader::readFile("shaders/frag.spv");
	auto vertShaderModule = createShaderModule(vertShaderCode);
	auto fragShaderModule = createShaderModule(fragShaderCode);
	// Configuration des sommets
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
	// Entr�e des sommets
	auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	// Input Assembly (nature de la g�om�trie)
	auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	// View port (r�gion du framebuffer sur laquelle le rendu sera effectu�)
	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapChainExtent.width);
	viewport.height = static_cast<float>(m_swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	// Scissor rectangle
	auto scissor = VkRect2D{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapChainExtent;
	// Config viewport et scissors
	auto viewportState = VkPipelineViewportStateCreateInfo{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	// Rasterizer
	auto rasterizer = VkPipelineRasterizationStateCreateInfo{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;
	// Multisampling (= Anti aliasing)
	auto multisampling = VkPipelineMultisampleStateCreateInfo{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	// Tests de profondeurs et pochoir
	auto depthStencil = nullptr;
	// Color blending Attachement
	auto colorBlendAttachment = VkPipelineColorBlendAttachmentState{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
			| VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	// Color blending 
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;
	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
	// Cr�ation de la pipeline graphique
	auto pipelineInfo = VkGraphicsPipelineCreateInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;
	if (vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphic pipeline");
	}
	// Destruction des shader modules
	vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void CVulkanApplication::createRenderPass() {
	// D�finition des attachements de couleurs
	auto colorAttachment = VkAttachmentDescription{};
	colorAttachment.format = m_swapChainImageFormat;
	// Pas de multisampling donc un seul sample
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// D�fini ce qui doit �tre fait avec les donn�es de l'attachement avant et apr�s le rendu
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Remplace le contenu par une constante
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Le rendu est gard� en m�moire et accessible plus tard
	// D�fini ce qui doit �tre fait avec les donn�es de stencil (vu qu'on en a pas dans l'app on "DONT_CARE")
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// D�finition de l'organisation des pixels en m�moire
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // le format de l'image pr�c�dente ne nous int�resse pas
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image pr�sent�e � une swap chain
	// Subpasse
	auto colorAttachmentRef = VkAttachmentReference{};
	colorAttachmentRef.attachment = 0; // R�f�rence vers un index d'un tableau contenant les attachments
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // color buffer
	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	// Cr�ation du sous passe de rendu
	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// Cr�ation du passe de rendu
	auto renderPassInfo = VkRenderPassCreateInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void CVulkanApplication::createFramebuffers() {
	m_swapChainFramebuffers.resize(m_swapChainImagesViews.size());
	for (size_t i = 0; i < m_swapChainImagesViews.size(); i++) {
		VkImageView attachments[] = {m_swapChainImagesViews[i]};
		auto framebufferInfo = VkFramebufferCreateInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a framebuffer");
		}
	}
}

void CVulkanApplication::createCommandPool() {
	const auto queueFamilyIndices = findQueueFamilies(m_physicalDevice);
	auto poolInfo = VkCommandPoolCreateInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;
	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a command pool");
	}
}

void CVulkanApplication::createCommandBuffers() {
	// Allocation des commands buffers
	m_commandBuffers.resize(m_swapChainFramebuffers.size());
	auto allocInfo = VkCommandBufferAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; /* peut �tre envoy� a une queue pour y etre ex�cut� mais ne peut pas �tre appel� par d'autre cmdbuf */
	allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();
	if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate a command buffer");
	}
	// D�but des enregistrments des command buffers
	for (size_t i = 0; i < m_commandBuffers.size(); i++) {
		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Peut �tre renvoy� alors qu'il est en cours d'ex�cution
		beginInfo.pInheritanceInfo = nullptr;
		if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std_err("Failed to begin a command buffer");
		}
		// D�but de la render pass
		auto renderPassInfo = VkRenderPassBeginInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
		// D�finissent la taille du rendu
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;
		auto clearColor = VkClearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		// Activation de la pipeline graphique
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		// Affichage du triangle
		vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
		// Fin de l'affichage
		vkCmdEndRenderPass(m_commandBuffers[i]);
		if(vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
			throw std_err("Failed to end a command a buffer");
		}
	}

}

void CVulkanApplication::createSyncObjects() {
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHTS);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHTS);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHTS);
	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHTS; i++) {
		if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i])) {
			throw std_err("Failed to create syncronization objects");
		}
	}

}

std::vector<const char*> CVulkanApplication::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	// pointeur pointant sur un pointeur qui pointe un const char
	const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	auto extensions = std::vector<const char*>{glfwExtensions, glfwExtensions + glfwExtensionCount};
	if (enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
	return extensions;
}

int CVulkanApplication::rateDeviceSuitability(const VkPhysicalDevice device) {
	// R�cup�ration des fonctions de bases (version Vulkan support�e)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	// R�cup�ration des features (VR, float64 etc.)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	auto score{0};
	// Si c'est une carte graphique d�di�e on ajoute 1000 au score.
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { score += 1000; }
	// Taille max des textures affecte le score
	score += deviceProperties.limits.maxImageDimension2D;
	// Extensions support�es ?
	const auto extensionsSupported = checkDeviceExtensionSupport(device);
	// Swap chain ad�quate ?
	auto swapChainAdequate{false};
	if (extensionsSupported) {
		const auto swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	// L'application ne peut fonctionner sans les geometryShader ou la queue VK_QUEUE_GRAPHICS_BIT donc on retourne un score de 0
	if (!deviceFeatures.geometryShader || !findQueueFamilies(device).isComplete()
		|| !extensionsSupported
		|| !swapChainAdequate) { return 0; }
	std::cout << "Scored " << std::to_string(score) << " for device: " << deviceProperties.deviceName << std::endl;
	return score;
}

bool CVulkanApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	auto availableExtensions = std::vector<VkExtensionProperties>{extensionCount};
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());
	for (const auto& extension : availableExtensions) { requiredExtensions.erase(extension.extensionName); }
	return requiredExtensions.empty();
}

bool CVulkanApplication::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	auto availableLayers = std::vector<VkLayerProperties>{layerCount};
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validation_layers) {
		auto layerFound{false};
		for (auto const& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) { return false; }
	}

	return true;
}

QueueFamilyIndices CVulkanApplication::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	auto queueFamilies = std::vector<VkQueueFamilyProperties>{queueFamilyCount};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	/*
	* V�rification que la carte graphique puisse supporter les op�rations sur une queue VK_QUEUE_GRAPHICS_BIT
	* V�rificaiton que la carte graphique supporte supporter les op�ration de pr�sentation (KHR)
	*/
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport{false};
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport) { indices.presentFamily = i; }
		if (indices.isComplete()) { break; }
		i++;
	}
	return indices;
}

SwapChainSupportDetails CVulkanApplication::querySwapChainSupport(VkPhysicalDevice device) const {
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}
	return details;
}

VkSurfaceFormatKHR CVulkanApplication::
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace ==
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { return availableFormat; }
	}
	return availableFormats[0];
}

VkPresentModeKHR CVulkanApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) { return availablePresentMode; }
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D CVulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};
	actualExtent.width = std::max(capabilities.minImageExtent.width,
	                            std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height,
	                               std::min(capabilities.maxImageExtent.height, actualExtent.height));
	return actualExtent;
}

VkShaderModule CVulkanApplication::createShaderModule(const std::vector<char>& code) const {
	auto createInfo = VkShaderModuleCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a shader module");
	}
	return shaderModule;
}

VKAPI_ATTR VkBool32 VKAPI_CALL CVulkanApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                 const VkDebugUtilsMessengerCallbackDataEXT*
                                                                 pCallbackData,
                                                                 void* pUserdata) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}
