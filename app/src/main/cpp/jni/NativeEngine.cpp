#include "NativeEngine.h"
#include <android/log.h>
#include <cstring>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Kronos3D", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Kronos3D", __VA_ARGS__)

namespace kronos {

static std::unique_ptr<EngineState> g_engine;

static bool check_vulkan_support() {
    // Simple check - try to create instance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Kronos3D";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkInstance instance;
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    if (result == VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        return true;
    }
    return false;
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_kronos3d_MainActivity_nativeInit(JNIEnv* env, jobject obj, jobject surface, jint width, jint height) {
    LOGI("nativeInit: %dx%d", width, height);

    g_engine = std::make_unique<EngineState>();
    g_engine->width = width;
    g_engine->height = height;
    g_engine->window = ANativeWindow_fromSurface(env, surface);
    g_engine->running = true;

    // Check Vulkan support
    g_engine->vulkan_available = check_vulkan_support();
    g_engine->use_vulkan = g_engine->vulkan_available;

    LOGI("Vulkan available: %s", g_engine->vulkan_available ? "YES" : "NO");

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
        g_engine->gl_ctx = std::make_unique<opengl::GLContext>();
        g_engine->gl_renderer = std::make_unique<opengl::GLRenderer>();
        if (!g_engine->gl_renderer->init(*g_engine->gl_ctx, g_engine->window, width, height)) {
            LOGE("OpenGL ES init failed!");
            return JNI_FALSE;
        }
        g_engine->gl_renderer->upload_mesh(g_engine->scene_mesh);
    }

    g_engine->last_frame_time = 0;
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeResize(JNIEnv* env, jobject obj, jint width, jint height) {
    LOGI("nativeResize: %dx%d", width, height);
    if (!g_engine) return;

    g_engine->width = width;
    g_engine->height = height;
    g_engine->proj_matrix = Mat4::perspective(45.0f * 3.14159f/180.0f, (float)width / (float)height, 0.1f, 100.0f);

    if (g_engine->use_vulkan && g_engine->vk_renderer) {
        g_engine->vk_renderer->resize(width, height);
    } else if (g_engine->gl_renderer) {
        g_engine->gl_renderer->resize(width, height);
    }
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeRender(JNIEnv* env, jobject obj) {
    if (!g_engine || !g_engine->running) return;

    uint64_t current_time = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    current_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    if (g_engine->last_frame_time > 0) {
        g_engine->delta_time = (current_time - g_engine->last_frame_time) * 1e-9f;
        g_engine->total_time += g_engine->delta_time;
    }
    g_engine->last_frame_time = current_time;

    if (g_engine->use_vulkan && g_engine->vk_renderer) {
        g_engine->vk_renderer->draw_frame(
            g_engine->view_matrix,
            g_engine->proj_matrix,
            g_engine->camera_pos,
            g_engine->scene_mesh
        );
    } else if (g_engine->gl_renderer) {
        opengl::Uniforms uniforms;
        uniforms.model = Mat4::identity();
        uniforms.view = g_engine->view_matrix;
        uniforms.proj = g_engine->proj_matrix;
        uniforms.normal_matrix = Mat4::inverse(uniforms.model);
        uniforms.camera_pos = g_engine->camera_pos;
        uniforms.time = g_engine->total_time;

        // Get mesh from renderer (we need to store it)
        // For now, re-upload each frame (inefficient but works for demo)
        // In production, store GLMesh in EngineState
        static bool first = true;
        static opengl::GLMesh cached_mesh;
        if (first) {
            cached_mesh = g_engine->gl_renderer->upload_mesh(g_engine->scene_mesh);
            first = false;
        }
        g_engine->gl_renderer->draw_frame(uniforms, cached_mesh);
    }
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeTouchEvent(JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y, jfloat pressure) {
    if (!g_engine) return;

    static float last_x = 0, last_y = 0;
    static bool first_touch = true;

    if (first_touch) {
        last_x = x;
        last_y = y;
        first_touch = false;
        return;
    }

    float dx = x - last_x;
    float dy = y - last_y;
    last_x = x;
    last_y = y;

    const float sensitivity = 0.005f;

    switch (action) {
        case 0: // ACTION_DOWN
        case 2: // ACTION_MOVE
            // Single finger: orbit
            g_engine->orbit(dx * sensitivity, dy * sensitivity);
            break;
        // Multi-touch handling would go here
    }
}

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeCleanup(JNIEnv* env, jobject obj) {
    LOGI("nativeCleanup");
    if (g_engine) {
        g_engine->running = false;
        if (g_engine->vk_renderer) g_engine->vk_renderer->cleanup();
        g_engine->vk_renderer.reset();
        g_engine->vk_ctx.reset();
        g_engine->gl_renderer.reset();
        g_engine->gl_ctx.reset();
        if (g_engine->window) {
            ANativeWindow_release(g_engine->window);
            g_engine->window = nullptr;
        }
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
    std::string info = g_engine->use_vulkan ? "Vulkan 1.2+" : "OpenGL ES 3.2";
    return env->NewStringUTF(info.c_str());
}

} // extern "C"

} // namespace kronos