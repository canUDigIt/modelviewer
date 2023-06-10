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

struct Camera {
  glm::vec3 pos;
  glm::quat orientation;
};

struct MouseState {
  glm::vec2 pos {0.0f};
  bool pressedLeft = false;
} mouseState;

struct CameraMovement {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
  bool fastSpeed = false;
  bool resetUp = false;
  float lookSpeed = 4.0f;
  float acceleration = 150.0f;
  float damping = 0.2f;
  float maxSpeed = 10.0f;
  float fastCoef = 10.0f;
  glm::vec3 moveSpeed {0.0f};
} cameraMovement;

glm::mat4 getViewMatrix(const Camera& camera)
{
  const glm::mat4 t = glm::translate(glm::mat4{1.0f}, -camera.pos);
  const glm::mat4 r = glm::mat4_cast(camera.orientation);
  return r * t;
}

void setUpVector(Camera& camera, const glm::vec3& up)
{
  const glm::mat4 view = getViewMatrix(camera);
  const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
  camera.orientation = glm::lookAt(camera.pos, camera.pos + dir, up);
}

void resetMousePosition(MouseState& ms, const glm::vec2& p)
{
  ms.pos = p;
}

void updateCamera(Camera& camera, double deltaSeconds, const MouseState& newState, MouseState& oldState, const CameraMovement& movement)
{
  if (cameraMovement.resetUp) setUpVector(camera, glm::vec3{0.0f, 1.0f, 0.0f});

  if (mouseState.pressedLeft)
  {
    const glm::vec2 delta = newState.pos - oldState.pos;
    const glm::quat deltaQuat = glm::quat(glm::vec3(movement.lookSpeed * delta.y, movement.lookSpeed * delta.x, 0.0f));
    camera.orientation = glm::normalize(deltaQuat * camera.orientation);
  }
  oldState = newState;

  const glm::mat4 v = glm::mat4_cast(camera.orientation);
  const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
  const glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
  const glm::vec3 up = glm::cross(right, forward);

  glm::vec3 accel {0.0f};
  if (cameraMovement.forward) accel += forward;
  if (cameraMovement.backward) accel -= forward;
  if (cameraMovement.left) accel -= right;
  if (cameraMovement.right) accel += right;
  if (cameraMovement.up) accel += up;
  if (cameraMovement.down) accel -= up;
  if (cameraMovement.fastSpeed) accel *= cameraMovement.fastCoef;

  if (accel == glm::zero<glm::vec3>())
  {
    cameraMovement.moveSpeed -= cameraMovement.moveSpeed * std::min((1.0f / cameraMovement.damping) * static_cast<float>(deltaSeconds), 1.0f);
  }
  else
  {
    cameraMovement.moveSpeed += accel * cameraMovement.acceleration * static_cast<float>(deltaSeconds);
    const float maxSpeed = cameraMovement.fastSpeed ? cameraMovement.maxSpeed * cameraMovement.fastCoef : cameraMovement.maxSpeed;
    if (glm::length(cameraMovement.moveSpeed) > maxSpeed)
    {
      cameraMovement.moveSpeed = glm::normalize(cameraMovement.moveSpeed) * maxSpeed;
    }

    camera.pos += cameraMovement.moveSpeed * static_cast<float>(deltaSeconds);
  }
}

int main(int argc, char** argv)
{
  glm::vec3 camPos {-2.0f, 1.0f, 3.0f};
  glm::vec3 target {0.5f, 0.5f, 0.0f};
  glm::vec3 up {0.0f, 0.0f, 1.0f};
  Camera camera { camPos, glm::lookAt(camPos, target, up) };

  MouseState oldMouseState;

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

  glfwSetCursorPosCallback(window, [] (GLFWwindow* window, double x, double y) {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      mouseState.pos.x = static_cast<float>(x / width);
      mouseState.pos.y = static_cast<float>(y / width);
  });

  glfwSetMouseButtonCallback(window, [] (GLFWwindow* window, int button, int action, int mods) {
      if (button == GLFW_MOUSE_BUTTON_LEFT)
      {
        mouseState.pressedLeft = action == GLFW_PRESS;
      }
  });

  glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int scancode, int action, int mods) {
      const bool press = action != GLFW_RELEASE;
      if (key == GLFW_KEY_ESCAPE)
      {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
      if (key == GLFW_KEY_W)
      {
        cameraMovement.forward = press;
      }
      if (key == GLFW_KEY_S)
      {
        cameraMovement.backward = press;
      }
      if (key == GLFW_KEY_A)
      {
        cameraMovement.left = press;
      }
      if (key == GLFW_KEY_D)
      {
        cameraMovement.right = press;
      }
      if (key == GLFW_KEY_1)
      {
        cameraMovement.up = press;
      }
      if (key == GLFW_KEY_2)
      {
        cameraMovement.down = press;
      }
      if (key == GLFW_MOD_SHIFT)
      {
        cameraMovement.fastSpeed = press;
      }
      if (key == GLFW_KEY_SPACE)
      {
        cameraMovement.resetUp = press;
      }
  });

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

uniform mat4 mvp;
  
void main()
{
    gl_Position = mvp * vec4(aPos, 1.0);
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

  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 proj = glm::perspectiveRH(45.0f, WIDTH / (float)HEIGHT, 1.0f, 100.0f);
  GLint mvpLoc = glGetUniformLocation(program, "mvp");

  double lastUpdate = 0.0;

  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    double currentUpdate = glfwGetTime();
    double deltaSeconds = currentUpdate - lastUpdate;
    lastUpdate = currentUpdate;

    updateCamera(camera, deltaSeconds, mouseState, oldMouseState, cameraMovement);
    glm::mat4 view = getViewMatrix(camera);
    glm::mat4 mvp = proj * view * model;

    const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float depth = 1.0f;
    glClearBufferfv(GL_COLOR, 0, color);
    glClearBufferfv(GL_DEPTH, 0, &depth);

    glUseProgram(program);
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(vao);
    glDrawElements(triangles.mode, indAccessor.count, indAccessor.componentType, (void *)(indBufView.byteOffset + indAccessor.byteOffset));
    glfwSwapBuffers(window);
  }

  // Cleanup
  glfwTerminate();
  return 0;
}
