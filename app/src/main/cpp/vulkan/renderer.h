#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/native_window.h>
#include <vector>
#include <array>
#include <memory>
#include "math/vector.h"
#include "math/matrix.h"
#include "mesh/bmesh.h"

namespace kronos {
namespace vulkan {

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    uint32_t graphics_queue_family = UINT32_MAX;
    uint32_t present_queue_family = UINT32_MAX;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR swapchain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D swapchain_extent = {0, 0};
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    size_t current_frame = 0;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Memory allocator
    VkPhysicalDeviceMemoryProperties mem_properties;
};

struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;

    static VkVertexInputBindingDescription get_binding_description() {
        return {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    }

    static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() {
        return {{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
            {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
        }};
    }
};

struct UniformBufferObject {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Mat4 normal_matrix;
    Vec3 camera_pos;
    float time;
    Vec3 light_dir;
    float light_intensity;
    Vec3 light_color;
    float ambient_intensity;
};

struct MeshBuffer {
    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_memory = VK_NULL_HANDLE;
    VkBuffer index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory index_memory = VK_NULL_HANDLE;
    uint32_t index_count = 0;
    Bounds bounds;
};

struct DescriptorSets {
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> sets;
};

struct Pipeline {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline graphics = VK_NULL_HANDLE;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer() { cleanup(); }

    bool init(VulkanContext& ctx, ANativeWindow* window, int width, int height);
    void cleanup();
    void resize(int width, int height);
    void draw_frame(const Mat4& view, const Mat4& proj, const Vec3& camera_pos, const BMesh& mesh);
    void upload_mesh(const BMesh& mesh);

private:
    VulkanContext* ctx_ = nullptr;
    MeshBuffer mesh_buffer_;
    UniformBufferObject ubo_;
    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_memories_;
    std::vector<void*> uniform_mapped_;
    DescriptorSets desc_sets_;
    Pipeline pipeline_;

    bool create_instance();
    bool create_surface(ANativeWindow* window);
    bool pick_physical_device();
    bool create_logical_device();
    bool create_swapchain(int width, int height);
    bool create_render_pass();
    bool create_descriptor_set_layout();
    bool create_pipeline_layout();
    bool create_graphics_pipeline();
    bool create_command_pool();
    bool create_depth_resources();
    bool create_framebuffers();
    bool create_command_buffers();
    bool create_sync_objects();
    bool create_uniform_buffers();
    bool create_descriptor_pool();
    bool create_descriptor_sets();
    void create_mesh_buffers(const BMesh& mesh);
    void update_uniform_buffer(uint32_t current_image, const Mat4& view, const Mat4& proj, const Vec3& camera_pos);
    void record_command_buffer(VkCommandBuffer cmd, uint32_t image_index);
    void draw_mesh(VkCommandBuffer cmd);

    // Shader loading
    VkShaderModule create_shader_module(const char* code, size_t size);
    static std::vector<char> read_spv_file(const char* path);

    // Memory helpers
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
};

} // namespace vulkan
} // namespace kronos