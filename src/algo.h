#pragma once

#include <vector>
#include <limits>

#include <glm/glm.hpp>

namespace mt {

  // Node of a single branch.
  class node {
  public:
    node(glm::vec3 p = glm::vec3(0.f));

    node grow(float mag);

    std::vector<glm::vec3> attr;
    glm::vec3 pos;
    float size;
    int parent_idx;
    std::vector<int> child_idx;
    bool done;
  };

  // Points of attraction for the nodes.
  class attractor {
  public:
    attractor(glm::vec3 p = glm::vec3(0.f));

    void attract(std::vector<mt::node>& nds, size_t start, float min);

    glm::vec3 pos;
    float dist;
    bool alive;
    int node_idx;
  };

  struct bark {
    glm::vec3 pos;
    float size;
  };

  // Implementation of the space colonization algorithm.
  class colony {
  public:
    void init();
    void step();

    // Return i-th parent of node.
    int parent(size_t id, size_t i) {
      auto& nd0 = nodes[id];
      while (i--) {
        if (nd0.parent_idx == -1) break;
        nd0 = nodes[nd0.parent_idx];
      }
      return nd0.parent_idx;
    }

    std::vector<mt::attractor> attr;
    std::vector<mt::node> nodes;
    std::vector<glm::vec3> leaves;
    std::vector<mt::bark> bark;


    void create_envelope();
    void create_roots();

    glm::vec3 cursor;
    size_t attr_alive;
    float iter;
    size_t last;
    bool finished;
    float max_dist;

    glm::vec3 base = glm::vec3(0.f);
    glm::vec3 grow_dir = glm::normalize(glm::vec3(0.4f, 1.f, 0.f));

    float trunk_length = 0.4f;
    float trunk_width = 0.1f;
    float trunk_ppv = 5e3;
    float env_length = 0.8f;
    float env_width = 0.8f;
    float env_ppv = 8e4;
    float root_length = 0.4f;
    float root_width = 0.7f;
    float root_ppv = 8e4;
    float min_branch_size = 1e-5;
    float branch_growth_factor = 0.65f;

    float attr_rad = 0.07f;
    float kill_rad = 0.05f;
    float unit = 0.005f;

    float max_branch_leaves = 0.01f;
    float leaves_top_ppv = 30.f/unit;
    float bark_ppv = 1e4;
    float bark_ppl = 1.f;
  };

}
