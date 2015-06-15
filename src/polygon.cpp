#include "polygon.h"
#include "geometryutils.h"
#include "genericutils.h"
using namespace glm;
using namespace std;

Polygon::Polygon(std::vector<glm::vec2> points){
  vector<vec2> tri_points;
  triangulate_polygon(points,tri_points);
  for (size_t i=0; i < tri_points.size(); i+=3){
    tris.push_back( Triangle(tri_points[i],tri_points[i+1],tri_points[i+2]));
  }
  set_AABB();
}

void Polygon::set_AABB(){
  if (tris.size() <1) print_and_exit("we have an empty triangle");
  vec2 min;
  vec2 max;
  tris[0].get_AABB(min_AABB, max_AABB);

  for (size_t i=1; i < tris.size(); i++){
    tris[i].get_AABB(min, max);
    if (min.x < min_AABB.x) min_AABB.x = min.x;
    if (min.y < min_AABB.y) min_AABB.y = min.y;
    if (max.x > max_AABB.x) max_AABB.x = max.x;
    if (max.y > max_AABB.y) max_AABB.y = max.y;
  }
}

void Polygon::get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb){
  min_aabb = min_AABB;
  max_aabb = max_AABB;
}

void Polygon::apply_transform(glm::mat4 trans){
  for (size_t i = 0; i < tris.size(); i++) tris[i].apply_transform(trans);
  set_AABB();
}

bool Polygon::is_inside(vec2 p){
  //DCOUT( "Checking inside min: x" << min_AABB.x << " y"<<min_AABB.y, DEBUG_1);
  //DCOUT( "Checking inside max: x" << max_AABB.x << " y"<<max_AABB.y, DEBUG_1);
  if  (!((p.x >= min_AABB.x) && (p.x <= max_AABB.x) &&
         (p.y >= min_AABB.y) && (p.y <= max_AABB.y)) ) return false;
  for (size_t i=0; i < tris.size(); i++){
    if (tris[i].is_inside(p)) return true;
  }
  return false;
}



Triangle::Triangle(vec2 p0,
		   vec2 p1,
		   vec2 p2){

  ps[0] = (p0);
  ps[1] = (p1);
  ps[2] = (p2);
  /**
  cout<<"Instantiating triangle"<<endl;
  for (int i=0; i < 3; i++)
    cout<<"Tri point "<<i<<": x:"<<ps[i].x<<" y:"<<ps[i].y<<endl;
  **/
  set_AABB();
}

void Triangle::get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb){
  min_aabb = min_AABB;
  max_aabb = max_AABB;
}



void Triangle::set_AABB(){
  min_AABB = ps[0];
  max_AABB = ps[0];

  if (ps[1].x < min_AABB.x) min_AABB.x = ps[1].x;
  if (ps[2].x < min_AABB.x) min_AABB.x = ps[2].x;
  if (ps[1].y < min_AABB.y) min_AABB.y = ps[1].y;
  if (ps[2].y < min_AABB.y) min_AABB.y = ps[2].y;

  if (ps[1].x > max_AABB.x) max_AABB.x = ps[1].x;
  if (ps[2].x > max_AABB.x) max_AABB.x = ps[2].x;
  if (ps[1].y > max_AABB.y) max_AABB.y = ps[1].y;
  if (ps[2].y > max_AABB.y) max_AABB.y = ps[2].y;
}

void Triangle::apply_transform(mat4 trans){
  for (int i=0; i < 3; i++)
    ps[i] = transform_vec2(ps[i], trans);
  set_AABB();
}

bool Triangle::is_inside(vec2 p){
  if  (!((p.x >= min_AABB.x) && (p.x <= max_AABB.x) &&
	 (p.y >= min_AABB.y) && (p.y <= max_AABB.y)) ) return false;
  //return true;
  vec2 v0 = ps[2] - ps[0];
  vec2 v1 = ps[1] - ps[0];
  vec2 v2 = p - ps[0];
  float dot00 = dot(v0, v0);
  float dot01 = dot(v0, v1);
  float dot02 = dot(v0, v2);
  float dot11 = dot(v1, v1);
  float dot12 = dot(v1, v2);
  float inv_denom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  float u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
  float v = (dot00 * dot12 - dot01 * dot02) * inv_denom;
  return (u >= 0) && (v >= 0) && (u + v < 1);
}


