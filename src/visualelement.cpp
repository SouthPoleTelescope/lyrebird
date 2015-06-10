#include "visualelement.h"
#include <iostream>
#include <assert.h>

#include "genericutils.h"

using namespace std;
using namespace glm;

VisElem::VisElem(SimpleRen * simple_ren, EquationMap * eqs,
		 vis_elem_repr v
		 ){

  //DCOUT("Loading VisElem", DEBUG_0);
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
  sr_index = s_ren->add_ren_state(rs);  

  if (v.labels.size() == 0){
    print_and_exit("Vis elem with no label");
  }


  //set all our labels
  //v.labels;
  labels = vector<string>(v.labels.size());
  for (int i=0; i < v.labels.size(); i++)labels[i] = v.labels[i];

  group = v.group;
  l_data_labels = vector<string>(v.labelled_data.size());
  l_data_vals = vector<string>(v.labelled_data.size());

  for (int i=0; i < v.labelled_data.size(); i++){
    l_data_labels[i] = v.labelled_data[i];
    l_data_vals[i] = v.labelled_data_vs[i];
  }

  layer = v.layer;
  int highlight_geo_index = s_ren->get_geo_index(v.highlight_geo_id);

  hXScale = rs.x_scale;  
  hYScale = rs.y_scale;
  hTDelt = 0;

  rs.layer = 10;
  rs.geo_index = highlight_geo_index;
  highlight_index = s_ren->add_ren_state(rs);
  geo_id = v.geo_id;


  equation_map_ = eqs;
  equation_inds_ = std::vector<int>(v.equations.size());

  for (int i=0; i < v.equations.size(); i++){
    equation_inds_[i] = equation_map_->get_eq_index(v.equations[i]);
  }
  has_eq = true;
  eq_ind = 0;

  set_drawn();
  update_color();
}

void VisElem::set_eq_ind(int ind){
  eq_ind = ind;
}

int VisElem::get_layer(){
  return layer;
}

void VisElem::set_drawn(){
  s_ren->set_drawn(sr_index);
}

void VisElem::set_not_drawn(){
  s_ren->set_not_drawn(sr_index);
}


void VisElem::set_highlighted(glm::vec3 color){
  s_ren->set_drawn(highlight_index);
  s_ren->set_color( highlight_index, vec4(color, 1) );
}

void VisElem::set_not_highlighted(){
  s_ren->set_not_drawn(highlight_index);
}

void VisElem::animate_highlight(float tstep){
  hTDelt += tstep;
  s_ren->set_scale(highlight_index,  
		 (1 + .05*cos(15*hTDelt)) * hXScale,
		 (1 + .05*cos(15*hTDelt)) * hYScale);
  
}

glm::mat4 VisElem::get_ms_transform(){
  return s_ren->get_ms_transform(sr_index);
}

std::string VisElem::get_geo_id(){
  return geo_id;
}

Equation & VisElem::get_current_equation(){
  assert(equation_inds_.size() > 0);
  return equation_map_->get_eq(equation_inds_[eq_ind]);
}

void VisElem::update_color(){
  assert(has_eq);
  glm::vec4 col = get_current_equation().get_color();
  //cout<<"setting color"<<endl;
  s_ren->set_color(sr_index, col );
}


void VisElem::update_all_equations(){
  for (int i=0; i<equation_inds_.size();i++){
    equation_map_->get_eq(equation_inds_[i]).get_value();
  } 
}

bool VisElem::string_matches_labels(const char * pattern){
  for (size_t i=0; i < labels.size(); i++){
    if (is_glob_match(pattern, labels[i])) return true;
  }
  return false;
}

void VisElem::get_all_info(std::vector<string> & ai_labels, std::vector<string> & ai_tags,
			   std::vector<string> & ai_tag_vals,
			   std::vector<string> & ai_eq_labels,  std::vector<float*> & ai_eq_addrs
			  ){
  assert(labels.size() > 0);
  ai_labels = std::vector<string>(labels);
  assert(ai_labels.size() > 0);
  ai_tags = std::vector<string>(l_data_labels);
  ai_tag_vals = std::vector<string>(l_data_vals);
  ai_eq_labels = std::vector<string>(equation_inds_.size(), "");
  ai_eq_addrs = std::vector<float*>(equation_inds_.size(), NULL);
  for (size_t i=0; i < equation_inds_.size(); i++){
    Equation & cur_eq = equation_map_->get_eq(equation_inds_[i]);
    ai_eq_labels[i] = cur_eq.get_label();
    ai_eq_addrs[i] = cur_eq.get_value_address();
  }
}





