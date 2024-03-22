#include "xi_vulkan.h"

#include <stdio.h>

GlobalVar const U32 base_vert_code[] = {
    // 1112.3.1
	0x07230203,0x00010000,0x0008000b,0x00000043,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000014,0x00000028,0x0000003c,
	0x0000003f,0x00030003,0x00000002,0x000001c2,0x00080004,0x455f4c47,0x735f5458,0x616c6163,
	0x6c625f72,0x5f6b636f,0x6f79616c,0x00007475,0x00040005,0x00000004,0x6e69616d,0x00000000,
	0x00040005,0x00000009,0x74726556,0x00337865,0x00040006,0x00000009,0x00000000,0x00000070,
	0x00040006,0x00000009,0x00000001,0x00007675,0x00040006,0x00000009,0x00000002,0x00000063,
	0x00030005,0x0000000b,0x00787476,0x00040005,0x0000000c,0x74726556,0x00337865,0x00040006,
	0x0000000c,0x00000000,0x00000070,0x00040006,0x0000000c,0x00000001,0x00007675,0x00040006,
	0x0000000c,0x00000002,0x00000063,0x00050005,0x0000000e,0x74726556,0x73656369,0x00000000,
	0x00060006,0x0000000e,0x00000000,0x74726576,0x73656369,0x00000000,0x00030005,0x00000010,
	0x00000000,0x00060005,0x00000014,0x565f6c67,0x65747265,0x646e4978,0x00007865,0x00060005,
	0x00000026,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000026,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x00000026,0x00000001,0x505f6c67,0x746e696f,
	0x657a6953,0x00000000,0x00070006,0x00000026,0x00000002,0x435f6c67,0x4470696c,0x61747369,
	0x0065636e,0x00070006,0x00000026,0x00000003,0x435f6c67,0x446c6c75,0x61747369,0x0065636e,
	0x00030005,0x00000028,0x00000000,0x00040005,0x0000002b,0x65535f52,0x00707574,0x00060006,
	0x0000002b,0x00000000,0x77656976,0x6f72705f,0x0000006a,0x00060006,0x0000002b,0x00000001,
	0x656d6163,0x705f6172,0x00000000,0x00050006,0x0000002b,0x00000002,0x656d6974,0x00000000,
	0x00040006,0x0000002b,0x00000003,0x00007464,0x00060006,0x0000002b,0x00000004,0x646e6977,
	0x645f776f,0x00006d69,0x00040005,0x0000002d,0x75746573,0x00000070,0x00040005,0x0000003c,
	0x67617266,0x0076755f,0x00050005,0x0000003f,0x67617266,0x6c6f635f,0x0072756f,0x00050048,
	0x0000000c,0x00000000,0x00000023,0x00000000,0x00050048,0x0000000c,0x00000001,0x00000023,
	0x0000000c,0x00050048,0x0000000c,0x00000002,0x00000023,0x00000018,0x00040047,0x0000000d,
	0x00000006,0x0000001c,0x00040048,0x0000000e,0x00000000,0x00000018,0x00050048,0x0000000e,
	0x00000000,0x00000023,0x00000000,0x00030047,0x0000000e,0x00000003,0x00040047,0x00000010,
	0x00000022,0x00000000,0x00040047,0x00000010,0x00000021,0x00000000,0x00040047,0x00000014,
	0x0000000b,0x0000002a,0x00050048,0x00000026,0x00000000,0x0000000b,0x00000000,0x00050048,
	0x00000026,0x00000001,0x0000000b,0x00000001,0x00050048,0x00000026,0x00000002,0x0000000b,
	0x00000003,0x00050048,0x00000026,0x00000003,0x0000000b,0x00000004,0x00030047,0x00000026,
	0x00000002,0x00040048,0x0000002b,0x00000000,0x00000004,0x00050048,0x0000002b,0x00000000,
	0x00000023,0x00000000,0x00050048,0x0000002b,0x00000000,0x00000007,0x00000010,0x00050048,
	0x0000002b,0x00000001,0x00000023,0x00000040,0x00050048,0x0000002b,0x00000002,0x00000023,
	0x00000050,0x00050048,0x0000002b,0x00000003,0x00000023,0x00000054,0x00050048,0x0000002b,
	0x00000004,0x00000023,0x00000058,0x00030047,0x0000002b,0x00000002,0x00040047,0x0000003c,
	0x0000001e,0x00000000,0x00040047,0x0000003f,0x0000001e,0x00000001,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
	0x00000006,0x00000003,0x00040015,0x00000008,0x00000020,0x00000000,0x0005001e,0x00000009,
	0x00000007,0x00000007,0x00000008,0x00040020,0x0000000a,0x00000007,0x00000009,0x0005001e,
	0x0000000c,0x00000007,0x00000007,0x00000008,0x0003001d,0x0000000d,0x0000000c,0x0003001e,
	0x0000000e,0x0000000d,0x00040020,0x0000000f,0x00000002,0x0000000e,0x0004003b,0x0000000f,
	0x00000010,0x00000002,0x00040015,0x00000011,0x00000020,0x00000001,0x0004002b,0x00000011,
	0x00000012,0x00000000,0x00040020,0x00000013,0x00000001,0x00000011,0x0004003b,0x00000013,
	0x00000014,0x00000001,0x00040020,0x00000016,0x00000002,0x0000000c,0x00040020,0x0000001a,
	0x00000007,0x00000007,0x0004002b,0x00000011,0x0000001d,0x00000001,0x0004002b,0x00000011,
	0x00000020,0x00000002,0x00040020,0x00000021,0x00000007,0x00000008,0x00040017,0x00000023,
	0x00000006,0x00000004,0x0004002b,0x00000008,0x00000024,0x00000001,0x0004001c,0x00000025,
	0x00000006,0x00000024,0x0006001e,0x00000026,0x00000023,0x00000006,0x00000025,0x00000025,
	0x00040020,0x00000027,0x00000003,0x00000026,0x0004003b,0x00000027,0x00000028,0x00000003,
	0x00040018,0x00000029,0x00000023,0x00000004,0x00040017,0x0000002a,0x00000008,0x00000002,
	0x0007001e,0x0000002b,0x00000029,0x00000023,0x00000006,0x00000006,0x0000002a,0x00040020,
	0x0000002c,0x00000009,0x0000002b,0x0004003b,0x0000002c,0x0000002d,0x00000009,0x00040020,
	0x0000002e,0x00000009,0x00000029,0x0004002b,0x00000006,0x00000033,0x3f800000,0x00040020,
	0x00000039,0x00000003,0x00000023,0x00040020,0x0000003b,0x00000003,0x00000007,0x0004003b,
	0x0000003b,0x0000003c,0x00000003,0x0004003b,0x00000039,0x0000003f,0x00000003,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x0000000a,
	0x0000000b,0x00000007,0x0004003d,0x00000011,0x00000015,0x00000014,0x00060041,0x00000016,
	0x00000017,0x00000010,0x00000012,0x00000015,0x0004003d,0x0000000c,0x00000018,0x00000017,
	0x00050051,0x00000007,0x00000019,0x00000018,0x00000000,0x00050041,0x0000001a,0x0000001b,
	0x0000000b,0x00000012,0x0003003e,0x0000001b,0x00000019,0x00050051,0x00000007,0x0000001c,
	0x00000018,0x00000001,0x00050041,0x0000001a,0x0000001e,0x0000000b,0x0000001d,0x0003003e,
	0x0000001e,0x0000001c,0x00050051,0x00000008,0x0000001f,0x00000018,0x00000002,0x00050041,
	0x00000021,0x00000022,0x0000000b,0x00000020,0x0003003e,0x00000022,0x0000001f,0x00050041,
	0x0000002e,0x0000002f,0x0000002d,0x00000012,0x0004003d,0x00000029,0x00000030,0x0000002f,
	0x00050041,0x0000001a,0x00000031,0x0000000b,0x00000012,0x0004003d,0x00000007,0x00000032,
	0x00000031,0x00050051,0x00000006,0x00000034,0x00000032,0x00000000,0x00050051,0x00000006,
	0x00000035,0x00000032,0x00000001,0x00050051,0x00000006,0x00000036,0x00000032,0x00000002,
	0x00070050,0x00000023,0x00000037,0x00000034,0x00000035,0x00000036,0x00000033,0x00050091,
	0x00000023,0x00000038,0x00000030,0x00000037,0x00050041,0x00000039,0x0000003a,0x00000028,
	0x00000012,0x0003003e,0x0000003a,0x00000038,0x00050041,0x0000001a,0x0000003d,0x0000000b,
	0x0000001d,0x0004003d,0x00000007,0x0000003e,0x0000003d,0x0003003e,0x0000003c,0x0000003e,
	0x00050041,0x00000021,0x00000040,0x0000000b,0x00000020,0x0004003d,0x00000008,0x00000041,
	0x00000040,0x0006000c,0x00000023,0x00000042,0x00000001,0x00000040,0x00000041,0x0003003e,
	0x0000003f,0x00000042,0x000100fd,0x00010038
};

GlobalVar const U32 base_frag_code[] = {
    // 1112.3.1
	0x07230203,0x00010000,0x0008000b,0x0000001d,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000b,0x00000019,
	0x00030010,0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00080004,0x455f4c47,
	0x735f5458,0x616c6163,0x6c625f72,0x5f6b636f,0x6f79616c,0x00007475,0x00040005,0x00000004,
	0x6e69616d,0x00000000,0x00050005,0x00000009,0x6d617266,0x66756265,0x00726566,0x00050005,
	0x0000000b,0x67617266,0x6c6f635f,0x0072756f,0x00050005,0x0000000f,0x65745f75,0x72757478,
	0x00000065,0x00050005,0x00000013,0x61735f75,0x656c706d,0x00000072,0x00040005,0x00000019,
	0x67617266,0x0076755f,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000b,
	0x0000001e,0x00000001,0x00040047,0x0000000f,0x00000022,0x00000000,0x00040047,0x0000000f,
	0x00000021,0x00000002,0x00040047,0x00000013,0x00000022,0x00000000,0x00040047,0x00000013,
	0x00000021,0x00000001,0x00040047,0x00000019,0x0000001e,0x00000000,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
	0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,
	0x00000009,0x00000003,0x00040020,0x0000000a,0x00000001,0x00000007,0x0004003b,0x0000000a,
	0x0000000b,0x00000001,0x00090019,0x0000000d,0x00000006,0x00000001,0x00000000,0x00000001,
	0x00000000,0x00000001,0x00000000,0x00040020,0x0000000e,0x00000000,0x0000000d,0x0004003b,
	0x0000000e,0x0000000f,0x00000000,0x0002001a,0x00000011,0x00040020,0x00000012,0x00000000,
	0x00000011,0x0004003b,0x00000012,0x00000013,0x00000000,0x0003001b,0x00000015,0x0000000d,
	0x00040017,0x00000017,0x00000006,0x00000003,0x00040020,0x00000018,0x00000001,0x00000017,
	0x0004003b,0x00000018,0x00000019,0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,
	0x00000003,0x000200f8,0x00000005,0x0004003d,0x00000007,0x0000000c,0x0000000b,0x0004003d,
	0x0000000d,0x00000010,0x0000000f,0x0004003d,0x00000011,0x00000014,0x00000013,0x00050056,
	0x00000015,0x00000016,0x00000010,0x00000014,0x0004003d,0x00000017,0x0000001a,0x00000019,
	0x00050057,0x00000007,0x0000001b,0x00000016,0x0000001a,0x00050085,0x00000007,0x0000001c,
	0x0000000c,0x0000001b,0x0003003e,0x00000009,0x0000001c,0x000100fd,0x00010038
};

FileScope VkBool32 VK_DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                           VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                           void*                                       pUserData)
{
    (void) messageSeverity;
    (void) messageTypes;
    (void) pUserData;

    // @todo: logging proper
    //

    OutputDebugString("[VK][");

    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: { OutputDebugString("VRB] :: "); } break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    { OutputDebugString("INF] :: "); } break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: { OutputDebugString("WRN] :: "); } break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   { OutputDebugString("ERR] :: "); } break;
    }

    OutputDebugString(pCallbackData->pMessage);
    OutputDebugString("\n");

    return VK_FALSE;
}

FileScope RENDERER_SUBMIT(VK_Submit);

ExportFunc RENDERER_INIT(VK_Init) {
    B32 result = false;

    VK_Context *vk = VK_ContextCreate();
    if (vk) {
        vk->renderer      = renderer;
        renderer->backend = vk;
        renderer->Submit  = VK_Submit;

        // load global functions
        //
        #define VK_GLOBAL_FUNCTIONS
        #define VK_DYN_FUNCTION(x) vk->x = (PFN_vk##x) vk->GetInstanceProcAddr(0, Stringify(vk##x))
        #include "xi_vulkan_func.h"

        // create instance
        //
        {
            VkApplicationInfo app_info = { 0 };
            app_info.sType         = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.engineVersion = VK_MAKE_VERSION(XI_VERSION_MAJOR, XI_VERSION_MINOR, XI_VERSION_PATCH);
            app_info.pEngineName   = "XI Engine";
            app_info.apiVersion    = VK_API_VERSION_1_3;

            U32 num_layers = 0;
            const char *layers[] = { "VK_LAYER_KHRONOS_validation" };

            if (renderer->setup.debug) {
                num_layers = 1;
                vk->instance_extensions[vk->num_instance_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            }

            VkInstanceCreateInfo create_info = { 0 };
            create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            create_info.pApplicationInfo        = &app_info;
            create_info.enabledLayerCount       = num_layers;
            create_info.ppEnabledLayerNames     = layers;
            create_info.enabledExtensionCount   = vk->num_instance_extensions;
            create_info.ppEnabledExtensionNames = vk->instance_extensions;

            VkResult success = vk->CreateInstance(&create_info, 0, &vk->instance);
            if (success != VK_SUCCESS) { return result; }

            // load instance functions
            //
            #define VK_INSTANCE_FUNCTIONS
            #define VK_DYN_FUNCTION(x) vk->x = (PFN_vk##x) vk->GetInstanceProcAddr(vk->instance, Stringify(vk##x))
            #include "xi_vulkan_func.h"
        }

        // if requested, setup the debug utils extension
        //
        if (renderer->setup.debug) {
            #define VK_DEBUG_FUNCTIONS
            #define VK_DYN_FUNCTION(x) vk->x = (PFN_vk##x) vk->GetInstanceProcAddr(vk->instance, Stringify(vk##x))
            #include "xi_vulkan_func.h"

            VkDebugUtilsMessengerCreateInfoEXT create_info = { 0 };
            create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

            create_info.pfnUserCallback = VK_DebugMessageCallback;
            create_info.pUserData       = cast(void *) vk;

            VkResult success = vk->CreateDebugUtilsMessengerEXT(vk->instance, &create_info, 0, &vk->debug_messenger);
            if (success != VK_SUCCESS) { return result; }
        }

        // Select a physical device
        //
        {
            M_Arena *temp = M_TempGet();

            U32 num_devices = 0;
            vk->EnumeratePhysicalDevices(vk->instance, &num_devices, 0);

            VkPhysicalDevice *devices = M_ArenaPush(temp, VkPhysicalDevice, num_devices);
            vk->EnumeratePhysicalDevices(vk->instance, &num_devices, devices);

            if (num_devices == 0) { return result; }

            VkPhysicalDevice selected = devices[0]; // just fallback on the first device if nothing selected

            for (U32 it = 0; it < num_devices; ++it) {
                VkPhysicalDeviceProperties props;
                vk->GetPhysicalDeviceProperties(devices[it], &props);

                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    // We assume most people only have one gpu, all major vendors support combined
                    // graphics-present queue so we don't have to worry about figuring out support
                    //
                    selected = devices[it];
                    break;
                }
            }

            VK_Device *device = &vk->device;

            device->physical     = selected;
            device->queue.family = U32_MAX;

            vk->GetPhysicalDeviceProperties(device->physical, &device->properties);
            vk->GetPhysicalDeviceMemoryProperties(device->physical, &device->memory_properties);

            // Get queue properties
            //
            U32 num_queue_families = 0;
            vk->GetPhysicalDeviceQueueFamilyProperties(selected, &num_queue_families, 0);

            VkQueueFamilyProperties *props = M_ArenaPush(temp, VkQueueFamilyProperties, num_queue_families);
            vk->GetPhysicalDeviceQueueFamilyProperties(selected, &num_queue_families, props);

            for (U32 it = 0; it < num_queue_families; ++it) {
                VkQueueFamilyProperties *p = &props[it];
                if (p->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    device->queue.family = it;
                    break;
                }
            }

            if (device->queue.family == U32_MAX) { return result; }
        }

        // Create logical device
        //
        VK_Device *device = &vk->device;
        {
            F32 queue_priority = 1.0f;

            VkDeviceQueueCreateInfo queue_create = { 0 };
            queue_create.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create.queueFamilyIndex = device->queue.family;
            queue_create.queueCount       = 1;
            queue_create.pQueuePriorities = &queue_priority;

            const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

            // Enable any features we want, we should use the capabilities of the device for things
            // that we don't deem absolutly necessary, but this'll do for now
            //
            // Its mainly shader stuff anyway and if they don't support it there isn't much we can do I guess
            //
            VkPhysicalDeviceFeatures         features   = { 0 };
            VkPhysicalDeviceVulkan11Features features11 = { 0 };
            VkPhysicalDeviceVulkan12Features features12 = { 0 };
            VkPhysicalDeviceVulkan13Features features13 = { 0 };

            features.fillModeNonSolid = VK_TRUE;
            features.shaderInt16      = VK_TRUE;
            features.wideLines        = VK_TRUE;

            features11.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            features11.storageBuffer16BitAccess           = VK_TRUE;
            features11.uniformAndStorageBuffer16BitAccess = VK_TRUE;

            features12.sType                             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            features12.pNext                             = &features11;
            features12.shaderFloat16                     = VK_TRUE;
            features12.shaderInt8                        = VK_TRUE;
            features12.storageBuffer8BitAccess           = VK_TRUE;
            features12.uniformAndStorageBuffer8BitAccess = VK_TRUE;
            features12.scalarBlockLayout                 = VK_TRUE; // the best!

            features13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            features13.pNext            = &features12;
            features13.synchronization2 = VK_TRUE;
            features13.dynamicRendering = VK_TRUE;

            VkDeviceCreateInfo create_info = { 0 };
            create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.pNext                   = &features13;
            create_info.queueCreateInfoCount    = 1;
            create_info.pQueueCreateInfos       = &queue_create;
            create_info.enabledExtensionCount   = ArraySize(extensions);
            create_info.ppEnabledExtensionNames = extensions;

            VkResult success = vk->CreateDevice(device->physical, &create_info, 0, &device->handle);
            if (success != VK_SUCCESS) { return result; }

            #define VK_DEVICE_FUNCTIONS
            #define VK_DYN_FUNCTION(x) vk->x = (PFN_vk##x) vk->GetDeviceProcAddr(device->handle, Stringify(vk##x))
            #include "xi_vulkan_func.h"

            vk->GetDeviceQueue(device->handle, device->queue.family, 0, &device->queue.handle);

            device->vk = vk;
        }

        // Create immediate vertex and index buffers, one range per frame. we could instead use a circular
        // system to make this more efficient but the buffer sizes even for like 16k quads is tiny without
        // even compressing our vertex type so its whatever really
        //

        U32 num_vertices = renderer->vertices.limit;
        U32 num_indices  = renderer->indices.limit;

        // you can "misalign" the number of vertices/indices to the number of quads meaning you
        // will run out of one before the other, but thats up to the user
        //
        if (num_vertices == 0) { num_vertices = 65536; } // 16k quads * 4 vertices
        if (num_indices  == 0) { num_indices  = 98304; } // 16k quads * 6 indices

        // Buffer for vertices and indices
        //
        VK_Buffer *vbo = &device->vertex_buffer;

        vbo->size        = VK_NUM_FRAMES * num_vertices * sizeof(Vertex3);
        vbo->host_mapped = true;
        vbo->usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        VK_BufferCreate(device, vbo);

        // @speed: these could be rolled into one buffer!
        //

        VK_Buffer *ibo = &device->index_buffer;

        ibo->size        = VK_NUM_FRAMES * num_indices * sizeof(U16);
        ibo->host_mapped = true;
        ibo->usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VK_BufferCreate(device, ibo);

        renderer->vertices.limit = num_vertices;
        renderer->indices.limit  = num_indices;

        // Frame data, this is single threaded for now, the whole rendering system will be changed
        // at some point so not really any point in thinking too hard about it
        //
        {
            VK_Frame *frames = M_ArenaPush(vk->arena, VK_Frame, VK_NUM_FRAMES);

            for (U32 it = 0; it < VK_NUM_FRAMES; ++it) {
                VK_Frame *frame = &frames[it];
                VK_Frame *next  = &frames[(it + 1) % VK_NUM_FRAMES];

                frame->next = next;

                frame->imm.vertex_offset = (it * num_vertices) * sizeof(Vertex3);
                frame->imm.index_offset  = (it * num_indices)  * sizeof(U16);

                // Command pool per frame
                //
                {
                    VkCommandPoolCreateInfo create_info = { 0 };
                    create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    create_info.queueFamilyIndex = device->queue.family;

                    VkResult success = vk->CreateCommandPool(device->handle, &create_info, 0, &frame->command_pool);
                    if (success != VK_SUCCESS) { return result; }

                    VkCommandBuffer cmds[2];

                    VkCommandBufferAllocateInfo alloc_info = { 0 };
                    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    alloc_info.commandPool        = frame->command_pool;
                    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    alloc_info.commandBufferCount = 2;

                    success = vk->AllocateCommandBuffers(device->handle, &alloc_info, cmds);
                    if (success != VK_SUCCESS) { return result; }

                    frame->cmds     = cmds[0];
                    frame->transfer = cmds[1];
                }

                // Descriptor pool per frame
                //
                {
                    VkDescriptorPoolSize pool_sizes[3] = { 0 };

                    // Fixed shaders at the moment only use 1 sampler, 1 sampled image and 1 storage buffer
                    // meaning we can hardcode that amount. This will change heavily in the future!
                    //

                    pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_SAMPLER;
                    pool_sizes[0].descriptorCount = 1024;

                    pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    pool_sizes[1].descriptorCount = 1024;

                    pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    pool_sizes[2].descriptorCount = 1024;

                    VkDescriptorPoolCreateInfo create_info = { 0 };
                    create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                    create_info.maxSets       = 1024;
                    create_info.poolSizeCount = ArraySize(pool_sizes);
                    create_info.pPoolSizes    = pool_sizes;

                    VkResult success = vk->CreateDescriptorPool(device->handle, &create_info, 0, &frame->descriptor_pool);
                    if (success != VK_SUCCESS) { return result; }
                }

                // Create semaphores
                //
                {
                    VkSemaphoreCreateInfo create_info = { 0 };
                    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                    VkResult success = vk->CreateSemaphore(device->handle, &create_info, 0, &frame->acquire);
                    if (success != VK_SUCCESS) { return result; }

                    success = vk->CreateSemaphore(device->handle, &create_info, 0, &frame->render);
                    if (success != VK_SUCCESS) { return result; }
                }

                // Create fence
                //
                {
                    VkFenceCreateInfo create_info = { 0 };
                    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                    VkResult success = vk->CreateFence(device->handle, &create_info, 0, &frame->fence);
                    if (success != VK_SUCCESS) { return result; }
                }
            }

            // Set the frame to the first frame
            //
            device->frame = &frames[0];

            U8 *vertex_base = cast(U8 *) vbo->data;
            U8 *index_base  = cast(U8 *) ibo->data;

            renderer->vertices.base = cast(Vertex3 *) (vertex_base + device->frame->imm.vertex_offset);
            renderer->indices.base  = cast(U16     *) (index_base  + device->frame->imm.index_offset);
        }

        // Setup swapchain
        {
            VK_Swapchain *swapchain = &device->swapchain;

            swapchain->platform = platform;
            swapchain->vsync    = true;

            if (!VK_SwapchainCreate(device, swapchain)) {
                return result;
            }
        }

        RendererTransferQueue *transfer_queue = &renderer->transfer_queue;

        if (transfer_queue->task_limit == 0) {
            transfer_queue->task_limit = 256;
        }
        else {
            transfer_queue->task_limit = U32_Pow2Next(transfer_queue->task_limit);
        }

        if (transfer_queue->limit == 0) {
            transfer_queue->limit = MB(256);
        }

        // Staging buffer
        //
        VK_Buffer *tbo = &device->transfer_buffer;

        tbo->size        = transfer_queue->limit;
        tbo->host_mapped = true;
        tbo->usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VK_BufferCreate(device, tbo);

        transfer_queue->base  = cast(U8 *) tbo->data;
        transfer_queue->tasks = M_ArenaPush(vk->arena, RendererTransferTask, transfer_queue->task_limit);

        U64 ubo_size = renderer->uniforms.limit;
        if (ubo_size == 0) { ubo_size = MB(16); }

        // Just going to use push constants for the render setup data for now
        //
        renderer->uniforms.limit = ubo_size;
        renderer->uniforms.used  = 0;
        renderer->uniforms.data  = M_ArenaPush(vk->arena, U8, ubo_size);

        // Setup command buffer
        //
        if (renderer->command_buffer.limit == 0) {
            renderer->command_buffer.limit = MB(16);
        }

        renderer->command_buffer.used = 0;
        renderer->command_buffer.data = M_ArenaPush(vk->arena, U8, renderer->command_buffer.limit);

        // Setup base pipeline, this can't be configured at the moment but large api changes are going
        // to be made to allow this
        //
        {
            VK_Pipeline *pipeline = &device->pipeline;

            pipeline->state.topology         = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            pipeline->state.cull_mode        = VK_CULL_MODE_BACK_BIT;
            pipeline->state.depth_test       = VK_TRUE;
            pipeline->state.depth_write      = VK_TRUE;
            pipeline->state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;

            pipeline->state.blend.blendEnable         = VK_TRUE;
            pipeline->state.blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            pipeline->state.blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            pipeline->state.blend.colorBlendOp        = VK_BLEND_OP_ADD;
            pipeline->state.blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            pipeline->state.blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            pipeline->state.blend.alphaBlendOp        = VK_BLEND_OP_ADD;
            pipeline->state.blend.colorWriteMask      = 0xF;

            pipeline->num_targets       = 1;
            pipeline->target_formats[0] = device->swapchain.surface_format;
            pipeline->depth_format      = VK_FORMAT_D32_SFLOAT; // pretty much universally supported

            pipeline->num_shaders = 2;

            Str8 vert_code = Str8_WrapCount((U8 *) base_vert_code, sizeof(base_vert_code));
            Str8 frag_code = Str8_WrapCount((U8 *) base_frag_code, sizeof(base_frag_code));

            VK_ShaderModuleCreate(device, &pipeline->shaders[0], vert_code);
            VK_ShaderModuleCreate(device, &pipeline->shaders[1], frag_code);

            VK_PipelineCreate(device, pipeline);
        }

        // Create sprite texture array
        //
        {
            U32 dimension   = renderer->sprite_array.dimension;
            U32 level_count = 1;

            if (dimension == 0) {
                renderer->sprite_array.dimension = 256;
                level_count = 9;
            }
            else {
                // @todo: replace with clz or something...
                //
                while (dimension > 1) {
                    dimension  >>= 1;
                    level_count += 1;
                }
            }

            U32 width  = renderer->sprite_array.dimension;
            U32 height = width;
            U32 count  = renderer->sprite_array.limit;
            if (count == 0) { count = renderer->sprite_array.limit = 512; }

            VkImageCreateInfo create_info = { 0 };
            create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
            create_info.imageType     = VK_IMAGE_TYPE_2D;
            create_info.extent.width  = width;
            create_info.extent.height = height;
            create_info.extent.depth  = 1;
            create_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
            create_info.mipLevels     = level_count;
            create_info.arrayLayers   = count;
            create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;

            VK_ImageCreate(device, &device->sprite_array, &create_info);

            // Allocate other textures which are not used yet because they don't fit in the sprite array
            //
            if (renderer->texture_limit == 0) { renderer->texture_limit = 128; }

            device->image_limit = renderer->texture_limit;
            device->images      = M_ArenaPush(vk->arena, VK_Image, renderer->texture_limit);
        }

        // Create default sampler
        //
        {
            VkSamplerCreateInfo create_info = { 0 };
            create_info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            create_info.magFilter    = VK_FILTER_LINEAR;
            create_info.minFilter    = VK_FILTER_LINEAR;
            create_info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            VkResult success = vk->CreateSampler(device->handle, &create_info, 0, &device->sampler);
            if (success != VK_SUCCESS) { return result; }
        }

        if (renderer->layer_offset == 0.0f) { renderer->layer_offset = 1.0f; }

        result = (vbo->data && ibo->data);
    }

    M_TempReset();

    return result;
}

FileScope U32 VK_MemoryTypeIndexGet(VK_Device *device, U32 type_bits, VkMemoryPropertyFlags properties) {
    U32 result = U32_MAX;

    VkPhysicalDeviceMemoryProperties *mem_props = &device->memory_properties;

    for (U32 it = 0; it < mem_props->memoryTypeCount; ++it) {
        VkMemoryType *type = &mem_props->memoryTypes[it];

        if (type_bits & (1 << it) && ((type->propertyFlags & properties) == properties)) {
            result = it;
            break;
        }
    }

    Assert(result != U32_MAX);

    return result;
}

FileScope void VK_ImageCreate(VK_Device *device, VK_Image *image, VkImageCreateInfo *create_info) {
    VK_Context *vk = device->vk;

    VkResult success = vk->CreateImage(device->handle, create_info, 0, &image->handle);
    if (success == VK_SUCCESS) {
        // Allocate and bind memory, for now we just have dedicated allocations per image
        // will move to a more substantial allocator once we have things up and running!
        //
        // :vk_allocator
        //
        VkMemoryRequirements reqs;
        vk->GetImageMemoryRequirements(device->handle, image->handle, &reqs);

        VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkMemoryAllocateInfo alloc_info = { 0 };
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = reqs.size;
        alloc_info.memoryTypeIndex = VK_MemoryTypeIndexGet(device, reqs.memoryTypeBits, memory_properties);

        success = vk->AllocateMemory(device->handle, &alloc_info, 0, &image->memory);
        if (success == VK_SUCCESS) {
            vk->BindImageMemory(device->handle, image->handle, image->memory, 0);

            B32 is_array = (create_info->arrayLayers > 1) || image->force_array;
            B32 is_depth = (create_info->format == VK_FORMAT_D32_SFLOAT); // @robustness: other formats?

            // @hack: this isn't robust for cube map and 3d image arrays (which are not supported!) but
            // will do the job for now
            //
            VkImageViewType type = cast(VkImageViewType) create_info->imageType + (is_array << 2);

            // Create image view
            //
            VkImageViewCreateInfo view_create = { 0 };
            view_create.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_create.image    = image->handle;
            view_create.viewType = type;
            view_create.format   = create_info->format;

            view_create.subresourceRange.aspectMask     = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            view_create.subresourceRange.baseMipLevel   = 0;
            view_create.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
            view_create.subresourceRange.baseArrayLayer = 0;
            view_create.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

            vk->CreateImageView(device->handle, &view_create, 0, &image->view);
        }

        image->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

FileScope void VK_BufferCreate(VK_Device *device, VK_Buffer *buffer) {
    VK_Context *vk = device->vk;

    VkBufferCreateInfo create_info = { 0 };
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = buffer->size;
    create_info.usage = buffer->usage;

    VkResult success = vk->CreateBuffer(device->handle, &create_info, 0, &buffer->handle);
    if (success == VK_SUCCESS) {
        // :vk_allocator
        //
        VkMemoryRequirements reqs;
        vk->GetBufferMemoryRequirements(device->handle, buffer->handle, &reqs);

        VkMemoryPropertyFlags memory_properties;
        if (buffer->host_mapped) {
            memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
        else {
            memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        VkMemoryAllocateInfo alloc_info = { 0 };
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = reqs.size;
        alloc_info.memoryTypeIndex = VK_MemoryTypeIndexGet(device, reqs.memoryTypeBits, memory_properties);

        success = vk->AllocateMemory(device->handle, &alloc_info, 0, &buffer->memory);
        if (success == VK_SUCCESS) {
            success = vk->BindBufferMemory(device->handle, buffer->handle, buffer->memory, 0);
            if (success == VK_SUCCESS) {
                vk->MapMemory(device->handle, buffer->memory, 0, buffer->size, 0, &buffer->data);
            }
        }
    }
}

void VK_ShaderModuleCreate(VK_Device *device, VK_Shader *shader, Str8 code) {
    VK_Context *vk = device->vk;

    VkShaderModuleCreateInfo create_info = { 0 };
    create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.count;
    create_info.pCode    = cast(U32 *) code.data;

    vk->CreateShaderModule(device->handle, &create_info, 0, &shader->handle);
}

void VK_PipelineCreate(VK_Device *device, VK_Pipeline *pipeline) {
    VK_Context *vk = device->vk;

    {
        VkDescriptorSetLayoutBinding bindings[3] = { 0 };

        bindings[0].binding         = 0;
        bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;

        bindings[1].binding         = 1;
        bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[1].descriptorCount = 1;

        bindings[2].binding         = 2;
        bindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[2].descriptorCount = 1;

        VkDescriptorSetLayoutCreateInfo create_info = { 0 };
        create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = ArraySize(bindings);
        create_info.pBindings    = bindings;

        VkResult success = vk->CreateDescriptorSetLayout(device->handle, &create_info, 0, &pipeline->set_layout);
        if (success != VK_SUCCESS) { return; }
    }

    {
        // Create pipeline layout, @hardcode: for the base shader for now. will parse spirv later
        // to create the pipeline layout
        //
        StaticAssert(sizeof(ShaderGlobals) <= 128);

        VkPushConstantRange push_range = { 0 };
        push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_range.offset     = 0;
        push_range.size       = sizeof(ShaderGlobals);

        VkPipelineLayoutCreateInfo create_info = { 0 };
        create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount         = 1;
        create_info.pSetLayouts            = &pipeline->set_layout;
        create_info.pushConstantRangeCount = 1;
        create_info.pPushConstantRanges    = &push_range;

        VkResult success = vk->CreatePipelineLayout(device->handle, &create_info, 0, &pipeline->layout);
        if (success != VK_SUCCESS) { return; }
    }

    VkPipelineShaderStageCreateInfo shader_stages[4] = { 0 };

    for (U32 it = 0; it < pipeline->num_shaders; ++it) {
        VkPipelineShaderStageCreateInfo *stage = &shader_stages[it];

        stage->sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage->module = pipeline->shaders[it].handle;
        stage->pName  = "main";

        // Will be parsed from the spir-v, like wtf why do we even have to set this the
        // shader and the driver KNOW what stage the shader modules are and honestly at this
        // point the shaders probably aren't even any of the pre-defined stages at this point anyway
        //
        switch (it) {
            case 0: { stage->stage = VK_SHADER_STAGE_VERTEX_BIT;   } break;
            case 1: { stage->stage = VK_SHADER_STAGE_FRAGMENT_BIT; } break;
            default: { Assert(!"UNSUPPORTED"); } break;
        }
    }

    // unused but we have to supply it anyway
    //
    VkPipelineVertexInputStateCreateInfo vi = { 0 };
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia = { 0 };
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = pipeline->state.topology;

    VkPipelineViewportStateCreateInfo vp = { 0 };
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rs = { 0 };
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode    = pipeline->state.cull_mode;
    rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms = { 0 };
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds = { 0 };
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = pipeline->state.depth_test;
    ds.depthWriteEnable = pipeline->state.depth_write;
    ds.depthCompareOp   = pipeline->state.depth_compare_op;

    VkPipelineColorBlendStateCreateInfo om = { 0 };
    om.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    om.attachmentCount = 1;
    om.pAttachments    = &pipeline->state.blend;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dyn = { 0 };
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = ArraySize(dynamic_states);
    dyn.pDynamicStates    = dynamic_states;

    // Rendering info for dynamic rendering
    //
    VkPipelineRenderingCreateInfo rendering_info = { 0 };
    rendering_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount    = pipeline->num_targets;
    rendering_info.pColorAttachmentFormats = pipeline->target_formats;
    rendering_info.depthAttachmentFormat   = pipeline->depth_format;

    VkGraphicsPipelineCreateInfo create_info = { 0 };
    create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext               = &rendering_info;
    create_info.stageCount          = pipeline->num_shaders;
    create_info.pStages             = shader_stages;
    create_info.pVertexInputState   = &vi;
    create_info.pInputAssemblyState = &ia;
    create_info.pViewportState      = &vp;
    create_info.pRasterizationState = &rs;
    create_info.pMultisampleState   = &ms;
    create_info.pDepthStencilState  = &ds;
    create_info.pColorBlendState    = &om;
    create_info.pDynamicState       = &dyn;
    create_info.layout              = pipeline->layout;

    VkPipelineCache cache = VK_NULL_HANDLE; // @todo: implement!
    vk->CreateGraphicsPipelines(device->handle, cache, 1, &create_info, 0, &pipeline->handle);
}

B32 VK_SwapchainCreate(VK_Device *device, VK_Swapchain *swapchain) {
    B32 result = false;

    VK_Context *vk = device->vk;

    // Wait for the device to go idle, it is most likely that the swapchain will be re-built on present
    // and we cannot destroy images which are currently in-use as the command buffer may not have
    // been fully processed yet we wait until the final swapchain image has been presented before re-building
    //
    vk->DeviceWaitIdle(device->handle);

    if (swapchain->surface == VK_NULL_HANDLE) {
        // Create the surface and select the other initial properties of the swapchain
        //
        if (!VK_SurfaceCreate(device, swapchain)) {
            return result;
        }

        // Find suitable surface format
        //
        U32 num_formats = 0;
        vk->GetPhysicalDeviceSurfaceFormatsKHR(device->physical, swapchain->surface, &num_formats, 0);

        M_Arena *temp = M_TempGet();
        VkSurfaceFormatKHR *formats = M_ArenaPush(temp, VkSurfaceFormatKHR, num_formats);
        vk->GetPhysicalDeviceSurfaceFormatsKHR(device->physical, swapchain->surface, &num_formats, formats);

        for (U32 it = 0; it < num_formats; ++it) {
            if (formats[it].format == VK_FORMAT_R8G8B8A8_SRGB ||
                formats[it].format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                swapchain->surface_format = formats[it].format;
                break;
            }
        }

        if (swapchain->surface_format == VK_FORMAT_UNDEFINED) { return result; }

        // @todo: find mode for no vsync, we only support vsync for now as it is the
        // only present mode guaranteed to be available to us!
        //
        swapchain->vsync_off = VK_PRESENT_MODE_FIFO_KHR;
        swapchain->mode      = VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSwapchainKHR old_swapchain = swapchain->handle;

    VkSurfaceCapabilitiesKHR surface_caps;
    vk->GetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical, swapchain->surface, &surface_caps);

    // Set swapchain extent
    //
    if (surface_caps.currentExtent.width == U32_MAX) {
        // We can just choose our own
        //
        swapchain->extent.width  = vk->renderer->setup.window_dim.w;
        swapchain->extent.height = vk->renderer->setup.window_dim.h;
    }
    else {
        swapchain->extent = surface_caps.currentExtent;
    }

    VkSwapchainCreateInfoKHR create_info = { 0 };
    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = swapchain->surface;
    create_info.minImageCount    = Max(surface_caps.minImageCount, VK_NUM_IMAGES);
    create_info.imageFormat      = swapchain->surface_format;
    create_info.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // @hardcode: non-hdr
    create_info.imageExtent      = swapchain->extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.preTransform     = surface_caps.currentTransform;
    create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode      = swapchain->mode;
    create_info.oldSwapchain     = old_swapchain;

    VkResult success = vk->CreateSwapchainKHR(device->handle, &create_info, 0, &swapchain->handle);

    result = (success == VK_SUCCESS);
    if (result) {
        if (old_swapchain != VK_NULL_HANDLE) {
            // This is a swapchain re-creation
            //
            for (U32 it = 0; it < swapchain->num_images; ++it) {
                vk->DestroyImageView(device->handle, swapchain->views[it], 0);
            }

            vk->DestroyImageView(device->handle, swapchain->depth.view, 0);
            vk->DestroyImage(device->handle, swapchain->depth.handle, 0);
            vk->FreeMemory(device->handle, swapchain->depth.memory, 0);

            vk->DestroySwapchainKHR(device->handle, old_swapchain, 0);
        }

        // Get swapchain images
        //
        vk->GetSwapchainImagesKHR(device->handle, swapchain->handle, &swapchain->num_images, 0);

        if (!swapchain->images) {
            swapchain->images = M_ArenaPush(vk->arena, VkImage,     VK_MAX_IMAGES);
            swapchain->views  = M_ArenaPush(vk->arena, VkImageView, VK_MAX_IMAGES);
        }

        vk->GetSwapchainImagesKHR(device->handle, swapchain->handle, &swapchain->num_images, swapchain->images);

        // Create image views
        //
        for (U32 it = 0; it < swapchain->num_images; ++it) {
            VkImageViewCreateInfo view_create = { 0 };
            view_create.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_create.image    = swapchain->images[it];
            view_create.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_create.format   = swapchain->surface_format;

            view_create.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_create.subresourceRange.levelCount = 1;
            view_create.subresourceRange.layerCount = 1;

            success = vk->CreateImageView(device->handle, &view_create, 0, &swapchain->views[it]);
            if (success != VK_SUCCESS) { break; }
        }

        result = (success == VK_SUCCESS);

        VkImageCreateInfo depth_create = { 0 };
        depth_create.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depth_create.imageType     = VK_IMAGE_TYPE_2D;
        depth_create.format        = VK_FORMAT_D32_SFLOAT;
        depth_create.extent.width  = swapchain->extent.width;
        depth_create.extent.height = swapchain->extent.height;
        depth_create.extent.depth  = 1;
        depth_create.mipLevels     = 1;
        depth_create.arrayLayers   = 1;
        depth_create.samples       = VK_SAMPLE_COUNT_1_BIT;
        depth_create.tiling        = VK_IMAGE_TILING_OPTIMAL;
        depth_create.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depth_create.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_ImageCreate(device, &swapchain->depth, &depth_create);
    }

    return result;
}

void VK_SwapchainRebuild(VK_Device *device, VK_Swapchain *swapchain, VkResult success, B32 from_present) {
    VK_Context *vk = device->vk;

    if (success == VK_ERROR_SURFACE_LOST_KHR) {
        // Surface lost so destroy our current surface and attempt to create a new one
        //

        for (U32 it = 0; it < swapchain->num_images; ++it) {
            vk->DestroyImageView(device->handle, swapchain->views[it], 0);
        }

        vk->DestroySwapchainKHR(device->handle, swapchain->handle, 0);
        vk->DestroySurfaceKHR(vk->instance, swapchain->surface, 0);

        swapchain->surface = VK_NULL_HANDLE;
        swapchain->handle  = VK_NULL_HANDLE;

        VK_SwapchainCreate(device, swapchain);
    }
    else if (success == VK_ERROR_OUT_OF_DATE_KHR) {
        // Out of date so just re-create the swapchain
        //
        VK_SwapchainCreate(device, swapchain);
    }
    else if (success == VK_SUBOPTIMAL_KHR && from_present) {
        VkSurfaceCapabilitiesKHR caps;
        vk->GetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical, swapchain->surface, &caps);

        // Only re-build the swapchain if the extents differ, if this sub-optimal result came when acquiring
        // and image we don't re-build right away. the system will let us present to a sub-optimal swapchain
        // and will return the same result so we rebuild after the present
        //
        if (caps.currentExtent.width  != swapchain->extent.width ||
            caps.currentExtent.height != swapchain->extent.height)
        {
            VK_SwapchainCreate(device, swapchain);
        }

    }
    // else do nothing
}

//
// --------------------------------------------------------------------------------
// :VK_Submit
// --------------------------------------------------------------------------------
//
FileScope void VK_TexturesUpload(VK_Device *device, RendererContext *renderer) {
    VK_Context *vk    = device->vk;
    VK_Frame   *frame = device->frame;

    RendererTransferQueue *queue = &renderer->transfer_queue;

    if (queue->task_count == 0) { return; }

    M_Arena *temp = M_TempGet();

    // :note we can only do a single pass over the tasks as there may be pending tasks being processed
    // by other threads. this means if we do multiple passes (i.e. to produce barriers then copy regions etc.)
    // we may not produce enough barriers or regions where some tasks are PENDING in the first pass
    // and LOADED in the second pass
    //

    VkCommandBuffer cmds = frame->transfer;

    VkCommandBufferBeginInfo begin_info = { 0 };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vk->BeginCommandBuffer(cmds, &begin_info);

    U32 num_tasks = 0; // this is the actual number of tasks we processed

    VkDependencyInfo dependency_info = { 0 };
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

    VkImageMemoryBarrier2    *before_barriers = M_ArenaPush(temp, VkImageMemoryBarrier2,    queue->task_count);
    VkImageMemoryBarrier2    *after_barriers  = M_ArenaPush(temp, VkImageMemoryBarrier2,    queue->task_count);
    VkCopyBufferToImageInfo2 *copy_info       = M_ArenaPush(temp, VkCopyBufferToImageInfo2, queue->task_count);

    B32 to_sprite = false;

    while (queue->task_count != 0) {
        RendererTransferTask *task = &queue->tasks[queue->first_task];
        if (task->state == RENDERER_TRANSFER_TASK_STATE_LOADED) {
            VkImageMemoryBarrier2    *before = &before_barriers[num_tasks];
            VkImageMemoryBarrier2    *after  = &after_barriers[num_tasks];
            VkCopyBufferToImageInfo2 *info   = &copy_info[num_tasks];

            num_tasks += 1;

            U32 width  = task->texture.width;
            U32 height = task->texture.height;
            U32 index  = task->texture.index;

            info->sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
            info->srcBuffer      = device->transfer_buffer.handle;
            info->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            // Transition from UNDEFINED -> TRANSFER_DST_OPTIMAL
            //
            before->sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            before->srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
            before->srcAccessMask = VK_ACCESS_2_NONE;
            before->dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            before->dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            before->oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            before->newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            // Transition from TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
            //
            after->sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            after->srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            after->srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            after->dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            after->dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            after->oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            after->newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (RendererTextureIsSprite(renderer, task->texture)) {
                VK_Image *sprite_array = &device->sprite_array;

                to_sprite = true;

                // Upload all of the mip-maps for a single layer
                //
                before->image  = sprite_array->handle;
                after->image   = sprite_array->handle;
                info->dstImage = sprite_array->handle;

                before->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                before->subresourceRange.baseMipLevel   = 0;
                before->subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
                before->subresourceRange.baseArrayLayer = index;
                before->subresourceRange.layerCount     = 1;

                after->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                after->subresourceRange.baseMipLevel   = 0;
                after->subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
                after->subresourceRange.baseArrayLayer = index;
                after->subresourceRange.layerCount     = 1;

                device->max_sprite_index = Max(index, device->max_sprite_index);

                // Setup mip map regions
                //
                info->regionCount = task->mip_levels + 1;

                VkBufferImageCopy2 *regions = M_ArenaPush(temp, VkBufferImageCopy2, info->regionCount);
                info->pRegions              = regions;

                U64 offset = task->offset;

                for (U32 it = 0; it < info->regionCount; ++it) {
                    VkBufferImageCopy2 *copy = &regions[it];

                    copy->sType              = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
                    copy->bufferOffset       = offset;
                    copy->imageExtent.width  = width;
                    copy->imageExtent.height = height;
                    copy->imageExtent.depth  = 1;

                    copy->imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    copy->imageSubresource.mipLevel       = it;
                    copy->imageSubresource.baseArrayLayer = index;
                    copy->imageSubresource.layerCount     = 1;

                    offset += (4 * width * height);

                    width  >>= 1;
                    height >>= 1;

                    if (width  == 0) { width  = 1; }
                    if (height == 0) { height = 1; }
                }

                Assert((task->offset + task->size) == offset);
            }
            else {
                // @todo: we have to create images here for non-sprite textures
                //
                Assert(index < device->image_limit);

                VK_Image *image = &device->images[index];

                image->force_array = true;

                VkImageCreateInfo create_info = { 0 };
                create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                create_info.format        = VK_FORMAT_R8G8B8A8_SRGB;
                create_info.imageType     = VK_IMAGE_TYPE_2D;
                create_info.extent.width  = width;
                create_info.extent.height = height;
                create_info.extent.depth  = 1;
                create_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                create_info.mipLevels     = 1;
                create_info.arrayLayers   = 1;
                create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
                create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;

                VK_ImageCreate(device, image, &create_info);

                before->image  = image->handle;
                after->image   = image->handle;
                info->dstImage = image->handle;

                before->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                before->subresourceRange.baseMipLevel   = 0;
                before->subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
                before->subresourceRange.baseArrayLayer = 0;
                before->subresourceRange.layerCount     = 1;

                after->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                after->subresourceRange.baseMipLevel   = 0;
                after->subresourceRange.levelCount     = 1;
                after->subresourceRange.baseArrayLayer = 0;
                after->subresourceRange.layerCount     = 1;

                // Setup mip map regions
                //

                VkBufferImageCopy2 *region = M_ArenaPush(temp, VkBufferImageCopy2);

                info->regionCount = 1;
                info->pRegions    = region;

                region->sType              = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
                region->bufferOffset       = task->offset;
                region->imageExtent.width  = width;
                region->imageExtent.height = height;
                region->imageExtent.depth  = 1;

                region->imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                region->imageSubresource.mipLevel       = 0;
                region->imageSubresource.baseArrayLayer = 0;
                region->imageSubresource.layerCount     = 1;
            }
        }
        else if (task->state == RENDERER_TRANSFER_TASK_STATE_PENDING) {
            break;
        }

        queue->first_task += 1;
        queue->first_task &= (queue->task_limit - 1);

        queue->task_count -= 1;

        queue->read_offset = task->offset;
    }

    // Issue commands
    //
    dependency_info.imageMemoryBarrierCount = num_tasks;
    dependency_info.pImageMemoryBarriers    = before_barriers;

    vk->CmdPipelineBarrier2(cmds, &dependency_info);

    for (U32 it = 0; it < num_tasks; ++it) {
        vk->CmdCopyBufferToImage2(cmds, &copy_info[it]);
    }

    dependency_info.pImageMemoryBarriers = after_barriers;

    vk->CmdPipelineBarrier2(cmds, &dependency_info);

    vk->EndCommandBuffer(cmds);

    device->sprite_gen += to_sprite;

    // Submit the work to the gpu
    //
    VkSubmitInfo submit_info =  { 0 };
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &cmds;

    // This will want a fence to wait on instead of doing vkDeviceWaitIdle, that said we only
    // upload images that have actually been referenced, thus the must've been used in a draw command
    // at some point in the last frame. Due to the memory barriers waiting on specific stages for
    // layout transitions this means the texture upload is guaranteed to be finished by time we draw
    // and thus this command buffer _must_ be complete by the end of the frame
    //
    vk->QueueSubmit(device->queue.handle, 1, &submit_info, VK_NULL_HANDLE);
}

RENDERER_SUBMIT(VK_Submit) {
    VK_Context *vk     = renderer->backend;
    VK_Device  *device = &vk->device;
    VK_Frame   *frame  = device->frame;

    vk->ResetFences(device->handle, 1, &frame->fence);

    vk->ResetCommandPool(device->handle, frame->command_pool, 0);
    vk->ResetDescriptorPool(device->handle, frame->descriptor_pool, 0);

    VK_TexturesUpload(device, renderer);

    if (frame->sprite_gen != device->sprite_gen) {
        if (frame->sprite_view != VK_NULL_HANDLE) {
            vk->DestroyImageView(device->handle, frame->sprite_view, 0);
        }

        // Update our sprite view to make sure we have the image array available to us
        //
        VkImageViewCreateInfo create_info = { 0 };
        create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image    = device->sprite_array.handle;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        create_info.format   = VK_FORMAT_R8G8B8A8_SRGB;

        create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel   = 0;
        create_info.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount     = device->max_sprite_index;

        vk->CreateImageView(device->handle, &create_info, 0, &frame->sprite_view);

        frame->sprite_gen = device->sprite_gen;
    }

    // Acquire image from swapchain
    //
    VK_Swapchain *swapchain = &device->swapchain;
    VkResult success = vk->AcquireNextImageKHR(device->handle, swapchain->handle, U64_MAX, frame->acquire, 0, &frame->image);
    VK_SwapchainRebuild(device, swapchain, success, false /* from_present */);

    // start a command buffer
    //
    VkCommandBuffer cmds = frame->cmds;
    {
        VkCommandBufferBeginInfo begin_info = { 0 };
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vk->BeginCommandBuffer(cmds, &begin_info);
    }

    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = cast(F32) renderer->setup.window_dim.h;
    viewport.width    = cast(F32) renderer->setup.window_dim.w;
    viewport.height   = -viewport.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = renderer->setup.window_dim.w;
    scissor.extent.height = renderer->setup.window_dim.h;

    vk->CmdSetViewport(cmds, 0, 1, &viewport);
    vk->CmdSetScissor(cmds, 0, 1, &scissor);

    // Transition colour output (and depth if needed) to their respective attachment optimal layouts
    //
    {
        U32 num_barriers = 1;
        VkImageMemoryBarrier2 barriers[2] = { 0 };

        // Color attachment as always
        //
        barriers[0].sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barriers[0].srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
        barriers[0].srcAccessMask = VK_ACCESS_2_NONE;
        barriers[0].dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barriers[0].oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[0].newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barriers[0].image         = swapchain->images[frame->image];

        barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barriers[0].subresourceRange.levelCount = 1;
        barriers[0].subresourceRange.layerCount = 1;

        // Depth attachment the first time
        //
        if (swapchain->depth.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
            num_barriers += 1;

            barriers[1].sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barriers[1].srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
            barriers[1].srcAccessMask = VK_ACCESS_2_NONE;
            barriers[1].dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            barriers[1].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barriers[1].oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            barriers[1].newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barriers[1].image         = swapchain->depth.handle;

            // @incomplete: if we want stencil we need to check the format
            //
            barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barriers[1].subresourceRange.levelCount = 1;
            barriers[1].subresourceRange.layerCount = 1;

            swapchain->depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkDependencyInfo dependency_info = { 0 };
        dependency_info.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.imageMemoryBarrierCount = num_barriers;
        dependency_info.pImageMemoryBarriers    = barriers;

        vk->CmdPipelineBarrier2(cmds, &dependency_info);
    }

    // Begin rendering
    //
    VkRenderingAttachmentInfo colour_output = { 0 };
    VkRenderingAttachmentInfo depth_output  = { 0 };

    VkClearValue clear_colour = { 0 };
    clear_colour.color.float32[0] = 0.0f;
    clear_colour.color.float32[1] = 0.0f;
    clear_colour.color.float32[2] = 0.0f;
    clear_colour.color.float32[3] = 1.0f;

    VkClearValue clear_depth = { 0 };
    clear_depth.depthStencil.depth = 1.0f;

    colour_output.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colour_output.imageView   = swapchain->views[frame->image];
    colour_output.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour_output.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_output.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colour_output.clearValue  = clear_colour;

    depth_output.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_output.imageView   = swapchain->depth.view;
    depth_output.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_output.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_output.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    depth_output.clearValue  = clear_depth;

    VkRenderingInfo rendering_info = { 0 };
    rendering_info.sType                    = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.extent.width  = swapchain->extent.width;
    rendering_info.renderArea.extent.height = swapchain->extent.height;
    rendering_info.layerCount               = 1;
    rendering_info.colorAttachmentCount     = 1;
    rendering_info.pColorAttachments        = &colour_output;
    rendering_info.pDepthAttachment         = &depth_output;

    vk->CmdBeginRendering(cmds, &rendering_info);

    Buffer *commands = &renderer->command_buffer;

    vk->CmdBindPipeline(cmds, VK_PIPELINE_BIND_POINT_GRAPHICS, device->pipeline.handle);

    VkDescriptorBufferInfo vbuffer = { 0 };
    vbuffer.buffer = device->vertex_buffer.handle;
    vbuffer.offset = frame->imm.vertex_offset;
    vbuffer.range  = renderer->vertices.count * sizeof(Vertex3);

    VkDescriptorImageInfo sampler = { 0 };
    sampler.sampler = device->sampler;

    vk->CmdBindIndexBuffer(cmds, device->index_buffer.handle, frame->imm.index_offset, VK_INDEX_TYPE_UINT16);

    for (S64 offset = 0; offset < commands->used;) {
        RenderCommandType type = *cast(RenderCommandType *) (commands->data + offset);
        offset += sizeof(RenderCommandType);

        switch (type) {
            case RENDER_COMMAND_RenderCommandDraw: {
                RenderCommandDraw *draw = cast(RenderCommandDraw *) (commands->data + offset);
                offset += sizeof(RenderCommandDraw);

                VkDescriptorSet set;
                {
                    VkDescriptorSetAllocateInfo alloc_info = { 0 };
                    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    alloc_info.descriptorPool     = frame->descriptor_pool;
                    alloc_info.descriptorSetCount = 1;
                    alloc_info.pSetLayouts        = &device->pipeline.set_layout;

                    vk->AllocateDescriptorSets(device->handle, &alloc_info, &set);
                }

                // This is hardcoded for the base shader for now
                //

                U8 *gloabls = &renderer->uniforms.data[draw->ubo_offset];

                VkImageView view = VK_NULL_HANDLE;
                if (RendererTextureIsSprite(renderer, draw->texture)) {
                    view = frame->sprite_view;
                }
                else {
                    Assert(draw->texture.index < device->image_limit);
                    VK_Image *image = &device->images[draw->texture.index];
                    view = image->view;
                }

                // @hack: testing stuff
                //
                if (view != VK_NULL_HANDLE) {
                    VkDescriptorImageInfo image_info = { 0 };
                    image_info.imageView   = view;
                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkWriteDescriptorSet writes[3] = { 0 };

                    writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writes[0].dstSet          = set;
                    writes[0].dstBinding      = 0;
                    writes[0].dstArrayElement = 0;
                    writes[0].descriptorCount = 1;
                    writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    writes[0].pBufferInfo     = &vbuffer;

                    writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writes[1].dstSet          = set;
                    writes[1].dstBinding      = 1;
                    writes[1].dstArrayElement = 0;
                    writes[1].descriptorCount = 1;
                    writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
                    writes[1].pImageInfo      = &sampler;

                    writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writes[2].dstSet          = set;
                    writes[2].dstBinding      = 2;
                    writes[2].dstArrayElement = 0;
                    writes[2].descriptorCount = 1;
                    writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    writes[2].pImageInfo      = &image_info;

                    vk->UpdateDescriptorSets(device->handle, ArraySize(writes), writes, 0, 0);
                    vk->CmdBindDescriptorSets(cmds, VK_PIPELINE_BIND_POINT_GRAPHICS, device->pipeline.layout, 0, 1, &set, 0, 0);
                    vk->CmdPushConstants(cmds, device->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderGlobals), gloabls);

                    vk->CmdDrawIndexed(cmds, draw->index_count, 1, draw->index_offset, draw->vertex_offset, 0);
                }
            }
            break;
        }
    }

    vk->CmdEndRendering(cmds);

    // Transition swapchain image to present src optimal
    //
    {
        VkImageMemoryBarrier2 barrier = { 0 };

        // Color attachment as always
        //
        barrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask  = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_NONE;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.image         = swapchain->images[frame->image];

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;

        VkDependencyInfo dependency_info = { 0 };
        dependency_info.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.imageMemoryBarrierCount = 1;
        dependency_info.pImageMemoryBarriers    = &barrier;

        vk->CmdPipelineBarrier2(cmds, &dependency_info);
    }

    vk->EndCommandBuffer(cmds);

    VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &frame->acquire;
    submit_info.pWaitDstStageMask    = &wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmds;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &frame->render;

    vk->QueueSubmit(device->queue.handle, 1, &submit_info, frame->fence);

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &frame->render;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain->handle;
    present_info.pImageIndices      = &frame->image;

    // @todo: check result for swapchain resize
    //
    success = vk->QueuePresentKHR(device->queue.handle, &present_info);
    VK_SwapchainRebuild(device, swapchain, success, true /* from_present */);

    // Don't go too far ahead we need at least one in flight frame to be idle before we can
    // start writing to its portion of the vertex/index buffer
    //
    VK_Frame *next = frame->next;
    device->frame  = next;

    vk->WaitForFences(device->handle, 1, &next->fence, VK_FALSE, U64_MAX);

    // reset immediate mode buffers for next frame
    //
    renderer->vertices.count = 0;
    renderer->indices.count  = 0;

    U8 *vertex_base = cast(U8 *) device->vertex_buffer.data;
    U8 *index_base  = cast(U8 *) device->index_buffer.data;

    renderer->vertices.base = cast(Vertex3 *) (vertex_base + next->imm.vertex_offset);
    renderer->indices.base  = cast(U16     *) (index_base  + next->imm.index_offset);

    renderer->command_buffer.used = 0;
    renderer->uniforms.used       = 0;

    renderer->layer     = 0;
    renderer->draw_call = 0;
}

#if OS_WIN32

VK_Context *VK_ContextCreate() {
    VK_Context *vk = 0;

    M_Arena *arena = M_ArenaAlloc(GB(8), 0);

    vk = M_ArenaPush(arena, VK_Context);
    vk->arena = arena;

    HANDLE lib = LoadLibraryA("vulkan-1.dll");
    if (lib) {
        vk->lib = cast(void *) lib;
        vk->GetInstanceProcAddr = cast(PFN_vkGetInstanceProcAddr) GetProcAddress(lib, "vkGetInstanceProcAddr");
    }

    vk->num_instance_extensions = 2;
    vk->instance_extensions[0]  = VK_KHR_SURFACE_EXTENSION_NAME;
    vk->instance_extensions[1]  = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

    if (!vk->GetInstanceProcAddr) {
        M_ArenaRelease(arena);
        vk = 0;
    }

    return vk;
}

// The only real platform specific portion of vulkan init
//
typedef struct Win32_WindowData Win32_WindowData;
struct Win32_WindowData {
    HINSTANCE hInstance;
    HWND      hwnd;
};

B32 VK_SurfaceCreate(VK_Device *device, VK_Swapchain *swapchain) {
    B32 result;

    VK_Context *vk = device->vk;

    Win32_WindowData *window = cast(Win32_WindowData *) swapchain->platform;

    VkWin32SurfaceCreateInfoKHR create_info = { 0 };
    create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.hinstance = window->hInstance;
    create_info.hwnd      = window->hwnd;

    VkResult success = vk->CreateWin32SurfaceKHR(vk->instance, &create_info, 0, &swapchain->surface);

    result = (success == VK_SUCCESS);
    return result;
}

#elif OS_LINUX

VK_Context *VK_ContextCreate() {
    Assert(!"NOT IMPLEMENTED");
}

#endif
