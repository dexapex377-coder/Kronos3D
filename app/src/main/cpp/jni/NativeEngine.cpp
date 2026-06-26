#include "NativeEngine.h"
#include <android/log.h>
#include <cstring>
#include <ctime>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Kronos3D-JNI", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Kronos3D-JNI", __VA_ARGS__)

namespace kronos {

static std::unique_ptr<EngineState> g_engine;

static bool check_vulkan_support() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Kronos3D";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkInstance instance;
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        return false;

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) { vkDestroyInstance(instance, nullptr); return false; }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    bool found = false;
    for (auto& pd : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(pd, &props);
        if (props.apiVersion >= VK_API_VERSION_1_2) { found = true; break; }
    }
    vkDestroyInstance(instance, nullptr);
    return found;
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_kronos3d_MainActivity_nativeInit(JNIEnv* env, jobject obj, jobject surface, jint width, jint height) {
    LOGI("=== nativeInit %dx%d ===", width, height);

    g_engine = std::make_unique<EngineState>();
    g_engine->width = width;
    g_engine->height = height;
    g_engine->window = ANativeWindow_fromSurface(env, surface);
    g_engine->running = true;

    g_engine->vulkan_available = check_vulkan_support();
    g_engine->use_vulkan = g_engine->vulkan_available;
    LOGI("Vulkan available: %s", g_engine->vulkan_available ? "YES" : "NO");

    g_engine->proj_matrix = Mat4::perspective(45.0f * 3.14159f/180.0f, (float)width/(float)height, 0.1f, 100.0f);

    if (g_engine->use_vulkan) {
        g_engine->vk_ctx = std::make_unique<vulkan::VulkanContext>();
        g_engine->vk_renderer = std::make_unique<vulkan::Renderer>();
        if (!g_engine->vk_renderer->init(*g_engine->vk_ctx, g_engine->window, width, height)) {
            LOGE("Vulkan init failed, falling back to OpenGL ES");
            g_engine->vk_renderer.reset();
            g_engine->vk_ctx.reset();
            g_engine->use_vulkan = false;
        } else {
            g_engine->vk_renderer->upload_mesh(g_engine->scene_mesh);
        }
    }

    if (!g_engine->use_vulkan) {
        LOGI("OpenGL ES renderer objects created (lazy init on render thread)");
        g_engine->gl_ctx = std::make_unique<opengl::GLContext>();
        g_engine->gl_renderer = std::make_unique<opengl::GLRenderer>();
    }

    g_engine->last_frame_time = 0;
    LOGI("=== nativeInit complete ===");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeResize(JNIEnv* env, jobject obj, jint width, jint height) {
    if (!g_engine) return;
    g_engine->width = width;
    g_engine->height = height;
    g_engine->proj_matrix = Mat4::perspective(45.0f * 3.14159f/180.0f, (float)width/(float)height, 0.1f, 100.0f);
    if (g_engine->use_vulkan && g_engine->vk_renderer)
        g_engine->vk_renderer->resize(width, height);
    else if (g_engine->gl_renderer)
        g_engine->gl_renderer->resize(width, height);
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeRender(JNIEnv* env, jobject obj) {
    if (!g_engine || !g_engine->running) return;

    static timespec last_ts = {};
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (last_ts.tv_sec != 0) {
        float dt = (ts.tv_sec - last_ts.tv_sec) + (ts.tv_nsec - last_ts.tv_nsec) * 1e-9f;
        g_engine->delta_time = dt;
        g_engine->total_time += dt;
    }
    last_ts = ts;

    if (g_engine->use_vulkan && g_engine->vk_renderer) {
        g_engine->vk_renderer->draw_frame(
            g_engine->view_matrix, g_engine->proj_matrix,
            g_engine->camera_pos, g_engine->scene_mesh);
    } else if (g_engine->gl_renderer) {
        static bool first = true;
        static opengl::GLMesh cached_mesh;
        if (first) {
            LOGI("Initializing OpenGL ES on render thread...");
            if (!g_engine->gl_renderer->init(*g_engine->gl_ctx, g_engine->window, g_engine->width, g_engine->height)) {
                LOGE("OpenGL ES init failed on render thread!");
                return;
            }
            cached_mesh = g_engine->gl_renderer->upload_mesh(g_engine->scene_mesh);
            first = false;
        }
        opengl::Uniforms uniforms;
        uniforms.model = Mat4::identity();
        uniforms.view = g_engine->view_matrix;
        uniforms.proj = g_engine->proj_matrix;
        uniforms.normal_matrix = inverse(uniforms.model);
        uniforms.camera_pos = g_engine->camera_pos;
        uniforms.time = g_engine->total_time;
        g_engine->gl_renderer->draw_frame(uniforms, cached_mesh);
    }
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeTouchEvent(JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y, jfloat pressure) {
    if (!g_engine) return;
    static float last_x = 0, last_y = 0;
    if (action == 0) { last_x = x; last_y = y; return; }
    float dx = x - last_x, dy = y - last_y;
    last_x = x; last_y = y;
    const float sensitivity = 0.005f;
    if (action == 2) g_engine->orbit(dx * sensitivity, dy * sensitivity);
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeCleanup(JNIEnv* env, jobject obj) {
    LOGI("nativeCleanup");
    if (g_engine) {
        g_engine->running = false;
        if (g_engine->vk_renderer) g_engine->vk_renderer->cleanup();
        if (g_engine->gl_renderer) g_engine->gl_renderer->cleanup();
        g_engine->vk_renderer.reset(); g_engine->vk_ctx.reset();
        g_engine->gl_renderer.reset(); g_engine->gl_ctx.reset();
        if (g_engine->window) { ANativeWindow_release(g_engine->window); g_engine->window = nullptr; }
        g_engine.reset();
    }
}

JNIEXPORT jstring JNICALL
Java_com_kronos3d_MainActivity_nativeGetVersion(JNIEnv* env, jobject obj) {
    return env->NewStringUTF("Kronos3D 0.1.0 (Vulkan + GLES3.2)");
}

JNIEXPORT jstring JNICALL
Java_com_kronos3d_MainActivity_nativeGetRendererInfo(JNIEnv* env, jobject obj) {
    if (!g_engine) return env->NewStringUTF("Not initialized");
    return env->NewStringUTF(g_engine->vulkan_available ? "Vulkan 1.2+" : "OpenGL ES 3.2");
}

} // extern "C"

} // namespace kronos