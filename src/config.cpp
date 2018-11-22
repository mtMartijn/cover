#include "config.h"

#include <fstream>
#include <algorithm>

#include "json.hpp"
#include "stb_image_write.h"

namespace mt {

  bool init = true;
  bool show_attr = false;
  bool show_nodes = true;
  bool export_png = false;
  bool render_front = false;

  // Structures for tagged union for parameter types.
  enum var_type { FLOAT, VEC, INT };
  union var {
    float f;
    struct { float x,y,z; } v;
    int i;
  };

  // Tagged union of parameters.
  struct param {
    std::string key;
    mt::var_type tag;
    mt::var value, min, max;
  };

  static std::vector<mt::param> atlas; // Atlas of all parameters.
  static int curr = 0; // Index of current parameter being changed.
  static std::string file_name; // Name of json file.

  static std::string json_file(const std::string& file) { return file + ".json"; }
  static std::string json_file_path(const std::string& file) { return conf_dir + file + ".json"; }

  // Initialize a float parameter.
  static void init_param(const std::string& key, float val, float min, float max) {
    mt::param p = { key, FLOAT, val, min, max };
    atlas.push_back(p);
  }

  // Initialize a vec3 parameter.
  static void init_param(const std::string& key, glm::vec3 val, glm::vec3 min, glm::vec3 max) {
    mt::param p;
    p.key = key;
    p.tag = VEC;
    p.value.v = { val.x, val.y, val.z };
    p.min.v = { min.x, min.y, min.z };
    p.max.v = { max.x, max.y, max.z };
    atlas.push_back(p);
  }

  // Initialize an int or boolean parameter.
  static void init_param(const std::string& key, int val, int min = 0, int max = 1) {
    mt::param p;
    p.key = key;
    p.tag = INT;
    p.value.i = val;
    p.min.i = min;
    p.max.i = max;
    atlas.push_back(p);
  }

  // Return param in atlas.
  static mt::param* find_param(const std::string& key) {
    auto find_func = [key] (const mt::param& en) { return en.key == key; };
    auto iter = std::find_if(atlas.begin(), atlas.end(), find_func);
    if (iter != atlas.end())
      return &*iter;
    else
      return nullptr;
  }

  // Initialize parameters and load from file if provided.
  void load_params(const std::string& file) {
    init_param("g_camera", {0.f, 90.f, 4.f}, {-720.f, 0.1f, -50.f}, {720.f, 180.f, 50.f});
    init_param("g_height", 0.5f, -10.f, 10.f);
    init_param("g_seed", 0, 0, 99999);
    init_param("tree_bark_col_a", {53.f, 70.f, 40.f}, {0.f, 0.f, 0.f}, {100.f, 150.f, 360.f});
    init_param("tree_bark_col_b", {53.f, 70.f, 40.f}, {0.f, 0.f, 0.f}, {100.f, 150.f, 360.f});
    init_param("tree_bark_opac", 1.f, 0.f, 10.f);
    init_param("tree_bark_add", 0.f, 0.f, 1.f);
    init_param("tree_leaves_col_a", {89.f, 32.f, 128.f}, {0.f, 0.f, 0.f}, {100.f, 150.f, 360.f});
    init_param("tree_leaves_col_b", {89.f, 32.f, 128.f}, {0.f, 0.f, 0.f}, {100.f, 150.f, 360.f});
    init_param("tree_leaves_opac", 1.f, 0.f, 10.f);
    init_param("tree_leaves_add", 0.f, 0.f, 1.f);
    init_param("tree_leaves_size", 0.003f, 0.f, 0.2f);
    init_param("tree_attr_col", {50.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, {100.f, 150.f, 360.f});
    init_param("tree_attr_size", 0.003f, 0.f, 0.2f);
    init_param("back_bg_col", {1.f, 1.f, 1.f}, {0.f, 0.f, 0.f}, {2.f, 2.f, 2.f});
    init_param("back_cloud_col", {1.f, 1.f, 1.f}, {0.f, 0.f, 0.f}, {2.f, 2.f, 2.f});
    init_param("back_cloud_strength", 0.5f, 0.f, 1.f);
    init_param("back_blur_rad", 1.f, 0.f, 5.f);
    init_param("back_exposure", 1.f, 0.f, 20.f);
    init_param("back_waterline", -0.25f, -2.f, 2.f);
    init_param("back_sun_pos", {-0.2f, 0.3f, 0.f}, {-2.f, -2.f, 0.f}, {2.f, 2.f, 1.f});
    init_param("back_sun_col", {0.3f, 0.25f, 0.1f}, {0.f, 0.f, 0.f}, {2.f, 2.f, 2.f});
    init_param("back_sun_radius", 0.03f, 0.001f, 1.f);
    init_param("back_sun_strength", 0.2f, 0.001f, 5.f);
    init_param("front_blur_rad", 1.f, 0.f, 5.f);
    init_param("front_paper_col", {1.f, 1.f, 0.92f}, {0.f, 0.f, 0.f}, {2.f, 2.f, 2.f});


    file_name = file;
    std::ifstream in(json_file_path(file_name));
    if(!in.fail()) {
      nlohmann::json j;
      in >> j;
      for (auto it = j.begin(); it != j.end(); ++it) {
        auto p = find_param(it.key());
        if (!p) continue;
        switch (p->tag) {
          case FLOAT:
            p->value.f = it.value();
            break;
          case VEC:
            p->value.v = { it.value()[0], it.value()[1], it.value()[2] };
            break;
          case INT:
            p->value.i = it.value();
            break;
        }
      }
    }
  }

  // Convert parameter atlas to json string.
  static nlohmann::json convert_json() {
    nlohmann::json j;
    for (size_t i = 0; i < atlas.size(); ++i) {
      auto& p = atlas[i];
      switch (p.tag) {
        case FLOAT:
          j[p.key] = p.value.f;
          break;
        case VEC: {
          auto& v = p.value.v;
          j[p.key] = { v.x, v.y, v.z };
          break;
        }
        case INT:
          j[p.key] = p.value.i;
          break;
      }
    }
    return j;
  }

  // Save to a json file or discard.
  void save_params() {
    std::string line;
    do {
      std::cout << "Save / discard [s/D]? ";
      std::getline(std::cin, line);
    } while (line != "s" && line != "d" && line != "");
    if (line == "s") {
      do {
        std::cout << "Save to '" << json_file(file_name) << "'. [Y/n]? ";
        std::getline(std::cin, line);
      } while (line != "y" && line != "n" && line != "");
      if (line == "n") {
        do {
          std::cout << "Enter file name [<name>.json]: ";
          std::getline(std::cin, line);
        } while (line == "");
        file_name = line;
      }
      auto j = convert_json();
      std::ofstream out(json_file_path(file_name));
      out << std::setw(2) << j << std::endl;
      std::cout << "Saved." << std::endl;
    }
  }

  // Get a float parameter.
  float paramf(const std::string& key) {
    auto ptr = find_param(key);
    if (ptr)
      return ptr->value.f;
    else {
      std::cerr << "Can't find entry." << std::endl;
      return 0.f;
    }
  }

  // Get a vec3 parameter.
  glm::vec3 paramv(const std::string& key) {
    auto ptr = find_param(key);
    if (ptr) {
      auto v = ptr->value.v;
      return glm::vec3(v.x, v.y, v.z);
    } else {
      std::cerr << "Can't find entry." << std::endl;
      return glm::vec3(0.f);
    }
  }

  // Get an int parameter.
  int parami(const std::string& key) {
    auto ptr = find_param(key);
    if (ptr)
      return ptr->value.i;
    else {
      std::cerr << "Can't find entry." << std::endl;
      return 0;
    }
  }

  // Print the current parameter being manipulated.
  static void print_keys() {
    size_t width = 0;
    for (size_t i = 0; i < atlas.size(); ++i) {
      width = std::max(atlas[i].key.length(), width);
    }
    for (int i = curr-3; i < curr+4; ++i) {
      if (i == curr)
        std::cout << std::left << std::setw(width+10) << ">> " + atlas[i%atlas.size()].key + " <<";
      else
        std::cout << std::left << std::setw(width+2) << atlas[i%atlas.size()].key;
    }
    std::cout << std::endl;
  }

  // Go up one index.
  static void up_key() {
    curr = (curr+1) % atlas.size();
    print_keys();
  }

  // Go down one index.
  static void down_key() {
    curr--;
    if (curr < 0) curr += atlas.size();
    print_keys();
  }

#define KEY(k, expr) case k: expr; break

  // GLFW key callback.
  void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      switch(key) {
        // Close window.
        KEY(GLFW_KEY_ESCAPE, glfwSetWindowShouldClose(window, GLFW_TRUE));
        // Modify next parameter.
        KEY(GLFW_KEY_RIGHT, up_key());
        // Modify previous parameter.
        KEY(GLFW_KEY_LEFT, down_key());
        // Re-initialize graphics objects.
        KEY(GLFW_KEY_ENTER, mt::init = true);
        // Re-initialize grahpics objects and increment seed.
        KEY(GLFW_KEY_SPACE, mt::init = true; find_param("g_seed")->value.i++);
        // Show/hide attractors in colonization algo.
        KEY(GLFW_KEY_A, mt::show_attr = !mt::show_attr);
        // Show/hide nodes in colonization algo.
        KEY(GLFW_KEY_N, mt::show_nodes = !mt::show_nodes);
        // Export png file.
        KEY(GLFW_KEY_P, mt::export_png = true);
        // Switch cover sides.
        KEY(GLFW_KEY_TAB, mt::render_front = !mt::render_front);
      }
    }
  }

  // GLFW scroll callback.
  void scroll_cb(GLFWwindow* window, double xoffset, double yoffset) {
    auto& param = atlas[curr];
    if (param.tag == VEC) {
      float unit = (param.max.v.z - param.min.v.z) / 100.f;
      if (yoffset == 1) param.value.v.z += unit;
      else if (yoffset == -1) param.value.v.z -= unit;
      param.value.v.z = glm::clamp(param.value.v.z, param.min.v.z, param.max.v.z);
    }
  }

  // Cursor never goes below 0 so this double-serves as a boolean.
  static glm::dvec2 start = { -1, -1 };
  static mt::param param_start;

  // GLFW click callback.
  void click_cb(GLFWwindow* window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
      if(action == GLFW_PRESS) {
        glfwGetCursorPos(window, &start.x, &start.y);
        param_start = atlas[curr];
      } else if(action == GLFW_RELEASE) {
        start = { -1, -1 };
        param_start = {};
        std::cout << std::endl;
      }
    }
  }

#define VEC3(v) glm::vec3(v.x, v.y, v.z)

  // GLFW drag callback.
  void drag_cb(GLFWwindow* window, double xpos, double ypos) {
    if (start.x > -1) {
      glm::dvec2 factor = glm::dvec2(xpos-start.x, ypos-start.y);
      factor /= mt::window_size.y;
      switch (param_start.tag) {
        case FLOAT: {
          float scale = param_start.max.f - param_start.min.f;
          float val = param_start.value.f - factor.y*scale;
          val = glm::clamp(val, param_start.min.f, param_start.max.f);
          std::cout << "\r" << param_start.key << ": " << std::setw(10) << std::left << val << std::flush;
          atlas[curr].value.f = val;
          break;
        }
        case VEC: {
          glm::vec3 scale = VEC3(param_start.max.v) - VEC3(param_start.min.v);

          float valx = param_start.value.v.x - factor.x*scale.x;
          float valy = param_start.value.v.y - factor.y*scale.y;
          float valz = param_start.value.v.z;

          valx = glm::clamp(valx, param_start.min.v.x, param_start.max.v.x);
          valy = glm::clamp(valy, param_start.min.v.y, param_start.max.v.y);

          auto word = glm::to_string(glm::vec3(valx, valy, valz));
          std::cout << "\r" << param_start.key << ": " << std::setw(50) << std::left << word << std::flush;

          atlas[curr].value.v = { valx, valy, valz };
          break;
        }
        case INT: {
          float val = (float)param_start.value.i - (ypos-start.y)/30.f;
          val = glm::clamp(val, (float)param_start.min.i, (float)param_start.max.i);
          std::cout << "\r" << param_start.key << ": " << std::setw(10) << std::left << (int)val << std::flush;
          atlas[curr].value.i = (int)val;
          break;
        }
      }
    }
  }

  // Return camera position.
  glm::vec3 camera() {
    auto cam = mt::paramv("g_camera");
    float polar = -glm::radians(cam.x-90.f);
    float azi = glm::radians(cam.y);
    glm::vec3 v = glm::vec3(sin(azi)*cos(polar), cos(azi), sin(azi)*sin(polar));
    return cam.z*v;
  }

}

