#include <iostream>
#include <cassert>
#include <ctime>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "objects.h"

// GLFW error callback.
static void error_callback(int error, const char* description) {
  std::cerr << "GLFW Error: " << description << std::endl;
}

int main(int argc, char *argv[]) {
  // Parse file name.
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
    exit(EXIT_FAILURE);
  }
  mt::load_params(argv[1]);

  // GLFW setup.
  glfwSetErrorCallback(error_callback);
  assert(glfwInit());

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  // Create window.
  GLFWwindow* window = glfwCreateWindow(mt::window_size.x, mt::window_size.y, 
      "cover", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Set callbacks (defined in config.cpp).
  glfwSetKeyCallback(window, mt::key_cb);
  glfwSetScrollCallback(window, mt::scroll_cb);
  glfwSetMouseButtonCallback(window, mt::click_cb);
  glfwSetCursorPosCallback(window, mt::drag_cb);

  // GLEW setup.
  glewExperimental = true; // Needed for core profile.
  assert(glewInit() == GLEW_OK);

  // Global OpenGL settings.
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Needs premultiplied alpha.
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glViewport(0, 0, mt::window_size.x, mt::window_size.y);

  // Buffer for reading raw pixels.
  std::vector<char> buffer(mt::window_size.x * mt::window_size.y * 4);

  // Graphics objects.
  mt::data data(mt::max_atoms);
  mt::tree tree(&data);
  mt::back back;
  mt::front front;
  mt::blur blur;
  mt::blur blur2;

  // Render loop.
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (mt::init) {
      std::srand(mt::parami("g_seed")); // Set seed for reproducability.
      tree.init();
      data.init();
      mt::init = false;
    }

    // Draw objects.
    blur.enable();
    tree.step();
    data.send();
    blur.disable();
    blur.process(mt::paramf("back_blur_rad"));

    if (mt::render_front) blur2.enable();
    back.draw(blur.id());
    if (mt::render_front) {
      blur2.disable();
      blur2.process(mt::paramf("front_blur_rad"));
      blur2.process(mt::paramf("front_blur_rad"));
      blur2.process(mt::paramf("front_blur_rad"));
      blur2.process(mt::paramf("front_blur_rad"));
      front.draw(blur2.id());
    }

    // Export pixels to png.
    if (mt::export_png) {
      int w = (int)mt::window_size.x;
      int h = (int)mt::window_size.y;

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, &buffer[0]);
      time_t rawtime;
      time(&rawtime);
      struct tm* timeinfo = localtime(&rawtime);
      char tbuffer[80];
      strftime(tbuffer, 80, "%y%m%d_%H%M%S_", timeinfo);
      std::string png_name(tbuffer);
      png_name += argv[1];
      png_name += ".png";

      stbi_write_png((mt::conf_dir + png_name).c_str(), w, h, 4, 
          &buffer[0] + (w * 4 * (h - 1)), -w * 4);

      std::cout << "Exported image: " << png_name << std::endl;
      mt::export_png = false;
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  mt::save_params();
  exit(EXIT_SUCCESS);
}
