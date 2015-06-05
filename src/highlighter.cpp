#include "highlighter.h"

#include <glm/gtx/color_space.hpp>

#include "geometryutils.h"
#include "genericutils.h"


using namespace std;
using namespace glm;



int Highlighter::get_clicked_elem(glm::vec2 click_point){
  int is_set = 0;
  int elem_id = -1;
  int layer = -1;
  for (int i=0; i < geo_polys.size(); i++){
    if (geo_polys[i].is_inside(click_point)){
      if (!is_set){
	elem_id = geo_ids[i];
	layer = geo_layer[i];
	is_set = 1;
      }else{
	if (geo_layer[i] > layer){
	  layer = geo_layer[i];
	  elem_id = geo_ids[i];
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
  for (int i=0; i < polygons.size();i++){
    pvec.push_back(Polygon(polygons[i]));
  }
  shape_polys[id] = pvec;
}

void Highlighter::add_defined_shape(std::string id, glm::mat4 transform, int elem_id, int layer){
  if ( shape_polys.find(id) == shape_polys.end()){
    print_and_exit("trying to add unknown shape");
  }
  vector<Polygon> s_polys = shape_polys[id];

  //cout<<"Found  s_polys.size() "<<s_polys.size()<<endl;
  for (int i=0; i < s_polys.size(); i++){
    geo_polys.push_back(s_polys[i]);
    geo_ids.push_back(elem_id);
    geo_polys[geo_polys.size()-1].apply_transform(transform);
    geo_layer.push_back(layer);
  }
}
void Highlighter::set_AABB(){
  if (geo_polys.size() <1) print_and_exit("we have empty clickgeo");
  vec2 min;
  vec2 max;
  geo_polys[0].get_AABB(min_AABB_, max_AABB_);
  for (int i=1; i < geo_polys.size(); i++){
    geo_polys[i].get_AABB(min, max);
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


vector<int> Highlighter::find_elems(string s){
  vector<int> vals;
  for (int i=0; i < search_strs.size(); i++){
    if (search_strs[i].find(s) !=std::string::npos ){
      vals.push_back(search_ids[i]);
    }
  }
  return vals;
}



Highlighter::Highlighter(TwBar * info_bar, std::vector<VisElem> * vis_elems){
  info_bar_ = info_bar;
  vis_elems_ = vis_elems;

  info_bar_index=-1;
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



void Highlighter::run_search(){

}



void Highlighter::fill_info_bar(){
  if (hl_inds.size() == 1){
    int el = hl_inds.front();
    if (el == info_bar_index) return;
    else  TwRemoveAllVars(info_bar_);
    info_bar_index = el;

    cout<<"setting visible"<<endl;
    int visible = 1;
    TwSetParam(info_bar_, NULL, "visible", TW_PARAM_INT32, 1, &visible);
    (*vis_elems_)[el].update_all_equations();

    int n_labels, n_equations, n_str_tags;
    std::string ** labels;
    std::string ** tags;
    std::string ** tag_vals;
    std::string ** eq_labels;
    float ** eq_vals;
    cout<<"getting all info" <<endl;
    (*vis_elems_)[el].get_all_info(n_labels, labels,
			       n_str_tags, tags, tag_vals,
			       n_equations,  eq_labels, eq_vals);
    
    //TW_TYPE_FLOAT
    //TW_TYPE_STDSTRING


    string group_label = string(" group=") + (*(labels[0])) + string( " ");
    //cout<<"group label:"<<group_label.c_str() <<endl;

    for (int i=0; i < n_str_tags; i++)
      TwAddVarRO(info_bar_, (*tags[i]).c_str(), TW_TYPE_STDSTRING, 
		 tag_vals[i], group_label.c_str());
    for (int i=0; i < n_equations; i++)
      TwAddVarRO(info_bar_, (*eq_labels[i]).c_str(), TW_TYPE_FLOAT, 
		 eq_vals[i], group_label.c_str());
  }

}

void Highlighter::update_info_bar(){
  if (info_bar_index >= 0){
    TwRefreshBar(info_bar_);
    (*vis_elems_)[info_bar_index].update_all_equations();
  }
}



void Highlighter::clear_hls(){
  info_bar_index = -1;
  TwRemoveAllVars(info_bar_);
  int visible = 0;
  TwSetParam(info_bar_, NULL, "visible", TW_PARAM_INT32, 1, &visible);

  while (!(hl_inds.empty())){
    int el = hl_inds.front();
    (*vis_elems_)[el].set_not_highlighted();
    hl_inds.pop_front();
    hl_colors.pop_front();
  }
}

void Highlighter::add_hl(int index){
  for (auto iter = hl_inds.begin(); iter != hl_inds.end(); ++iter) {
    if (*iter == index) return;
  }

  vec3 hcol =  get_hl_color(hl_inds.size());
  (*vis_elems_)[index].set_highlighted(hcol);
  hl_inds.push_back(index);
  hl_colors.push_back(hcol);
}

std::list<int> Highlighter::get_plot_inds(){
  return hl_inds;
}
std::list<glm::vec3> Highlighter::get_plot_colors(){
  return hl_colors;
}

//random utils

vec3 Highlighter::get_hl_color(int ind){
  vec3 hsv_col;  // 0-360, 0-1, 0-1 are the units  
  hsv_col[0] = (ind * 67)%360;
  hsv_col[1] = 1;
  hsv_col[2] = 1;
  
  return rgbColor(hsv_col);
}

