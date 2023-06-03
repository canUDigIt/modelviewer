#include <cstdio>

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

const GLuint WIDTH = 800, HEIGHT = 600;

void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
  auto const src_str = [source]() {
    switch (source)
    {
      case GL_DEBUG_SOURCE_API: return "API";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
      case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
      case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
      case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
      case GL_DEBUG_SOURCE_OTHER: return "OTHER";
      default: return "UNKNOWN";
    }
  }();

  auto const type_str = [type]() {
    switch (type)
    {
      case GL_DEBUG_TYPE_ERROR: return "ERROR";
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
      case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
      case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
      case GL_DEBUG_TYPE_MARKER: return "MARKER";
      case GL_DEBUG_TYPE_OTHER: return "OTHER";
      default: return "UNKNOWN";
    }
  }();

  auto const severity_str = [severity]() {
    switch (severity) {
      case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
      case GL_DEBUG_SEVERITY_LOW: return "LOW";
      case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
      case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
      default: return "UNKNOWN";
    }
  }();

  std::printf("%s, %s, %s, %d: %s\n", src_str, type_str, severity_str, id, message);
}

int main(int argc, char** argv)
{
  glfwSetErrorCallback([](int error, const char* description) {
      std::printf("Error: %s\n", description);
  });

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Model Viewer", nullptr, nullptr);
  if (!window)
  {
    std::printf("Failed to create GLFW window\n" );
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  auto version = gladLoadGL(glfwGetProcAddress);
  if (!version)
  {
    std::printf("Failed to initialize OpenGL context\n" );
    return -1;
  }
  printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
  glDebugMessageCallback(message_callback, nullptr);

  glfwSwapInterval(1);

  tinygltf::Model gltfmodel;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  const bool load_success = loader.LoadASCIIFromFile(
      &gltfmodel,
      &err,
      &warn,
      "resources/triangle.gltf"
  );

  if (!warn.empty())
  {
    std::printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty())
  {
    std::printf("Err: %s\n", err.c_str());
  }

  if (!load_success)
  {
    std::printf("Unable to load gltf\n");
    return -1;
  }
  
  static const char* vSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
  
void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

  static const char* fSource = R"(
#version 330 core
out vec4 FragColor;  
  
void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

  GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vShader, 1, &vSource, nullptr);
  glCompileShader(vShader);
  int success;
  glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char infoLog[512];
    glGetShaderInfoLog(vShader, 512, nullptr, infoLog);
    std::printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
  }


  GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fShader, 1, &fSource, nullptr);
  glCompileShader(fShader);
  glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char infoLog[512];
    glGetShaderInfoLog(fShader, 512, nullptr, infoLog);
    std::printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vShader);
  glAttachShader(program, fShader);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success)
  {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::printf("ERROR::PROGRAM::LINK_FAILED\n%s\n", infoLog);
  }

  tinygltf::Primitive triangles = gltfmodel.meshes[0].primitives[0];
  const int positionAttr = triangles.attributes.find("POSITION")->second;
  tinygltf::Accessor posAccessor = gltfmodel.accessors[positionAttr];
  tinygltf::BufferView posBufView = gltfmodel.bufferViews[posAccessor.bufferView];
  tinygltf::Buffer posBuf = gltfmodel.buffers[posBufView.buffer];
  tinygltf::Accessor indAccessor = gltfmodel.accessors[triangles.indices];
  tinygltf::BufferView indBufView = gltfmodel.bufferViews[indAccessor.bufferView];

  GLint alignment = GL_NONE;
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

  std::printf("OpenGL alignment: %d\n", alignment);

  GLuint vao;
  glCreateVertexArrays(1, &vao);

  GLuint meshBuf;
  glCreateBuffers(1, &meshBuf);

  glNamedBufferStorage(meshBuf, posBuf.data.size(), posBuf.data.data(), 0);

  glVertexArrayVertexBuffer(vao, 0, meshBuf, posBufView.byteOffset, sizeof(glm::vec3));
  glVertexArrayElementBuffer(vao, meshBuf);

  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribFormat(vao, 0, 3, posAccessor.componentType, GL_FALSE, posAccessor.byteOffset);
  glVertexArrayAttribBinding(vao, 0, 0);

  glViewport(0, 0, WIDTH, HEIGHT);

  glEnable(GL_DEPTH_TEST);

  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float depth = 1.0f;
    glClearBufferfv(GL_COLOR, 0, color);
    glClearBufferfv(GL_DEPTH, 0, &depth);

    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawElements(triangles.mode, indAccessor.count, indAccessor.componentType, (void *)(indBufView.byteOffset + indAccessor.byteOffset));
    glfwSwapBuffers(window);
  }

  // Cleanup
  glfwTerminate();
  return 0;
}
