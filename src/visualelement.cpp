#include "visualelement.h"
#include <algorithm>
#include <string.h>

#include "genericutils.h"
#include "logging.h"

using namespace std;
using namespace glm;

VisElem::VisElem(SimpleRen * simple_ren, EquationMap * eqs,
		 vis_elem_repr v
		 ){

  s_ren = simple_ren;

  int geo_index = s_ren->get_geo_index(v.geo_id);
  render_state rs = get_empty_ren_state();

  rs.x_center = v.x_center;
  rs.y_center = v.y_center;
  rs.x_scale = v.x_scale;
  rs.y_scale = v.y_scale;
  rs.rotation = v.rotation;
  rs.layer = v.layer;
  rs.geo_index = geo_index;  
  simple_ren_index_ = s_ren->add_ren_state(rs);  

  if (v.labels.size() == 0){
    log_fatal("Vis elem with no label");
  }


  //set all our labels
  //v.labels;
  labels = vector<string>(v.labels.size());
  for (size_t i=0; i < v.labels.size(); i++)labels[i] = v.labels[i];

  group_ = v.group;
  l_data_labels = vector<string>(v.labelled_data.size());
  l_data_vals = vector<string>(v.labelled_data.size());

  for (size_t i=0; i < v.labelled_data.size(); i++){
    l_data_labels[i] = v.labelled_data[i];
    l_data_vals[i] = v.labelled_data_vs[i];
  }

  layer_ = v.layer;
  int highlight_geo_index = s_ren->get_geo_index(v.highlight_geo_id);

  hXScale = rs.x_scale;  
  hYScale = rs.y_scale;
  hTDelt = 0;

  rs.layer = -10;
  rs.geo_index = highlight_geo_index;
  highlight_index_ = s_ren->add_ren_state(rs);
  geo_id_ = v.geo_id;


  equation_map_ = eqs;
  equation_inds_ = std::vector<int>(v.equations.size());

  for (size_t i=0; i < v.equations.size(); i++){
    equation_inds_[i] = equation_map_->get_eq_index(v.equations[i]);
  }
  has_eq_ = true;
  eq_ind_ = 0;

  set_drawn();
  update_color(0);
  is_highlighted_ = false;
}

void VisElem::set_eq_ind(unsigned int ind){
	if (ind < equation_inds_.size())
		eq_ind_ = ind;
	else
		eq_ind_ = 0;
}

int VisElem::get_num_eqs(){
  return equation_inds_.size();
}

int VisElem::get_layer(){
  return layer_;
}

void VisElem::set_drawn(){
  s_ren->set_drawn(simple_ren_index_);
}

void VisElem::set_not_drawn(){
  s_ren->set_not_drawn(simple_ren_index_);
}

bool VisElem::is_drawn(){
  return s_ren->is_drawn(simple_ren_index_);
}


void VisElem::set_highlighted(glm::vec3 color){
  is_highlighted_ = true;
  s_ren->set_drawn(highlight_index_);
  s_ren->set_color( highlight_index_, vec4(color, 1) );
}

void VisElem::set_not_highlighted(){
  is_highlighted_ = false;
  s_ren->set_not_drawn(highlight_index_);

}

void VisElem::animate_highlight(float tstep){
  hTDelt += tstep;
  /**
  s_ren->set_scale(highlight_index_,  
		 (1 + .05*cos(15*hTDelt)) * hXScale,
		 (1 + .05*cos(15*hTDelt)) * hYScale);
  **/
  if (is_highlighted_){
    if (fmod(hTDelt, 1) > .25 )
      s_ren->set_drawn(highlight_index_);      
    else
      s_ren->set_not_drawn(highlight_index_);

  }
}

glm::mat4 VisElem::get_ms_transform(){
  return s_ren->get_ms_transform(simple_ren_index_);
}

std::string VisElem::get_geo_id(){
  return geo_id_;
}

Equation & VisElem::get_current_equation(){
  l3_assert(equation_inds_.size() > 0);
  return equation_map_->get_eq(equation_inds_[eq_ind_]);
}

void VisElem::update_color(size_t index){
  l3_assert(has_eq_);
  glm::vec4 col = get_current_equation().get_color(index);
  s_ren->set_color(simple_ren_index_, col );
}


void VisElem::update_all_equations(){
  for (size_t i=0; i<equation_inds_.size();i++){
    equation_map_->get_eq(equation_inds_[i]).get_value();
  } 
}

bool VisElem::string_matches_labels(const char * pattern){
  for (size_t i=0; i < labels.size(); i++){
    if (is_glob_match(pattern, labels[i])) return true;
  }
  return false;
}



bool VisElem::string_matches_labels_quick(const char * pattern){
	for (size_t i=0; i < labels.size(); i++){
		if (! strcmp(pattern, labels[i].c_str())) return true;
	}
	return false;
}

void VisElem::get_all_info(std::vector<string> & ai_labels, std::vector<string> & ai_tags,
			   std::vector<string*> & ai_tag_vals,
			   std::vector<string> & ai_eq_labels,  std::vector<float*> & ai_eq_addrs
			  ){
  l3_assert(labels.size() > 0);
  ai_labels = std::vector<string>(labels);
  l3_assert(ai_labels.size() > 0);
  ai_tags = std::vector<string>(l_data_labels);
  ai_tag_vals = std::vector<string*>(l_data_vals.size(), NULL);
  for (size_t i=0; i < l_data_vals.size(); i++){
    ai_tag_vals[i] = &(l_data_vals[i]);
  }
  
  //ai_eq_labels = std::vector<string>(equation_inds_.size(), "");
  //ai_eq_addrs = std::vector<float*>(equation_inds_.size(), NULL);
  ai_eq_labels.clear();
  ai_eq_addrs.clear();
  for (size_t i=0; i < equation_inds_.size(); i++){
    Equation & cur_eq = equation_map_->get_eq(equation_inds_[i]);
    if (cur_eq.display_in_info_bar()) {
	    //ai_eq_labels[i] = cur_eq.get_display_label();
	    //ai_eq_addrs[i] = cur_eq.get_value_address();
	    ai_eq_labels.push_back( cur_eq.get_display_label() );
	    ai_eq_addrs.push_back(cur_eq.get_value_address());
    }
  }
}


std::string VisElem::get_group(){
  return group_;
}

