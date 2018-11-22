#pragma once

#include <iostream>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>

#define COUT(x) std::cout << #x << ": " << x << std::endl;

// Convert mm to pixels using 300 DPI.
#define MM2P(x) ((x)*(300.f/25.4))

namespace mt {

  // Constant globals.
  const float bleed = 2.f; // 2 mm bleed.
  const glm::vec2 window_size = 
    glm::vec2(MM2P(170.f + 2*bleed), MM2P(240.f + 2*bleed)); // B5 format.
  const float spine = MM2P(7.47f); // 7.47 mm spine (121 pages).
  const std::string conf_dir = "data/";
  const std::string glsl_dir = "glsl/";
  const double pi = glm::pi<double>();
  const double tau = glm::pi<double>()*2.0;
  const size_t max_atoms = 1000000;

  // Global variables.
  extern bool init;
  extern bool show_attr;
  extern bool show_nodes;
  extern bool export_png;
  extern bool render_front;

  // Callbacks.
  void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);
  void scroll_cb(GLFWwindow* window, double xoffset, double yoffset);
  void click_cb(GLFWwindow* window, int button, int action, int mods);
  void drag_cb(GLFWwindow* window, double xpos, double ypos);

  // Edit/load/get json parameters.
  void load_params(const std::string& file);
  void save_params();
  float paramf(const std::string& key);
  glm::vec3 paramv(const std::string& key);
  int parami(const std::string& key);

  // Utils.
  glm::vec3 camera();

}
