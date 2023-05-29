#include <cstdio>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <vector>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void do_movement(GLfloat deltaTime);

const GLuint WIDTH = 800, HEIGHT = 600;
const float aspectratio = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

float lastX {0.0f};
float lastY {0.0f};
float xRotationSpeed = 2 * glm::pi<float>() / WIDTH;
float yRotationSpeed = glm::pi<float>() / HEIGHT;
bool keys[1024];

struct PointLight {
  glm::vec3 position;

  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;

  float constant;
  float linear;
  float quadratic;
};

int main(int argc, char** argv)
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Model Viewer", nullptr, nullptr);
  if (window == nullptr)
  {
    std::printf("Failed to create GLFW window\n" );
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  glfwSetKeyCallback(window, key_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);

  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
  {
    std::printf("Failed to initialize OpenGL context\n" );
    return -1;
  }

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
  
  tinygltf::Primitive triangles = gltfmodel.meshes[0].primitives[0];
  const int positionAttr = triangles.attributes.find("POSITION")->second;
  tinygltf::Accessor posAccessor = gltfmodel.accessors[positionAttr];
  tinygltf::BufferView posBufView = gltfmodel.bufferViews[posAccessor.bufferView];
  tinygltf::Buffer posBuf = gltfmodel.buffers[gltfmodel.bufferViews[posAccessor.bufferView].buffer];

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint meshBuf;
  glCreateBuffers(1, &meshBuf);
  glNamedBufferStorage(meshBuf, posBuf.data.size(), posBuf.data.data(), 0);

  glVertexArrayVertexBuffer(vao, 0, meshBuf, posBufView.byteOffset, posBufView.byteStride);

  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribFormat(vao, 0, 3, posAccessor.componentType, GL_FALSE, posAccessor.byteOffset);
  glVertexArrayAttribBinding(vao, 0, 0);

  glViewport(0, 0, WIDTH, HEIGHT);

  glEnable(GL_DEPTH_TEST);

  while(!glfwWindowShouldClose(window))
  {
    GLfloat currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glfwPollEvents();
    do_movement(deltaTime);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transformation matrices
    glm::mat4 projection = glm::perspective(90.0f, aspectratio, 0.1f, 100.0f);
    glm::mat4 view(1.0f);
    glm::mat4 model(1.0f);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(triangles.mode, 0, posAccessor.count);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(triangles.mode, 0, posAccessor.count);

    glfwSwapBuffers(window);
  }

  // Cleanup
  glfwTerminate();
  return 0;
}

void do_movement(GLfloat deltaTime)
{
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
  if (action == GLFW_PRESS)
    keys[key] = true;
  if (action == GLFW_RELEASE)
    keys[key] = false;
}

bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
  if (firstMouse)
  {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float deltaX = xpos - lastX;
  // Invert Y
  float deltaY = lastY - ypos;

  lastX = xpos;
  lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
}
