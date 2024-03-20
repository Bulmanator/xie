#if !defined(XI_VULKAN_H_)

#define RENDERER_BACKEND 1
typedef struct VK_Context RendererBackend;

#include <xi/xi.h>

#if OS_WIN32
    #define VK_USE_PLATFORM_WIN32_KHR 1
#elif OS_LINUX
    #define VK_USE_PLATFORM_XLIB_KHR    1
    #define VK_USE_PLATFORM_WAYLAND_KHR 1
#endif

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

#define VK_MAX_INSTANCE_EXTENSIONS 4
#define VK_NUM_FRAMES 2
#define VK_NUM_IMAGES 3 // triple-buffer
#define VK_MAX_IMAGES 8

typedef struct VK_Context VK_Context;

typedef struct VK_Buffer VK_Buffer;
struct VK_Buffer {
    VkBuffer handle;
    VkDeviceMemory memory;

    B32 host_mapped;

    VkBufferUsageFlags usage;

    U64   size;
    void *data;
};

typedef struct VK_Image VK_Image;
struct VK_Image {
    VkImage handle;
    VkDeviceMemory memory;

    // we use texture2DArray in shader which means the VkImageView must be of type 2D_ARRAY, so for
    // now as a hack this will force the creation of the image view to be an array even if the
    // image itself isn't an array for non-sprite images
    //
    B32 force_array;

    VkImageView view;

    VkImageLayout layout;
};

// If the implementation of the command pool/command buffers wasn't insane we wouldn't have to
// do this, they got it right with the descriptor pool how did they screw this up ???
//
typedef struct VK_CommandBufferNode VK_CommandBufferNode;
struct VK_CommandBufferNode {
    VK_CommandBufferNode *next;
    VK_CommandBufferNode *next_submit;

    VkCommandBuffer handle;
};

typedef struct VK_Frame VK_Frame;
struct VK_Frame {
    VK_Frame *next;

    VK_CommandBufferNode *free;
    VK_CommandBufferNode *allocated;
    VK_CommandBufferNode *submitted;

    VkSemaphore acquire; // wait on for swapchain image acquire
    VkSemaphore render;  // wait on for present complete

    // This is the view of the current status of the sprite array, we have one per frame but maybe
    // we could transition to a single global one due to the transition barriers waiting for execution
    // to complete
    //
    VkImageView sprite_view;

    VkCommandBuffer _cmds; // these will go away
    VkCommandBuffer _transfer;

    VkFence fence; // this should go in VK_CommandBufferNode so you can wait on individual command buffers

    U32 image;

    VkDescriptorPool descriptor_pool;
    VkCommandPool    command_pool;
};

typedef struct VK_Queue VK_Queue;
struct VK_Queue {
    VkQueue handle;
    U32 family;
};

typedef struct VK_PipelineState VK_PipelineState;
struct VK_PipelineState {
    VkPrimitiveTopology topology;

    VkCullModeFlags cull_mode;

    VkBool32    depth_test;
    VkBool32    depth_write;
    VkCompareOp depth_compare_op;

    VkPipelineColorBlendAttachmentState blend;
};

typedef struct VK_Shader VK_Shader;
struct VK_Shader {
    VkShaderModule handle;

    // other things will be in here after parsing the shader code to get resources
    //
};

typedef struct VK_Pipeline VK_Pipeline;
struct VK_Pipeline {
    VkPipeline handle;

    VkPipelineLayout layout;
    VkDescriptorSetLayout set_layout;

    VK_PipelineState state;

    U32 num_targets;
    VkFormat target_formats[4];
    VkFormat depth_format;

    U32 num_shaders;
    VK_Shader shaders[4];
};

typedef struct VK_Swapchain VK_Swapchain;
struct VK_Swapchain {
    VkSwapchainKHR handle;
    VkSurfaceKHR surface;

    VkFormat surface_format;

    VkExtent2D extent;

    VkPresentModeKHR vsync_off;
    VkPresentModeKHR mode;

    B32 vsync;

    U32 num_images;
    VkImage     *images;
    VkImageView *views;

    VK_Image depth;

    void *platform; // window data for creating the surface
};

typedef struct VK_Device VK_Device;
struct VK_Device {
    VkPhysicalDevice physical;
    VkDevice handle;

    VK_Queue queue; // combined graphics-present

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VK_Swapchain swapchain;

    VK_Buffer vertex_buffer;
    VK_Buffer index_buffer;
    VK_Buffer transfer_buffer;

    U32 max_sprite_index;
    VK_Image sprite_array;

    U32 image_limit;
    VK_Image *images;

    VkSampler sampler;

    VK_Pipeline pipeline;

    VK_Frame   *frame;
    VK_Context *vk;

};

struct VK_Context {
    RendererContext *renderer;

    M_Arena *arena;

    // basic loader
    //
    void *lib;
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;

    U32 num_instance_extensions;
    const char *instance_extensions[VK_MAX_INSTANCE_EXTENSIONS];

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VK_Device device;

    #define VK_GLOBAL_FUNCTIONS
    #define VK_DEBUG_FUNCTIONS
    #define VK_INSTANCE_FUNCTIONS
    #define VK_DEVICE_FUNCTIONS

    #define VK_DYN_FUNCTION(x) PFN_vk##x x
    #include "xi_vulkan_func.h"
};

FileScope VK_Context *VK_ContextCreate();

FileScope void VK_BufferCreate(VK_Device *device, VK_Buffer *buffer);
FileScope void VK_ImageCreate(VK_Device *device, VK_Image *image, VkImageCreateInfo *create_info);
FileScope void VK_ShaderModuleCreate(VK_Device *device, VK_Shader *shader, Str8 code);
FileScope void VK_PipelineCreate(VK_Device *device, VK_Pipeline *pipeline);

FileScope B32 VK_SurfaceCreate(VK_Device *device, VK_Swapchain *swapchain);
FileScope B32 VK_SwapchainCreate(VK_Device *device, VK_Swapchain *swapchain);

#endif  // XI_VULKAN_H_
