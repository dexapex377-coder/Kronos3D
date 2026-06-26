#include "renderer.h"
#include <android/native_window.h>
#include <android/log.h>
#include <cstring>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "Kronos3D-GL", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Kronos3D-GL", __VA_ARGS__)

namespace kronos {
namespace opengl {

bool GLRenderer::init(GLContext& ctx, ANativeWindow* window, int width, int height) {
    ctx_ = &ctx;
    ctx_->width = width;
    ctx_->height = height;

    ctx_->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ctx_->display == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(ctx_->display, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        return false;
    }

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(ctx_->display, attribs, &ctx_->config, 1, &num_configs) || num_configs == 0) {
        LOGE("Failed to choose EGL config");
        return false;
    }

    ctx_->surface = eglCreateWindowSurface(ctx_->display, ctx_->config, window, nullptr);
    if (ctx_->surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    ctx_->context = eglCreateContext(ctx_->display, ctx_->config, EGL_NO_CONTEXT, context_attribs);
    if (ctx_->context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }

    if (!eglMakeCurrent(ctx_->display, ctx_->surface, ctx_->surface, ctx_->context)) {
        LOGE("Failed to make EGL current");
        return false;
    }

    LOGI("EGL initialized: %d.%d, GL vendor: %s, renderer: %s",
         major, minor,
         glGetString(GL_VENDOR),
         glGetString(GL_RENDERER));

    check_gl_error("init");
    return load_default_shaders();
}

void GLRenderer::cleanup() {
    if (!ctx_) return;
    if (default_shader_.program) {
        glDeleteProgram(default_shader_.program);
        default_shader_.program = 0;
    }
    if (ctx_->context != EGL_NO_CONTEXT) {
        eglMakeCurrent(ctx_->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(ctx_->display, ctx_->context);
    }
    if (ctx_->surface != EGL_NO_SURFACE) eglDestroySurface(ctx_->display, ctx_->surface);
    if (ctx_->display != EGL_NO_DISPLAY) eglTerminate(ctx_->display);
}

void GLRenderer::resize(int width, int height) {
    if (!ctx_) return;
    ctx_->width = width;
    ctx_->height = height;
    glViewport(0, 0, width, height);
}

void GLRenderer::draw_frame(const Uniforms& uniforms, const GLMesh& mesh) {
    if (!default_shader_.program || !mesh.vao) return;

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glUseProgram(default_shader_.program);

    glUniformMatrix4fv(default_shader_.u_model, 1, GL_FALSE, &uniforms.model.m[0][0]);
    glUniformMatrix4fv(default_shader_.u_view, 1, GL_FALSE, &uniforms.view.m[0][0]);
    glUniformMatrix4fv(default_shader_.u_proj, 1, GL_FALSE, &uniforms.proj.m[0][0]);
    glUniformMatrix4fv(default_shader_.u_normal_matrix, 1, GL_FALSE, &uniforms.normal_matrix.m[0][0]);
    glUniform3f(default_shader_.u_camera_pos, uniforms.camera_pos.x, uniforms.camera_pos.y, uniforms.camera_pos.z);
    glUniform3f(default_shader_.u_light_dir, uniforms.light_dir.x, uniforms.light_dir.y, uniforms.light_dir.z);
    glUniform3f(default_shader_.u_light_color, uniforms.light_color.x, uniforms.light_color.y, uniforms.light_color.z);
    glUniform1f(default_shader_.u_light_intensity, uniforms.light_intensity);
    glUniform1f(default_shader_.u_ambient_intensity, uniforms.ambient_intensity);
    glUniform1f(default_shader_.u_time, uniforms.time);

    glBindVertexArray(mesh.vao);
    glDrawElements(mesh.primitive_type, mesh.index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    eglSwapBuffers(ctx_->display, ctx_->surface);
    check_gl_error("draw_frame");
}

GLMesh GLRenderer::upload_mesh(const BMesh& bmesh) {
    GLMesh mesh;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (const auto& v : bmesh.verts) {
        vertices.push_back(v.co.x);
        vertices.push_back(v.co.y);
        vertices.push_back(v.co.z);
        vertices.push_back(v.no.x);
        vertices.push_back(v.no.y);
        vertices.push_back(v.no.z);
        vertices.push_back(0.0f); // UV u
        vertices.push_back(0.0f); // UV v
    }

    for (const auto& f : bmesh.faces) {
        BMLoop* l = f.loops;
        if (!l) continue;
        if (f.len == 3) {
            indices.push_back(l->vert->index);
            indices.push_back(l->next->vert->index);
            indices.push_back(l->next->next->vert->index);
        } else if (f.len == 4) {
            indices.push_back(l->vert->index);
            indices.push_back(l->next->vert->index);
            indices.push_back(l->next->next->vert->index);
            indices.push_back(l->next->next->next->vert->index);
        }
    }

    mesh.index_count = (GLuint)indices.size();
    mesh.primitive_type = GL_TRIANGLES;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position (location=0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal (location=1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // UV (location=2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    check_gl_error("upload_mesh");
    return mesh;
}

void GLRenderer::free_mesh(GLMesh& mesh) {
    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
    mesh = {};
}

GLShader GLRenderer::load_shader(const char* vert_src, const char* frag_src) {
    GLShader shader;
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!vert || !frag) return shader;

    shader.program = link_program(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    if (shader.program) {
        shader.u_model = glGetUniformLocation(shader.program, "uModel");
        shader.u_view = glGetUniformLocation(shader.program, "uView");
        shader.u_proj = glGetUniformLocation(shader.program, "uProj");
        shader.u_normal_matrix = glGetUniformLocation(shader.program, "uNormalMatrix");
        shader.u_camera_pos = glGetUniformLocation(shader.program, "uCameraPos");
        shader.u_light_dir = glGetUniformLocation(shader.program, "uLightDir");
        shader.u_light_color = glGetUniformLocation(shader.program, "uLightColor");
        shader.u_light_intensity = glGetUniformLocation(shader.program, "uLightIntensity");
        shader.u_ambient_intensity = glGetUniformLocation(shader.program, "uAmbientIntensity");
        shader.u_time = glGetUniformLocation(shader.program, "uTime");
    }
    return shader;
}

bool GLRenderer::load_default_shaders() {
    default_shader_ = load_shader(PBR_VERTEX_SHADER, PBR_FRAGMENT_SHADER);
    if (!default_shader_.program) {
        LOGE("Failed to load PBR shaders");
        return false;
    }
    return true;
}

GLuint GLRenderer::compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(shader, sizeof(info), nullptr, info);
        LOGE("Shader compile error (%s): %s",
             type == GL_VERTEX_SHADER ? "vertex" : "fragment", info);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint GLRenderer::link_program(GLuint vert, GLuint frag) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetProgramInfoLog(program, sizeof(info), nullptr, info);
        LOGE("Program link error: %s", info);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void GLRenderer::check_gl_error(const char* op) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOGE("GL error after %s: 0x%x", op, err);
    }
}

} // namespace opengl
} // namespace kronos