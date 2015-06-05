#pragma once
#include <string>
#include <vector>
#include <map>
#include <list>

#include <glm/glm.hpp>
#include <AntTweakBar.h>

#include "polygon.h"
#include "visualelement.h"



/**
   All of the shapes are taken to be in model space not in View Space.  
 **/

class Highlighter{
 public:
  Highlighter(TwBar * info_bar, std::vector<VisElem> * vis_elems);
  
  //code for handling shape geometry
  int get_clicked_elem(glm::vec2 click_point);
  void add_shape_definition(std::string id, std::string svg_path);
  void add_defined_shape(std::string id, glm::mat4 transform, int elem_id, int layer);

  void set_AABB();
  void get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb);

  
  //code for parsing inputs
  void parse_click(glm::vec2 pos, int mod_key);
  void run_search();
  void clear_hls();
  void add_hl(int index);
  void update_info_bar();

  std::list<int> get_plot_inds();
  std::list<glm::vec3> get_plot_colors();
  glm::vec3 get_hl_color(int ind);

  void fill_info_bar();

 private:
  //used for shape geometry
  glm::vec2 min_AABB_;
  glm::vec2 max_AABB_;
  std::vector<Polygon> geo_polys;
  std::vector<int> geo_ids;
  std::vector<int> geo_layer;

  std::map< std::string, std::vector< Polygon > >  shape_polys;
  
  std::list<int> hl_inds;
  std::list<glm::vec3> hl_colors;
  std::vector<VisElem> * vis_elems_;
  
  TwBar * info_bar_;
  int info_bar_index;
  
  //from searcher
  std::vector<int> find_elems(std::string s);
  std::vector<std::string> search_strs;
  std::vector<int> search_ids;

};

