#pragma once
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

const int WINDOW_HEIGHT{600};
const int WINDOW_WIDTH{800};
const int MAX_FRAMES_IN_FLIGHTS = 2;

// Activation des validations layers en fonction du mode de compilation (release/debug)
const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	[[nodiscard]]
	bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class CVulkanApplication {
public:
	void run();
private:
	/*************************
	 	Membres
	**************************/

	/*
	 * Fenêtre GLFW
	 */
	GLFWwindow* m_window;

	/*
	 * Instance de Vulkan
	 */
	VkInstance m_instance;

	/*
	 * Messager
	 * Utilitaire qui permet de diagnostiquer et de debugger le programme de façon efficace
	 */
	VkDebugUtilsMessengerEXT m_debugMessenger;

	/*
	 * Carte graphique
	 */
	VkPhysicalDevice m_physicalDevice{nullptr};

	/*
	 * Device logique : interface faisant le lien entre la carte graphique et l'application
	 */
	VkDevice m_device;

	/*
	 * Références aux queues
	 */
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	/*
	 * Surfaces
	 */
	VkSurfaceKHR m_surface;

	/*
	 * Swapchain
	 */
	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkFramebuffer> m_swapChainFramebuffers;
	/*
	 * ImageView: permet de manipuler les VkImage
	 */
	std::vector<VkImageView> m_swapChainImagesViews;

	/*
	 * Pipeline layout
	 */
	VkPipelineLayout m_pipelineLayout;

	/*
	 * Passe de rendu
	 */
	VkRenderPass m_renderPass;

	/*
	 * Pipeline graphique
	 */
	VkPipeline m_pipeline;

	/*
	 * Pool de commandes
	 */
	VkCommandPool m_commandPool;

	/*
	 * Command buffers
	 */
	std::vector<VkCommandBuffer> m_commandBuffers;

	/*
	 * Sémaphores (synchronisation des opérations d'affichage)
	 */
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;

	/*
	 * Frame courante
	 */
	size_t m_currentFrame{ 0 };

	/*
	 * Fences (sync CPU-GPU)
	 */
	std::vector<VkFence> m_inFlightFences;


	/*************************
		Méthodes
	**************************/


	/*
	* Initialisation de la fenêtre GLFW
	*/
	void initWindow();

	/*
	* Initialisation de Vulkan
	*/
	void initVulkan();

	/*
	* Boucle indéfiniment tant que le programme est actif.
	* s'occupe d'écouter les évènements sur la fenêtre GLFW
	*/
	void mainLoop();

	/*
	* Désallocation de mémoire lorsque l'application se ferme
	*/
	void cleanup();

	/*
	 * Rend la frame courante
	 */
	void drawFrame();

	/*
	* Créer une instance Vulkan
	* Fourni les informations à Vulkan pour le driver afin d'optimiser les performances ainsi que diagnostiquer
	* les erreurs lors de l'exécution
	*/
	void createInstance();

	/*
	* Créer une surface (géré par GLFW)
	*/
	void createSurface();

	/*
	* Créer la swapchain
	*/
	void createSwapChain();

	/*
	* Créer le logical device
	*/
	void createLogicalDevice();

	/*
	* Choisi la carte graphique qui va servir à afficher l'application
	*/
	void pickPhysicalDevice();

	/*
	* Peuple le contenu de la struct du messager
	*/
	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	/*
	* Configure le messager
	*/
	void setupDebugMessenger();

	/*
	 * Recréation de la swap chain (resize etc.)
	 */
	void recreateSwapChain();

	/*
	 * Cleanup de la swapchain avant recréation
	 */
	void cleanupSwapChain();

	/*
	* Créer les image views
	*/
	void createImageViews();

	/*
	* Créer la pipeline graphique.
	*/
	void createGraphicsPipeline();

	/*
	* Créer le passe de rendu.
	*/
	void createRenderPass();

	/*
	* Créer les framebuffers.
	*/
	void createFramebuffers();

	/*
	 * Créer les pools de commandes (opérations d'affichage et de transfert mémoire)
	 */
	void createCommandPool();

	/*
	 * Créer les commandbuffers
	 */
	void createCommandBuffers();

	/*
	 * Créer les objets de sync (sémaphores et fences)
	 */
	void createSyncObjects();

	/*
	* Récupère les extensions requises par l'application
	*/
	static std::vector<const char*> getRequiredExtensions();

	/*
	* Attribue un score aux cartes graphiques en fonction des extensions, fonctionnalités supportés
	*/
	int rateDeviceSuitability(VkPhysicalDevice device);

	/*
	* Vérifie si la carte graphique possède toutes les extensions requises par l'application
	*/
	static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	/*
	* Vérifie si l'ordinateur supporte les validations layer.
	*/
	static bool checkValidationLayerSupport();

	/*
	* Recherche et renvoie les queues families que l'application requiert
	*/
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	/*
	* Récupération des détails du support de la swapchain de la carte graphique
	*/
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

	/*
	* Défini quel format va être utilisé parmis ceux disponible (sRGB)
	*/
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	/*
	* Défini quel mode de présentation va être utilisé parmi ceux disponible, en l'occurrence : vsync (VK_PRESENT_MODE_FIFO_KHR)
	*/
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	/*
	* Défini les dimensions de l'affichage
	*/
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	/*
	* Création d'un VkShaderModule
	*/
	[[nodiscard]]
	VkShaderModule createShaderModule(const std::vector<char>& code) const;

	/*
	* Fonction de rappel des erreurs, permet que dès une erreur est attrapée par les validations layer de l'envoyer dans la console.
	*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserdata);
};
