#pragma once

#include <GLES3/gl32.h>
#include <EGL/egl.h>
#include <vector>
#include <string>
#include "math/vector.h"
#include "math/matrix.h"
#include "mesh/bmesh.h"

namespace kronos {
namespace opengl {

struct GLContext {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config = nullptr;
    int width = 0, height = 0;
};

struct GLMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint index_count = 0;
    GLenum primitive_type = GL_TRIANGLES;
};

struct GLShader {
    GLuint program = 0;
    GLint u_model = -1;
    GLint u_view = -1;
    GLint u_proj = -1;
    GLint u_normal_matrix = -1;
    GLint u_camera_pos = -1;
    GLint u_light_dir = -1;
    GLint u_light_color = -1;
    GLint u_light_intensity = -1;
    GLint u_ambient_intensity = -1;
    GLint u_time = -1;
};

struct Uniforms {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Mat4 normal_matrix;
    Vec3 camera_pos;
    Vec3 light_dir = {0.5f, -1.0f, 0.3f};
    Vec3 light_color = {1.0f, 1.0f, 1.0f};
    float light_intensity = 3.0f;
    float ambient_intensity = 0.2f;
    float time = 0.0f;
};

class GLRenderer {
public:
    GLRenderer() = default;
    ~GLRenderer() { cleanup(); }

    bool init(GLContext& ctx, ANativeWindow* window, int width, int height);
    void cleanup();
    void resize(int width, int height);
    void draw_frame(const Uniforms& uniforms, const GLMesh& mesh);
    GLMesh upload_mesh(const BMesh& bmesh);
    void free_mesh(GLMesh& mesh);
    GLShader load_shader(const char* vert_src, const char* frag_src);

private:
    GLContext* ctx_ = nullptr;
    GLShader default_shader_;

    bool create_context(ANativeWindow* window);
    bool load_default_shaders();
    static GLuint compile_shader(GLenum type, const char* source);
    static GLuint link_program(GLuint vert, GLuint frag);
    static std::string read_asset(const char* path);
    void check_gl_error(const char* op);
};

// PBR Shader Sources (GLSL ES 3.2)
constexpr const char* PBR_VERTEX_SHADER = R"(#version 320 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uNormalMatrix;

out vec3 WorldPos;
out vec3 Normal;
out vec2 UV;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;
    Normal = normalize(mat3(uNormalMatrix) * aNormal);
    UV = aUV;
    gl_Position = uProj * uView * worldPos;
}
)";

constexpr const char* PBR_FRAGMENT_SHADER = R"(#version 320 es
precision highp float;

in vec3 WorldPos;
in vec3 Normal;
in vec2 UV;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientIntensity;
uniform float uTime;

out vec4 FragColor;

const float PI = 3.14159265359;

vec3 albedo = vec3(0.8, 0.1, 0.1);
float metallic = 0.0;
float roughness = 0.5;
float ao = 1.0;

vec3 F_Schlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float D_GGX(float NdH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdH2 = NdH * NdH;
    float denom = (NdH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float geo_Smith(float NdV, float NdL, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float gv = NdV * (1.0 - k) + k;
    float gl = NdL * (1.0 - k) + k;
    return 0.5 / (gv * gl);
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(uCameraPos - WorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    float NdV = max(dot(N, V), 0.001);
    float NdL = max(dot(N, L), 0.0);
    float NdH = max(dot(N, H), 0.0);
    float VdH = max(dot(V, H), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F_Schlick(F0, VdH);

    float D = D_GGX(NdH, roughness);

    float G = geo_Smith(NdV, NdL, roughness);

    vec3 num = D * G * F;
    float den = 4.0 * NdV * NdL + 0.001;
    vec3 specular = num / den;

    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    vec3 radiance = uLightColor * uLightIntensity;
    vec3 Lo = (diffuse + specular) * radiance * NdL;

    vec3 ambient = vec3(0.03) * albedo * ao * uAmbientIntensity;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
)";

// Simple unlit shader for wireframe/debug
constexpr const char* UNLIT_VERTEX_SHADER = R"(#version 320 es
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)";

constexpr const char* UNLIT_FRAGMENT_SHADER = R"(#version 320 es
precision highp float;
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

} // namespace opengl
} // namespace kronos