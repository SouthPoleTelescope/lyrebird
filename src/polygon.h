#pragma once
#include <vector>

#include "glm/glm.hpp"


class Triangle{
 public:
  Triangle(glm::vec2 p0,
	   glm::vec2 p1,
	   glm::vec2 p2
	   );
  void get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb);
  void apply_transform(glm::mat4 trans);
  bool is_inside(glm::vec2 p);
 private:

  void set_AABB();
  glm::vec2 ps[3];
  
  glm::vec2 min_AABB;
  glm::vec2 max_AABB;
};


class Polygon{
 public:
  Polygon(std::vector<glm::vec2> points);
  void get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb);
  void apply_transform(glm::mat4 trans);
  bool is_inside(glm::vec2 p);
 private:
  std::vector<Triangle> tris; 
  glm::vec2 min_AABB;
  glm::vec2 max_AABB;
  void set_AABB();

};


