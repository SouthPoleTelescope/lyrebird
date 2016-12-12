#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <AntTweakBar.h>
#include "glm/glm.hpp"

#include "simplerender.h"
#include "datavals.h"
#include "equation.h"
#include "equationmap.h"

struct vis_elem_repr{
  float x_center;
  float y_center;
  float x_scale;
  float y_scale;
  float rotation;
  int layer;

  std::string svg_path;
  std::string highlight_svg_path;

  std::string geo_id;
  std::string highlight_geo_id;

  std::vector<std::string> labels;

  std::vector< std::string > equations;
  std::string group;

  std::vector< std::string > labelled_data;
  std::vector< std::string > labelled_data_vs;
};

typedef struct vis_elem_repr vis_elem_repr;


class VisElem{
  /**
     Represents a visual element.


     I've kind of made a sketchy choice, so if you change any of the labelled data,
     labels, or equations you need to call updateAIPointers

   **/

 public:
  VisElem(SimpleRen * simple_ren, EquationMap * eqs,
	  vis_elem_repr v );
  
  void set_drawn();
  void set_not_drawn();
  bool is_drawn();


  void set_highlighted(glm::vec3 col);
  void set_not_highlighted();
  
  void update_color(size_t index);  
  void update_all_equations();

  glm::mat4 get_ms_transform();//ms = model space
  std::string get_geo_id();
  
  int get_layer();
  Equation & get_current_equation();

  void animate_highlight(float tstep);

  void set_eq_ind(unsigned int ind);
  int get_num_eqs();

  void get_all_info(std::vector<std::string> &labels, std::vector<std::string> & tags,
		    std::vector<std::string*> & tag_vals,
		    std::vector<std::string> & eq_labels,std::vector<float*> & eq_addrs
		    );

  bool string_matches_labels(const char * pattern);
  bool string_matches_labels_quick(const char * pattern);

  std::string get_group();

  std::vector< std::string > labels;
 private:
  //VisElem( const VisElem& );
  const VisElem& operator=( const VisElem& );

  int has_eq_;
  int simple_ren_index_;
  int highlight_index_;
  int layer_;
  std::string geo_id_;
  SimpleRen * s_ren;
  float hXScale, hYScale, hTDelt;

  
  std::vector<int> equation_inds_;
  EquationMap * equation_map_;
  int eq_ind_;

  std::string group_;
	
	
  std::vector< std::string > l_data_labels; //labelled data things
  std::vector< std::string > l_data_vals;
  bool is_highlighted_;
};


typedef std::shared_ptr<VisElem> VisElemPtr;
