#include "algo.h"

#include <limits>

#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/spline.hpp>

#include "config.h"

namespace mt {

  // Area of outside of cylinder.
  static float cyl_area(float r, float h) {
    return mt::tau*r*h;
  }

  // Volume of cylinder.
  static float cyl_volume(float r, float h) {
    return mt::pi*r*r*h;
  }

  // Return random vector on cylinder surface.
  static glm::vec3 cyl_rand(glm::vec3 dir, float r) {
    glm::vec2 circ = glm::circularRand(r);
    float y = glm::linearRand(0.f, glm::length(dir));

    glm::vec3 res = glm::vec3(circ.x, y, circ.y);

    glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
    // Rotate to direction.
    dir = glm::normalize(dir);
    if (dir == up || dir == glm::vec3(0.f))
      return res;
    else  {
      glm::vec3 axis = glm::cross(dir, up);
      float angle = glm::angle(dir, up);
      return glm::rotate(res, -angle, axis);
    }
  }

  // Area of bulge. Used for normalizing density.
  static float bulge_area(float x, float y) {
    y = glm::clamp(y, 0.f, x);
    if (y == 0.f) return mt::pi*x*x;
    float r = y/2.f + x*x/(2.f*y);
    return mt::tau*r*y;
  }

  // Return random vector inside circle with curvature out of plane.
  static glm::vec3 bulge_rand(glm::vec3 dir, float x, float y) {
    glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 res;
    y = glm::clamp(y, 0.f, x);

    if (y == 0.f) {
      auto r = glm::diskRand(x);
      res = glm::vec3(r.x, 0.f, r.y);
    } else {
      // Calculate y-point of origin of sphere.
      float yc = y/2.f - x*x/(2.f*y);
      float r = y - yc;
      float a = y/r/2.f;

      float polar = glm::linearRand(0.f, (float)mt::tau);
      float azi = glm::acos(1.f - 2.f*glm::linearRand(0.f, a));
      res = r * glm::vec3(sin(azi)*cos(polar), cos(azi), sin(azi)*sin(polar));
      res.y += yc;
    }

    // Rotate to direction.
    dir = glm::normalize(dir);
    if (dir == up || dir == glm::vec3(0.f))
      return res;
    else  {
      glm::vec3 axis = glm::cross(dir, up);
      float angle = glm::angle(dir, up);
      return glm::rotate(res, -angle, axis);
    }
  }

  // Node class constructor.
  node::node(glm::vec3 p): 
    pos(p),
    size(0.f),
    parent_idx(-1),
    done(false) {
  }

  // Spawn new node in average direction of attractors.
  node node::grow(float mag) {
    glm::vec3 avg = glm::vec3(0.f);
    for (size_t i = 0; i < attr.size(); ++i)
      avg += attr[i];
    avg /= (float)attr.size();

    attr.clear();
    glm::vec3 dir = glm::normalize(avg - pos) * mag;
    auto n = node(pos + dir);
    return n;
  }

  // Attractor class constructor.
  attractor::attractor(glm::vec3 p):
    pos(p),
    dist(std::numeric_limits<float>::max()),
    alive(true),
    node_idx(-1) {
  }

  // Find closest node and add own position as influence.
  void attractor::attract(std::vector<mt::node>& nds, size_t start, float min) {
    for (size_t i = start; i < nds.size(); ++i) {
      float mag = glm::distance(nds[i].pos, pos);
      if (mag <= dist) {
        dist = mag;
        node_idx = i;
      }
    }
    if (dist < min && node_idx >= 0)
      nds[node_idx].attr.push_back(pos);
  }

  // Initialize colony; node and attractors.
  void colony::init() {
    last = 0;
    iter = 0.f;
    finished = false;
    cursor = base;
    max_dist = 0;

    attr.clear();
    nodes.clear();
    leaves.clear();
    bark.clear();
    attr.push_back(cursor);
    nodes.push_back(cursor);

    create_envelope();
    attr_alive = attr.size();
  }

  static glm::vec3 next_env(glm::vec3 dir, float d, float y, float ppv, float* du) {
    auto vec = bulge_rand(dir, d*0.5f, y);
    vec -= dir*y;
    float area = bulge_area(d*0.5f, y);
    *du = 1.f / (area*ppv);
    return vec;
  }

  // Create attraction points for tree envelope.
  void colony::create_envelope() {
    float du;
    while (iter < env_length) {
      float i = iter/env_length;
      float w = glm::mix(trunk_width, env_width, i*i);
      float y = glm::mix(0.f, env_width*0.5f, i);
      float ppv = glm::mix(trunk_ppv, env_ppv, i*i*i*i*i);

      auto res = next_env(grow_dir, w, y, ppv, &du);
      attr.push_back(cursor + res);

      iter += du;
      cursor += grow_dir*du;
    }

  }

  // Create attraction points for roots.
  void colony::create_roots() {
    cursor = base;
    iter = 0.f;

    float du;
    while (iter < root_length) {
      float i = iter/root_length;
      float w = glm::mix(trunk_width, root_width, i);
      float y = glm::mix(0.f, root_width*0.5f, i);
      float ppv = glm::mix(root_ppv, root_ppv, i);

      auto res = next_env(-grow_dir, w, y, ppv, &du);
      attr.push_back(cursor + res);

      iter += du;
      cursor += -grow_dir*du;
    }

    // First node of roots.
    auto rp = base - grow_dir*unit;
    auto nn = node(rp);
    nn.parent_idx = 0;
    nodes[0].child_idx.push_back(nodes.size());
    nodes.push_back(nn);
  }

  static glm::vec3 donut_rand(glm::vec3 dir, float rad, float sd) {
    auto xy = glm::circularRand(glm::linearRand(0.f, rad));
    /* auto xy = glm::circularRand(glm::gaussRand(rad, sd)); */
    auto res = glm::vec3(xy.x, 0.f, xy.y);
    auto up = glm::vec3(0.f, 1.f, 0.f);

    // Rotate to direction.
    dir = glm::normalize(dir);
    if (dir == up || dir == glm::vec3(0.f))
      return res;
    else  {
      glm::vec3 axis = glm::cross(dir, up);
      float angle = glm::angle(dir, up);
      return glm::rotate(res, -angle, axis);
    }
  }

  static bool chance(float prob) {
    float r = std::rand() / (float)RAND_MAX;
    return (r <= glm::clamp(prob, 0.f, 1.f));
  }

  // Perform one iteration of the space colonization algorithm.
  void colony::step() {
    if (attr.size() == 0) return;
    for (size_t i = 0; i < attr.size(); ++i)
      if (attr[i].alive)
        attr[i].attract(nodes, last, attr_rad);

    float step = unit-0.002f*std::pow(1.f - attr_alive/(float)attr.size(), 0.5f);
    // If nodes are still growing.
    if (nodes.size() != last) {
      last = nodes.size();
      for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].attr.size() > 0) {
          auto n = nodes[i].grow(step);
          n.parent_idx = i;
          float d = glm::distance(n.pos, nodes[0].pos);
          max_dist = std::max(d, max_dist);
          nodes[i].child_idx.push_back(nodes.size());
          nodes.push_back(n);
        }
      }

      // Each iteration the trunk sizes are computed from scratch.
      for (size_t i = 0; i < nodes.size(); ++i)
        nodes[i].size = 0.f;

      // Propagate backwards from child to parent to calculate trunk widths.
      for (int i = nodes.size()-1; i > 0; --i) {
        if (nodes[i].size == 0.f) nodes[i].size = min_branch_size;

        int p = nodes[i].parent_idx;
        nodes[p].size += nodes[i].size;
      }
      nodes[0].size = 0.f; // Genesis node is never shown.

      // Square root all node sizes.
      for (size_t i = 0; i < nodes.size(); ++i)
        nodes[i].size = std::pow(nodes[i].size, branch_growth_factor);

      attr_alive = attr.size();
      for (size_t i = 0; i < attr.size(); ++i)
        if (attr[i].dist < kill_rad) {
          attr[i].alive = false;
          attr_alive--;
        }

      // Sloppy...
      if (nodes.size() == 20) create_roots();

    } else if (!finished) {
      for (int i = nodes.size()-1; i > 0; --i) {
        // Add leaves.
        if (nodes[i].size < max_branch_leaves && nodes[i].pos.y >= unit*30.f) {
          int p = nodes[i].parent_idx;
          auto fin = nodes[p].pos;
          auto start = nodes[i].pos;
          auto dir = fin - start;

          int segs = 1;
          for (int k = 0; k < segs; ++k) {
            auto an = start + dir*(k/(float)segs);
            float d = glm::distance(an, nodes[0].pos)/max_dist;

            if (chance(0.1f*glm::smoothstep(0.01f, 0.1f, d))) {
              auto pt = donut_rand(dir, std::sqrt(nodes[i].size)/5.f, unit);
              leaves.push_back(pt + an);
            }
          }
        }
        finished = true;
      }
    }

  }

  // Return nearest point. Pass pointer to also return distance.
  static glm::vec3 nearest(const std::vector<glm::vec3>& arr, const glm::vec3& p, float* mag) {
    if (arr.size() == 0) return p;
    glm::vec3 res;
    float dist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < arr.size(); ++i) {
      float d = glm::distance(arr[i], p);
      if (d < dist) {
        res = arr[i];
        dist = d;
      }
    }
    *mag = dist;
    return res;
  }

}
