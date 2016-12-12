#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "datavals.h"

#define MAX_PP_STACK_SIZE 64

//color map defs
typedef glm::vec4 (* color_map_t)(float val);

//polish prefix parser defs
template <class T> struct PPStack{
  size_t size;
  T items[MAX_PP_STACK_SIZE];
};
typedef void (* pp_func)( PPStack<float> * pp_val_stack, float * val, int offset);

struct PPToken{
	pp_func func;
	int arg_num;
	float val;
	float * val_addr;
	int dv_index;
};


// equation class defs
struct equation_desc{
	std::string eq;
	std::string cmap_id;
	std::string label;
	std::string display_label;
	std::string sample_rate_id;
	bool display_in_info_bar;
	bool color_is_dynamic;
};
typedef struct equation_desc equation_desc;


class VisElem;

class Equation{
	friend class VisElem;
public:
	Equation();
	void set_equation(DataVals * dvs, equation_desc desc);
	float get_value();
	void get_bulk_value(float * v);
	glm::vec4 get_color(size_t index);
	std::string get_label();
	std::string get_display_label();
	float * get_value_address();

	float get_sample_rate();

	bool display_in_info_bar() const {return display_in_info_bar_;}
private:
	const Equation& operator=( const Equation& );
	
	bool is_set;
	//fl_func eq_func;
	//float * eq_inputs[FUNC_LIB_MAX_ARGS];  
	//int eq_indices[FUNC_LIB_MAX_ARGS];
	
	PPStack<PPToken> ppp_stack;
	
	int n_args;
	color_map_t cmap;
	std::string label_;
	std::string display_label_;
	DataVals * data_vals;
	
	float cached_value;

	int sample_rate_index;
	
	bool display_in_info_bar_;
	
	bool color_is_dynamic_;
	float dynamic_min_val_;
	float dynamic_max_val_;
};
