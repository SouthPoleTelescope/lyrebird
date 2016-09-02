#include "highlighter.h"

#include "glm/gtx/color_space.hpp"
#include <list>
#include "geometryutils.h"
#include "genericutils.h"
#include <unistd.h>
#include <assert.h>

using namespace std;
using namespace glm;

uint32_t get_ant_tweak_bar_color(glm::vec3 col){
  uint32_t r = ((uint8_t)(255 * col.r));
  uint32_t g = ((uint8_t)(255 * col.g));
  uint32_t b = ((uint8_t)(255 * col.b));
  uint32_t a = 0;
  return r << 0 | g << 8| b << 16 | a << 24;
}


int Highlighter::get_clicked_elem(glm::vec2 click_point){
  int is_set = 0;
  int elem_id = -1;
  int layer = -1;
  for (size_t i=0; i < geo_polys_.size(); i++){
    if (geo_polys_[i].is_inside(click_point) && ((*vis_elems_)[geo_ids_[i]]->is_drawn())){
      if (!is_set){
	elem_id = geo_ids_[i];
	layer = geo_layer_[i];
	is_set = 1;
      }else{
	if (geo_layer_[i] > layer){
	  layer = geo_layer_[i];
	  elem_id = geo_ids_[i];
	}
      }
    }
  }
  return  elem_id;
}


void Highlighter::add_shape_definition(std::string id, std::string svg_path){
  /**
     void con_svg_to_polys(string fn, float tol,
		      std::vector<std::vector<glm::vec2> > & polygons,
		      std::vector< glm::vec4 > & polygon_colors
		      );
   **/
  std::vector<std::vector<glm::vec2> > polygons;
  std::vector< glm::vec4 > polygon_colors;
  con_svg_to_polys(svg_path, .1, polygons, polygon_colors);

  vector<Polygon> pvec;
  for (size_t i=0; i < polygons.size();i++){
    pvec.push_back(Polygon(polygons[i]));
  }
  shape_polys_[id] = pvec;
}

void Highlighter::add_defined_shape(std::string id, glm::mat4 transform, int elem_id, int layer){
  if ( shape_polys_.find(id) == shape_polys_.end()){
    print_and_exit("trying to add unknown shape");
  }
  vector<Polygon> s_polys = shape_polys_[id];

  //cout<<"Found  s_polys.size() "<<s_polys.size()<<endl;
  for (size_t i=0; i < s_polys.size(); i++){
    geo_polys_.push_back(s_polys[i]);
    geo_ids_.push_back(elem_id);
    geo_polys_[geo_polys_.size()-1].apply_transform(transform);
    geo_layer_.push_back(layer);
  }
}
void Highlighter::set_AABB(){
  if (geo_polys_.size() <1) print_and_exit("we have empty clickgeo");
  vec2 min;
  vec2 max;
  geo_polys_[0].get_AABB(min_AABB_, max_AABB_);
  for (size_t i=1; i < geo_polys_.size(); i++){
    geo_polys_[i].get_AABB(min, max);
    if (min.x < min_AABB_.x) min_AABB_.x = min.x;
    if (min.y < min_AABB_.y) min_AABB_.y = min.y;
    if (max.x > max_AABB_.x) max_AABB_.x = max.x;
    if (max.y > max_AABB_.y) max_AABB_.y = max.y;
  }
}
void Highlighter::get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb){
  set_AABB();
  min_aabb = min_AABB_;
  max_aabb = max_AABB_;

}



Highlighter::Highlighter(TwBar * info_bar, std::vector<VisElemPtr> * vis_elems, size_t num_info_bar_elems)
  : num_info_bar_elems_(num_info_bar_elems)
{
  info_bar_ = info_bar;
  vis_elems_ = vis_elems;
  info_bar_index=-1;

  if (bind_udp_socket(listen_socket_, "127.0.0.1", 5555)){
	  listen_socket_ = -1;
  }
  send_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
  if ( fill_addr("127.0.0.1", 5556, send_addr_) ){
	  send_socket_ = -1;
  }
}

void Highlighter::check_socket(){
	if (listen_socket_ < 0) return;
	std::vector<std::string> strs;
	if (get_string_list(listen_socket_, strs)) return;
	for (size_t i=0; i < strs.size(); i++) {
		if (strs[i].size() > 0) {
			run_search(strs[i].c_str(), true);
		}
	}
}

//parsing inputs
//given in modelspace coordinates
void Highlighter::parse_click(glm::vec2 pos, int mod_key){
  int clicked_elem = get_clicked_elem(pos);
  if (!mod_key){
    clear_hls();
  }
  if (clicked_elem >= 0){
    add_hl(clicked_elem);
  }
  fill_info_bar();
}

void Highlighter::run_search(const char * search_str, bool no_send){
  clear_hls();
  for (size_t i=0; i < vis_elems_->size(); i++){
    if ((*vis_elems_)[i]->string_matches_labels(search_str) && (*vis_elems_)[i]->is_drawn()){
	    add_hl(i, no_send);
    }
  }
  fill_info_bar();
}

void Highlighter::fill_info_bar(){
  
  if (hl_inds_.size() < num_info_bar_elems_ && hl_inds_.size() > 0){
    TwRemoveAllVars(info_bar_);
    TwRefreshBar(info_bar_);
    info_bar_is_visible_ = 1;
    TwSetParam(info_bar_, NULL, "visible", TW_PARAM_INT32, 1, &info_bar_is_visible_);
    info_bar_index = hl_inds_.front();

    atb_colors_ = std::vector<uint32_t>( hl_colors_.size());
    int i=0;
    auto hl_colors_it = hl_colors_.begin();
    for (auto hl_ind=hl_inds_.begin(); hl_ind != hl_inds_.end(); hl_ind++, hl_colors_it++, i++){
      int el = *hl_ind;
      (*vis_elems_)[el]->update_all_equations();
      std::vector<string> ai_labels;
      std::vector<string> ai_tags;
      std::vector<string*> ai_tag_vals;
      std::vector<string> ai_eq_labels;
      std::vector<float*> ai_eq_addrs;
      (*vis_elems_)[el]->get_all_info(ai_labels, ai_tags, ai_tag_vals, ai_eq_labels, ai_eq_addrs);

      std::string group_label = std::string(" group=") + ai_labels[0] + std::string( " ");

      atb_colors_[i] = get_ant_tweak_bar_color(*hl_colors_it);
      std::string color_label = ai_labels[0] + std::string("_color");
      std::string color_opts = group_label + std::string(" label=color ");
      TwAddVarRO(info_bar_, color_label.c_str(), TW_TYPE_COLOR32, &(atb_colors_[i]), color_opts.c_str());


      for (size_t i=0; i < ai_tags.size(); i++){
	std::string tag_name = ai_labels[0] + ai_tags[i];
	std::string atb_opts = group_label + std::string(" label='") + ai_tags[i] + std::string("' ");
	TwAddVarRO(info_bar_, 
		   tag_name.c_str(), TW_TYPE_STDSTRING,
		   ai_tag_vals[i], atb_opts.c_str()
		   );
      }


      for (size_t i=0; i < ai_eq_labels.size(); i++){
	std::string eq_name = ai_labels[0] + ai_eq_labels[i];
	std::string atb_opts = group_label + std::string(" label='") + ai_eq_labels[i] + std::string("' ");
	TwAddVarRO(info_bar_, eq_name.c_str(), TW_TYPE_FLOAT,
		   ai_eq_addrs[i],atb_opts.c_str());
      }
    }
  }
}

void Highlighter::update_info_bar(){
  if (info_bar_index >= 0){
    TwRefreshBar(info_bar_);
    if (hl_inds_.size() < num_info_bar_elems_ && hl_inds_.size() > 0){
      for (auto j=hl_inds_.begin(); j != hl_inds_.end(); j++){
	(*vis_elems_)[*j]->update_all_equations();
      }
    }
  }
}

void Highlighter::clear_hls(){
  info_bar_index = -1;
  TwRemoveAllVars(info_bar_);
  int visible = 0;
  TwSetParam(info_bar_, NULL, "visible", TW_PARAM_INT32, 1, &visible);

  while (!(hl_inds_.empty())){
    int el = hl_inds_.front();
    (*vis_elems_)[el]->set_not_highlighted();
    hl_inds_.pop_front();
    hl_colors_.pop_front();
  }
}

void Highlighter::add_hl(int index, bool no_send){
  for (auto iter = hl_inds_.begin(); iter != hl_inds_.end(); ++iter) {
    if (*iter == index) return;
  }
  glm::vec3 hcol =  get_hl_color(hl_inds_.size());
  (*vis_elems_)[index]->set_highlighted(hcol);
  hl_inds_.push_back(index);
  hl_colors_.push_back(hcol);

  if (! no_send || send_socket_ < 0) {
	  if ((*vis_elems_)[index]->labels.size()== 0) return;
	  std::string s( (*vis_elems_)[index]->labels[0] );
	  sendto(send_socket_, 
		 s.c_str(), s.size()+1,
		 0, (struct sockaddr *)&send_addr_,
		 sizeof(send_addr_));
  }
}

std::list<int> Highlighter::get_plot_inds(){
  return hl_inds_;
}
std::list<glm::vec3> Highlighter::get_plot_colors(){
  return hl_colors_;
}

//random utils
glm::vec3 Highlighter::get_hl_color(int ind){
  glm::vec3 hsv_col;  // 0-360, 0-1, 0-1 are the units  
  hsv_col[0] = ((ind+1) * 67)%360 ;
  hsv_col[1] = 1;
  hsv_col[2] = 1;  
  return rgbColor(hsv_col);
}


