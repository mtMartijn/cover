#include "objects.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

namespace mt {

  // Data constructor.
  data::data(size_t num): total_num(num), alive_num(0) {
    glGenVertexArrays(1, &vao_id);
    glGenBuffers(2, vbo_ids);

    shader.load_file(glsl_dir + "quad.vert", glsl_dir + "sprite.frag");
    buffer.resize(num);
    order.resize(num);
    init();

    glBindVertexArray(vao_id);

    // Allocate static quad vertices for billboarding.
    const GLuint quad_loc = 0;
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::quad::verts), gl::quad::verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(quad_loc);
    glVertexAttribPointer(quad_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(atom)*num, nullptr, GL_STREAM_DRAW);

    const GLuint pos_loc = 1;
    glEnableVertexAttribArray(pos_loc);
    glVertexAttribPointer(pos_loc, 4, GL_FLOAT, GL_FALSE, sizeof(atom), (void*)(offsetof(atom, _pos)));
    glVertexAttribDivisor(pos_loc, 1);

    const GLuint cola_loc = 2;
    glEnableVertexAttribArray(cola_loc);
    glVertexAttribPointer(cola_loc, 4, GL_FLOAT, GL_FALSE, sizeof(atom), (void*)(offsetof(atom, _cola)));
    glVertexAttribDivisor(cola_loc, 1);

    const GLuint colb_loc = 3;
    glEnableVertexAttribArray(colb_loc);
    glVertexAttribPointer(colb_loc, 4, GL_FLOAT, GL_FALSE, sizeof(atom), (void*)(offsetof(atom, _colb)));
    glVertexAttribDivisor(colb_loc, 1);

    const GLuint attr_loc = 4;
    glEnableVertexAttribArray(attr_loc);
    glVertexAttribPointer(attr_loc, 4, GL_FLOAT, GL_FALSE, sizeof(atom), (void*)(offsetof(atom, _attr)));
    glVertexAttribDivisor(attr_loc, 1);

    const GLuint dir_loc = 5;
    glEnableVertexAttribArray(dir_loc);
    glVertexAttribPointer(dir_loc, 4, GL_FLOAT, GL_FALSE, sizeof(atom), (void*)(offsetof(atom, _dir)));
    glVertexAttribDivisor(dir_loc, 1);

    glBindVertexArray(0);
  }

  // Initialize buffer data.
  void data::init() {
    alive_num = 0;
    for (size_t i = 0; i < total_num; ++i) {
      buffer[i] = mt::atom();
      buffer[i].id = i;
      order[i] = i;
    }
  }

  // Data destructor.
  data::~data() {
    glDeleteBuffers(2, vbo_ids);
    glDeleteVertexArrays(1, &vao_id);
  }

  // Send data to GPU and render.
  void data::send() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(atom)*alive_num, &buffer[0], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    float h = mt::paramf("g_height");
    auto mv = glm::lookAt(mt::camera(), glm::vec3(0.f, h, 0.f), glm::vec3(0.f, 1.f, 0.f));

    float ratio = mt::window_size.x/mt::window_size.y;
    auto proj = glm::perspective(glm::pi<float>()*0.15f, ratio, 0.1f, 100.f);

    shader.enable();
    shader.uniform("mv", mv);
    shader.uniform("proj", proj);
    shader.uniformui("rotation", false);
    glBindVertexArray(vao_id);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, alive_num);
    glBindVertexArray(0);
    shader.disable();
  }

  // Do k iterations of bubble sort.
  void data::bsort(size_t k) {
    if (alive_num < 2) return;
    auto cam = mt::camera();

    while (k--) {
      bool done = true;
      for (size_t i = 1; i < alive_num; ++i) {
        auto& p0 = buffer[i-1];
        auto& p1 = buffer[i];
        if (glm::distance(p0.pos(), cam) < glm::distance(p1.pos(), cam)) {
          std::swap(p0, p1);
          std::swap(order[p0.id], order[p1.id]);
          done = false;
        }
      }
      if (done) return;
    }
  }

  tree::tree(mt::data* d):
    nodes(d),
    attr(d),
    leaves(d) {
  }

  void tree::init() {
    colony.init();

    nodes.clear();
    attr.clear();
    leaves.clear();
  }

  static float hashf(float n) {
    return glm::fract(glm::cos(n*89.42)*343.42);
  }

  static float hashv(const glm::vec3& v) {
    return hashf(glm::dot(v, v));
  }

  static glm::vec3 cmix(const glm::vec3& a, const glm::vec3& b, float i, float p = 1.f) {
    i = glm::pow(glm::clamp(i, 0.f, 1.f), p);
    return a + i*b;
  }

  static float fmix(float a, float b, float i, float p = 1.f) {
    i = glm::pow(glm::clamp(i, 0.f, 1.f), p);
    return a + i*b;
  }

  void tree::step() {
    colony.step();

    // Attractors.
    attr.alloc(colony.attr.size());
    for (size_t i = 0; i < attr.len(); ++i) {
      auto& atr = attr[i];
      atr.pos(colony.attr[i].pos);
      atr.cola(colony.attr[i].alive ? glm::vec3(80.f, 0.f, 0.f) : glm::vec3(30.f, 30.f, 30.f));
      atr.size(mt::show_attr ? 0.004f : 0.f);
    }

    // Nodes.
    nodes.alloc(colony.nodes.size());
    for (size_t i = 0; i < nodes.len(); ++i) {
      auto& nd = nodes[i];
      nd.pos(colony.nodes[i].pos);
      float prn = hashv(nd.pos());
      auto a = mt::paramv("tree_bark_col_a");
      auto b = mt::paramv("tree_bark_col_b");
      nd.cola(cmix(a, b, prn, 1.f));
      nd.size(mt::show_nodes ? colony.nodes[i].size : 0.f);
      nd.opac(mt::paramf("tree_bark_opac"));
      nd.add(mt::paramf("tree_bark_add"));
    }

    // Leaves.
    leaves.alloc(colony.leaves.size());
    for (size_t i = 0; i < leaves.len(); ++i) {
      auto& lv = leaves[i];
      lv.pos(colony.leaves[i]);
      float prn = hashv(lv.pos());
      auto a = mt::paramv("tree_leaves_col_a");
      auto b = mt::paramv("tree_leaves_col_b");
      lv.cola(cmix(a, b, prn, 1.f));
      auto s = mt::paramf("tree_leaves_size");
      prn = hashv(lv.pos() + glm::vec3(0.2253f));
      lv.size(fmix(0.5f*s, 3.f*s, prn, 2.5f));
      lv.opac(mt::paramf("tree_leaves_opac"));
      lv.add(mt::paramf("tree_leaves_add"));
    }
  }

  // Back object constructor.
  back::back() {
    shader.load_file(glsl_dir + "pass.vert", glsl_dir + "back.frag");
    canvas.attach(GL_COLOR_ATTACHMENT0, GL_RGBA16F, (int)mt::window_size.x, (int)mt::window_size.y);
  }

  // Draw back cover.
  void back::draw(GLuint id) {
    shader.enable();
    shader.uniform("window", mt::window_size);
    shader.uniform("sky_col", mt::paramv("back_bg_col"));
    shader.uniform("sun_pos", mt::paramv("back_sun_pos"));
    shader.uniform("sun_col", mt::paramv("back_sun_col"));
    shader.uniform("cloud_col", mt::paramv("back_cloud_col"));
    shader.uniform("cloud_strength", mt::paramf("back_cloud_strength"));
    shader.uniform("sun_radius", mt::paramf("back_sun_radius"));
    shader.uniform("sun_strength", mt::paramf("back_sun_strength"));
    shader.uniform("waterline", mt::paramf("back_waterline"));
    shader.uniform("exposure", mt::paramf("back_exposure"));
    shader.texture("tex", id);
    gl::quad::draw(0, 0, mt::window_size.x, mt::window_size.y);
    shader.disable();
  }

  // Blur constructor.
  blur::blur() {
    shader.load_file(glsl_dir + "pass.vert", glsl_dir + "blur.frag");
    pp[0].attach(GL_COLOR_ATTACHMENT0, GL_RGBA16F, (int)mt::window_size.x, (int)mt::window_size.y);
    pp[1].attach(GL_COLOR_ATTACHMENT0, GL_RGBA16F, (int)mt::window_size.x, (int)mt::window_size.y);
  }

  // Capture frame into FBO.
  void blur::enable() {
    pp[0].enable();
  }

  // End capture.
  void blur::disable() {
    pp[0].disable();
  }

  // Run one blur step.
  void blur::process(float radius) {
    pp[1].enable();
    shader.enable();
    shader.uniform("window", mt::window_size);
    shader.texture("tex", pp[0].get_ID());
    shader.uniform("dir", glm::vec2(0.f, 1.f));
    shader.uniform("radius", radius);
    gl::quad::draw(0, 0, mt::window_size.x, mt::window_size.y);
    shader.disable();
    pp[1].disable();

    pp[0].enable();
    shader.enable();
    shader.uniform("window", mt::window_size);
    shader.texture("tex", pp[1].get_ID());
    shader.uniform("dir", glm::vec2(1.f, 0.f));
    shader.uniform("radius", radius);
    gl::quad::draw(0, 0, mt::window_size.x, mt::window_size.y);
    shader.disable();
    pp[0].disable();
  }

  // Front object constructor.
  front::front() {
    shader.load_file(glsl_dir + "pass.vert", glsl_dir + "front.frag");
  }

  // Draw front cover.
  void front::draw(GLuint id) {
    shader.enable();
    shader.uniform("window", mt::window_size);
    shader.uniform("paper_col", mt::paramv("front_paper_col"));
    shader.texture("tex", id);
    gl::quad::draw(0, 0, mt::window_size.x, mt::window_size.y);
    shader.disable();
  }

}
