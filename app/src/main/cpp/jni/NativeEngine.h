#pragma once

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <memory>
#include "../vulkan/renderer.h"
#include "../opengl/renderer.h"
#include "../core/mesh/bmesh.h"
#include "../core/math/matrix.h"

namespace kronos {

struct EngineState {
    bool use_vulkan = true;
    bool vulkan_available = false;

    std::unique_ptr<vulkan::VulkanContext> vk_ctx;
    std::unique_ptr<vulkan::Renderer> vk_renderer;

    std::unique_ptr<opengl::GLContext> gl_ctx;
    std::unique_ptr<opengl::GLRenderer> gl_renderer;

    ANativeWindow* window = nullptr;
    int width = 0, height = 0;
    bool running = false;

    // Scene
    BMesh scene_mesh;
    Mat4 view_matrix;
    Mat4 proj_matrix;
    Vec3 camera_pos = {0, 0, 5};
    Vec3 camera_target = {0, 0, 0};
    float camera_distance = 5.0f;
    float camera_yaw = 0.0f;
    float camera_pitch = 0.0f;

    // Timing
    uint64_t last_frame_time = 0;
    float delta_time = 0.0f;
    float total_time = 0.0f;

    EngineState() {
        // Create default cube
        scene_mesh = BMesh::cube(1.0f);
        update_camera();
    }

    void update_camera() {
        float x = camera_distance * cosf(camera_pitch) * sinf(camera_yaw);
        float y = camera_distance * sinf(camera_pitch);
        float z = camera_distance * cosf(camera_pitch) * cosf(camera_yaw);
        camera_pos = {x, y, z} + camera_target;
        view_matrix = Mat4::look_at(camera_pos, camera_target, {0, 1, 0});
    }

    void orbit(float delta_yaw, float delta_pitch) {
        camera_yaw += delta_yaw;
        camera_pitch = clamp(camera_pitch + delta_pitch, -1.5f, 1.5f);
        update_camera();
    }

    void zoom(float delta) {
        camera_distance = clamp(camera_distance + delta, 0.5f, 50.0f);
        update_camera();
    }

    void pan(float dx, float dy) {
        Vec3 right = normalize(cross({0,1,0}, camera_pos - camera_target));
        Vec3 up = {0, 1, 0};
        camera_target = camera_target + right * dx + up * dy;
        update_camera();
    }
};

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_kronos3d_MainActivity_nativeInit(JNIEnv* env, jobject obj, jobject surface, jint width, jint height);

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeResize(JNIEnv* env, jobject obj, jint width, jint height);

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeRender(JNIEnv* env, jobject obj);

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeTouchEvent(JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y, jfloat pressure);

JNIEXPORT void JNICALL
Java_com_kronos3d_MainActivity_nativeCleanup(JNIEnv* env, jobject obj);

JNIEXPORT jstring JNICALL
Java_com_kronos3d_MainActivity_nativeGetVersion(JNIEnv* env, jobject obj);

JNIEXPORT jstring JNICALL
Java_com_kronos3d_MainActivity_nativeGetRendererInfo(JNIEnv* env, jobject obj);

} // extern "C"

} // namespace kronos