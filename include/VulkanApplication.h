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
	 * Fen�tre GLFW
	 */
	GLFWwindow* m_window;

	/*
	 * Instance de Vulkan
	 */
	VkInstance m_instance;

	/*
	 * Messager
	 * Utilitaire qui permet de diagnostiquer et de debugger le programme de fa�on efficace
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
	 * R�f�rences aux queues
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
	 * S�maphores (synchronisation des op�rations d'affichage)
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
		M�thodes
	**************************/


	/*
	* Initialisation de la fen�tre GLFW
	*/
	void initWindow();

	/*
	* Initialisation de Vulkan
	*/
	void initVulkan();

	/*
	* Boucle ind�finiment tant que le programme est actif.
	* s'occupe d'�couter les �v�nements sur la fen�tre GLFW
	*/
	void mainLoop();

	/*
	* D�sallocation de m�moire lorsque l'application se ferme
	*/
	void cleanup();

	/*
	 * Rend la frame courante
	 */
	void drawFrame();

	/*
	* Cr�er une instance Vulkan
	* Fourni les informations � Vulkan pour le driver afin d'optimiser les performances ainsi que diagnostiquer
	* les erreurs lors de l'ex�cution
	*/
	void createInstance();

	/*
	* Cr�er une surface (g�r� par GLFW)
	*/
	void createSurface();

	/*
	* Cr�er la swapchain
	*/
	void createSwapChain();

	/*
	* Cr�er le logical device
	*/
	void createLogicalDevice();

	/*
	* Choisi la carte graphique qui va servir � afficher l'application
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
	 * Recr�ation de la swap chain (resize etc.)
	 */
	void recreateSwapChain();

	/*
	 * Cleanup de la swapchain avant recr�ation
	 */
	void cleanupSwapChain();

	/*
	* Cr�er les image views
	*/
	void createImageViews();

	/*
	* Cr�er la pipeline graphique.
	*/
	void createGraphicsPipeline();

	/*
	* Cr�er le passe de rendu.
	*/
	void createRenderPass();

	/*
	* Cr�er les framebuffers.
	*/
	void createFramebuffers();

	/*
	 * Cr�er les pools de commandes (op�rations d'affichage et de transfert m�moire)
	 */
	void createCommandPool();

	/*
	 * Cr�er les commandbuffers
	 */
	void createCommandBuffers();

	/*
	 * Cr�er les objets de sync (s�maphores et fences)
	 */
	void createSyncObjects();

	/*
	* R�cup�re les extensions requises par l'application
	*/
	static std::vector<const char*> getRequiredExtensions();

	/*
	* Attribue un score aux cartes graphiques en fonction des extensions, fonctionnalit�s support�s
	*/
	int rateDeviceSuitability(VkPhysicalDevice device);

	/*
	* V�rifie si la carte graphique poss�de toutes les extensions requises par l'application
	*/
	static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	/*
	* V�rifie si l'ordinateur supporte les validations layer.
	*/
	static bool checkValidationLayerSupport();

	/*
	* Recherche et renvoie les queues families que l'application requiert
	*/
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	/*
	* R�cup�ration des d�tails du support de la swapchain de la carte graphique
	*/
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

	/*
	* D�fini quel format va �tre utilis� parmis ceux disponible (sRGB)
	*/
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	/*
	* D�fini quel mode de pr�sentation va �tre utilis� parmi ceux disponible, en l'occurrence : vsync (VK_PRESENT_MODE_FIFO_KHR)
	*/
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	/*
	* D�fini les dimensions de l'affichage
	*/
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	/*
	* Cr�ation d'un VkShaderModule
	*/
	[[nodiscard]]
	VkShaderModule createShaderModule(const std::vector<char>& code) const;

	/*
	* Fonction de rappel des erreurs, permet que d�s une erreur est attrap�e par les validations layer de l'envoyer dans la console.
	*/
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserdata);
};
