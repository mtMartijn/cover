#pragma once

#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "glutils.h"
#include "config.h"
#include "algo.h"

namespace mt {

  // Structure which holds data of a single particle.
  struct atom {
    atom():
      _pos(0.f),
      _cola(100.f, 0.f, 0.f, 1.f),
      _colb(100.f, 0.f, 0.f, 0.f),
      _attr(0.f),
      _dir(),
      id(0) {
    }

    glm::vec4 _pos;
    glm::vec4 _cola;
    glm::vec4 _colb;
    glm::vec4 _attr;
    glm::quat _dir;
    size_t id;

    void pos(const glm::vec3& p) { _pos.x = p.x; _pos.y = p.y; _pos.z = p.z; }
    glm::vec3 pos() const { return glm::vec3(_pos.x, _pos.y, _pos.z); }
    void size(float s) { _pos.w = s; }
    void cola(const glm::vec3& c) { _cola.x = c.x; _cola.y = c.y; _cola.z = c.z; }
    void colb(const glm::vec3& c) { _colb.x = c.x; _colb.y = c.y; _colb.z = c.z; }
    void opac(float o) { _cola.w = o; }
    void add(float a) { _colb.w = a; }
    void type(uint32_t t) { _attr.x = (float)t; }
    void seed(uint32_t s) { _attr.y = (float)s; }
    void dir(const glm::quat& q) { _dir.x = q.x; _dir.y = q.y; _dir.z = q.z; _dir.w = q.w; }
  };

  // Forward declaration.
  class block;

  // Structure which holds a buffer of all particles.
  class data {
  public:
    data(size_t num);
    ~data();

    void init();
    void send();
    void bsort(size_t k = 1);

    friend mt::block;

  /* private: */
    std::vector<mt::atom> buffer;
    std::vector<size_t> order;
    size_t total_num;
    size_t alive_num;

    gl::shader shader;
    GLuint vao_id;
    GLuint vbo_ids[2];
  };

  // A 'projection' into the data buffer.
  class block {
  public:
    block(mt::data* d = nullptr): ptr(d) {}
    ~block() {}

    void alloc(size_t num) { while (len() < num) index.push_back(ptr->alive_num++); }
    size_t len() const { return index.size(); }
    mt::atom& operator[](size_t i) { return ptr->buffer[ptr->order[index[i]]]; }
    void clear() { index.clear(); }

  private:
    std::vector<size_t> index;
    mt::data* ptr;
  };

  // Tree object using the space colonization algorithm.
  class tree {
  public:
    tree(mt::data* d);
    ~tree() {}

    void init();
    void step();

    mt::block nodes;
    mt::block attr;
    mt::block leaves;
    mt::colony colony;
  };

  // Back cover shader step.
  class back {
  public:
    back();
    void draw(GLuint id);

  private:
    gl::shader shader;
    gl::fbo canvas;
  };

  // Front cover shader step.
  class front {
  public:
    front();
    void draw(GLuint id);

  private:
    gl::shader shader;
    gl::fbo canvas;
  };

  // Gaussian blur object.
  class blur {
  public:
    blur();
    void enable();
    void disable();
    void process(float radius);
    void draw();
    GLuint id() { return pp[0].get_ID(); }

  private:
    gl::shader shader;
    gl::fbo pp[2];
  };

}
