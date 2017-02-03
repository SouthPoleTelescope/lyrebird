#include "equation.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <tgmath.h>
#include <ctype.h>
#include "glm/gtx/color_space.hpp"
#include "genericutils.h"

#include <dfmux/Housekeeping.h>

using namespace std;


/////////////////////////////////////
// Color map portion
/////////////////////////////////////


glm::vec4 white_cmap(float val){
  val = val > 1.0 ? 1.0 : val;
  val = val < 0.0 ? 0.0 : val;
  return glm::vec4(val,val,val,1.0);
}



glm::vec4 white_cmap_fs(float val){
  val = val > 1.0 ? 1.0 : val;
  val = val < -1.0 ? -1.0 : val;

  if (val == 0){
	  return glm::vec4(0,0,0,1);
  } else {
	  return white_cmap( (val + 1) / 2 );
  }
}


glm::vec4 red_cmap(float val){
  val = val > 1.0 ? 1.0 : val;
  val = val < 0.0 ? 0.0 : val;
  return glm::vec4(val,0,0,1.0);
}

glm::vec4 green_cmap(float val){
  val = val > 1.0 ? 1.0 : val;
  val = val < 0.0 ? 0.0 : val;
  return glm::vec4(0,val,0,1.0);
}

glm::vec4 blue_cmap(float val){
  val = val > 1.0 ? 1.0 : val;
  val = val < 0.0 ? 0.0 : val;
  return glm::vec4(0,0,val,1.0);
}


glm::vec4 rainbow_cmap(float val){
  val = val > 1.0 ? 1.0 : val;
  val = val < 0.0 ? 0.0 : val;
  glm::vec3 hsv_col;
  hsv_col[0] = 240 * (1-val);
  hsv_col[1] = 1;
  hsv_col[2] = 1;
return glm::vec4(glm::rgbColor(hsv_col),1.0);
}

glm::vec4 rainbow_cmap_fs(float val){
	val = val > 1.0 ? 1.0 : val;
	val = val < -1.0 ? -1.0 : val;
	if (val == 0){
		return glm::vec4(0,0,0,1);
	}
	else {
		return rainbow_cmap( (val + 1) / 2 );
	}
}




glm::vec4 phase_cmap(float val){
	// -pi to pi
	return rainbow_cmap(fabsf(val) / 3.14159265);
}

glm::vec4 bolo_cmap(float val, glm::vec4 base_color){
	glm::vec4 ret_vec;
	const float white_cutoff = 0.95;
	const float low_cutoff = 0.3;
	if (val == 0 || ! isfinite(val)) {
		return glm::vec4(0.8, 0.8, 0.8, 0.4);
	}else if (val < low_cutoff) 
		ret_vec = glm::vec4(1.0,0.0,0.0,1.0);
	else if (val >= 1.2) 
		ret_vec = glm::vec4(1.0,0.5,0.0,1.0);
	else if (val > white_cutoff && val < 1.0) 
		ret_vec =  glm::vec4(1.0,1.0,1.0,1.0)*(val-white_cutoff)/(1.01f-white_cutoff) + 
			   (1.0f - (val-white_cutoff)/(1.01f-white_cutoff)) * base_color;
	else if (val >= 1.0) 
		ret_vec =  glm::vec4(1.0,1.0,1.0,1.0);
	else  
		ret_vec =(val-low_cutoff)/(1.0f-low_cutoff) *base_color;
	ret_vec.a = 1.0;
	return ret_vec;
}



glm::vec4 bolo_blue_cmap(float val){
	glm::vec4 base_color(0.0,0.0,1.0,1.0);
	return bolo_cmap(val,  base_color );
}

glm::vec4 bolo_cyan_cmap(float val){
	glm::vec4 base_color(0.0,1.0,1.0,1.0);
	return bolo_cmap(val,  base_color );
}

glm::vec4 bolo_purple_cmap(float val){
	glm::vec4 base_color(0.7,0.0,0.7,1.0);
	return bolo_cmap(val,  base_color );
}

glm::vec4 bolo_green_cmap(float val){
	glm::vec4 base_color(0.0,1.0,0.0,1.0);
	return bolo_cmap(val,  base_color );
}




color_map_t get_color_map(std::string n){
	if (n=="red_cmap"){
	  return red_cmap;
  }else if (n=="green_cmap"){
	  return green_cmap;
  }else if (n=="bolo_green_cmap"){
	  return bolo_green_cmap;
  }else if (n=="bolo_blue_cmap"){
	  return bolo_blue_cmap;
  }else if (n=="bolo_purple_cmap"){
	  return bolo_purple_cmap;
  }
  else if (n=="blue_cmap"){
	  return blue_cmap;
  }
  
  else if (n=="phase_cmap"){
	  return phase_cmap;
  }

  else if (n=="bolo_cyan_cmap"){
	  return bolo_cyan_cmap;
  }
  else if (n=="white_cmap"){
	  return white_cmap;
  } else if (n=="white_cmap_fs"){
	  return white_cmap_fs;
  }else if (n=="rainbow_cmap"){
	  return rainbow_cmap;
  }else if (n=="rainbow_cmap_fs"){
	  return rainbow_cmap_fs;
  } else {
	  cout<<"Color map: "<< n <<" not recognized, giving you a white one"<<endl;
	  return white_cmap;
  }
}


///////////////////////////////////////////////
// Code for parsing the polish prefix equations
///////////////////////////////////////////////
// c with templates stack

template <class T> inline void push(PPStack<T> * ps, T x)
{
  if (ps->size >= MAX_PP_STACK_SIZE) {
    fputs("Error: stack overflow\n", stderr);
    abort();
  } else
    ps->items[ps->size++] = x;
}

template <class T> inline T pop(PPStack<T> *ps)
{
  if (ps->size == 0){
    fputs("Error: stack underflow\n", stderr);
    abort();
  } else
    return ps->items[--ps->size];
}

// function library

 

inline void pp_func_and(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0 && v1 ? 1.0f : 0.0f);
}
inline void pp_func_or(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0 || v1 ? 1.0f : 0.0f);
}

inline void pp_func_eq(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0 == v1 ? 1.0f : 0.0f);
}


inline void pp_func_not(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  push(pp_val_stack, !v0 ? 1.0f : 0.0f);
}

inline void pp_func_add(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0+v1);
}

inline void pp_func_atan(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, atan2f(v0, v1));
}

inline void pp_func_sub(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0-v1);
}
inline void pp_func_mul(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0*v1);
}
inline void pp_func_div(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, v0/v1);
}

inline void pp_func_fmod(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, fmod(v0, v1));
}


inline void pp_func_exp(PPStack<float> * pp_val_stack, float * val, int offset){
  float v0 = pop(pp_val_stack);
  float v1 = pop(pp_val_stack);
  push(pp_val_stack, (float)pow(v0, v1));
}
inline void pp_func_abs(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)fabs(pop(pp_val_stack) ));
}
inline void pp_func_cos(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)cos(pop(pp_val_stack) ));
}
inline void pp_func_sin(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)sin(pop(pp_val_stack) ));
}
inline void pp_func_tan(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)tan(pop(pp_val_stack) ));
}

inline void pp_func_sqrt(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)sqrt(pop(pp_val_stack) ));
}

inline void pp_func_push(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, *val);
}

inline void pp_func_push_offset(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, *(val + offset));
}

pp_func get_pp_func(char id){
  switch(id){
  case '|':
    return pp_func_or;
    break;
  case '=':
    return pp_func_eq;
    break;
  case '!':
    return pp_func_not;
    break;
  case '&':
    return pp_func_and;
    break;
  case '+':
    return pp_func_add;
    break;
  case '-':
    return pp_func_sub;
    break;
  case '*':
    return pp_func_mul;
    break;
  case '/':
    return pp_func_div;
    break;
  case '%':
    return pp_func_fmod;
    break;
  case 'a':
    return pp_func_abs;
    break;
  case '^':
    return pp_func_exp;
    break;
  case 's':
    return pp_func_sin;
    break;
  case 'c':
    return pp_func_cos;
    break;
  case 't':
    return pp_func_tan;
    break;
  case 'T':
    return pp_func_atan;
    break;
  case 'q':
    return pp_func_sqrt;
    break;
  default:
    return NULL;
    break;
  }
  return NULL;
}

//stores the change in the stack vals
int get_pp_func_len(char id){
  switch(id){
  case '|':
    return 1;
    break;
  case '=':
    return 1;
    break;
  case '!':
    return 0;
    break;
  case '&':
    return 1;
    break;
  case '+':
    return 1;
    break;
  case '-':
    return 1;
    break;
  case '*':
    return 1;
    break;
  case '/':
    return 1;
    break;
  case '%':
    return 1;
    break;
  case 'a':
    return 0;
    break;
  case '^':
    return 1;
    break;
  case 's':
    return 0;
    break;
  case 'c':
    return 0;
    break;
  case 't':
    return 0;
    break;
  case 'T':
    return 1;
    break;
  case 'q':
    return 0;
    break;
  default:
    return 0;
    break;
  }
  return 0;
}

int is_pp_func(char id){
  return ( (id == '+') || (id == '-') ||
	   (id == '*') || (id == '/') ||
	   (id == '^') || (id == 'a') ||
	   (id == 's') || (id == 'c') ||
	   (id == 't') || 
	   (id == '&') || (id == '|') ||
	   (id == '!') || (id == 'T') ||
	   (id == 'q') || (id == '=') ||
	   (id == '%')
	  );
}

//////////////////////////////////////
//string parsing

//from stack overflow  Alexey Frunze
char** pp_str_split(char* a_str, const char a_delim)
{
  char** result    = 0;
  size_t count     = 0;
  char* tmp        = a_str;
  char* last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp) {
    if (a_delim == *tmp) {
      count++;
      last_comma = tmp;
      }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen(a_str) - 1);
  /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
  count++;
  result = (char**)malloc(sizeof(char*) * count);
  if (result) {
      size_t idx  = 0;
      char* token = strtok(a_str, delim);

      while (token) {
	  assert(idx < count);
	  *(result + idx++) = strdup(token);
	  token = strtok(0, delim);
        }
      assert(idx == count - 1); //this usually means that two spaces are in a row
      *(result + idx) = 0;
    }
  return result;
}


int is_numeric (const char * s)
{
  if (s == NULL || *s == '\0' || isspace(*s))
    return 0;
  char * p;
  strtod (s, &p);
  return *p == '\0';
}



//////////////////////////////////////
//list of functions
//get number of arguments pushed / pulled
void tokenize_equation_or_die(const char * eq, PPStack<PPToken> * out_stack, DataVals * data_vals){
  out_stack->size = 0;

  //check for weird (ok not so fucking weird, shut up) edge cases
  //check that it is not empty
  int eqlen = strlen(eq);
  if ( eqlen == 0){
    fprintf(stderr, "empty equation '%s'\n", eq);
    exit(1);
  }  
  //check if first character is space
  if ( isspace(eq[0]) ){
    fprintf(stderr, "equation begins with whitespace '%s'\n", eq);
    exit(1);
  }

  //check if last character is space
  if ( isspace(eq[eqlen-1]) ){
    fprintf(stderr, "equation ends with whitespace '%s'\n", eq);
    exit(1);
  }
  
  //check if there are two spaces in a row
  char twospace[] = "  ";
  if (strstr (eq, twospace) != NULL){
    fprintf(stderr, "two spaces found in '%s'\n", eq);
    exit(1);
  }

  char * eq_copy;
  eq_copy = (char *) malloc((eqlen + 1) * sizeof(char));
  strncpy(eq_copy, eq, eqlen + 1 );


  //printf("actually tokenizing, %s \n", eq);
  
  //tokenize equation
  char ** split_eq = pp_str_split(eq_copy, ' ');
  if (split_eq){
    for (int i = 0; *(split_eq + i); i++){
      //printf("Tokenizing:  %s\n",  *(split_eq + i));
      //check if len 1 and func
      char * eqt = *(split_eq + i);
      PPToken tok;

      tok.val_addr = NULL;
      tok.dv_index = 0;
      if (strlen(eqt) == 1 && is_pp_func(eqt[0])){
	tok.func = get_pp_func(eqt[0]);
	tok.arg_num = get_pp_func_len(eqt[0]);
	tok.val = 0;
      } else if ( is_numeric( eqt )){
	tok.arg_num = -1;
	tok.val = atof(eqt);
	tok.func = pp_func_push;
      } else if ( data_vals->get_ind(  string(eqt)   ) != -1){
	tok.arg_num = -1;
	tok.val = -1;

	if (data_vals->is_buffered( data_vals->get_ind(  string(eqt) ))) {
		tok.func = pp_func_push_offset;
	} else {
		tok.func = pp_func_push;
	}
	tok.dv_index = data_vals->get_ind(string(eqt));
	tok.val_addr = data_vals->get_addr( tok.dv_index );
      }
      else{
	fprintf(stderr, "Token '%s' is not recognized\n", eqt);
	exit(1);
      }
      push(out_stack, tok);
      free( *(split_eq + i));
    }
    free(split_eq);
  }else{
    fprintf(stderr, "pp_str_split failed miserably on '%s'\n", eq);
    exit(1);
  }
  //check that it's a valid equation
  int check_val = 0;
  for (int i = out_stack->size-1; i >= 0; i--){
    check_val += out_stack->items[i].arg_num;
    //printf("%d\n",check_val);
    if (check_val > 0){
      fprintf(stderr, "equation is not valid, too many operators before variables '%s'\n", eq);
      exit(1);
    }
  }
  //check that we end with only one value on the stack
  if (check_val != -1){
    fprintf(stderr, "equation does not have a definite result '%s'\n", eq);
    exit(1);
  }
  free(eq_copy);

  //printf("done tokenizing, %s \n", eq);
}

float evaluate_tokenized_equation_or_die(PPStack<PPToken> * token_stack){
  PPStack<float> eval_stack;
  eval_stack.size = 0;
  for (int i = token_stack->size-1; i >= 0; i--){
    token_stack->items[i].func(&eval_stack, 
			       token_stack->items[i].val_addr == NULL ? &(token_stack->items[i].val) : token_stack->items[i].val_addr, 
			       0 );
  }
  return eval_stack.items[0];
}



/////////////////////////////////////
// Actual public interface portion
/////////////////////////////////////

Equation::Equation(){
  is_set = false;
  display_in_info_bar_ = true;
}


float Equation::get_sample_rate() {
	return data_vals->get_sample_rate(sample_rate_index);
}

void Equation::set_equation(DataVals * dvs, 
			    equation_desc desc){
  is_set=true;
  data_vals = dvs;
  tokenize_equation_or_die(desc.eq.c_str(), &ppp_stack, data_vals);
  cmap =  get_color_map(desc.cmap_id);
  label_ = desc.label;
  display_label_ = desc.display_label;
  sample_rate_index = data_vals->get_ind(desc.sample_rate_id);
  display_in_info_bar_ = desc.display_in_info_bar;
  color_is_dynamic_ = desc.color_is_dynamic;
}


float Equation::get_value(){
  if (is_set){
    cached_value = evaluate_tokenized_equation_or_die(&ppp_stack);
  }else{
    cached_value = 0.0f;
  }
  return cached_value;
}


void Equation::get_bulk_value(float * v){
  //currently a stub  
  if (is_set){
    data_vals->apply_bulk_func(&ppp_stack, v);
  }
}

glm::vec4 Equation::get_color(size_t index){
	float value;
	if (color_is_dynamic_){
		if (index == 0) {
			int buff_size = data_vals->get_buffer_size();
			float * v = new float[buff_size];
			get_bulk_value(v);
			dynamic_min_val_ = v[0];
			dynamic_max_val_ = v[1];
			for (int i=1; i < buff_size; i++) {
				if (v[i] < dynamic_min_val_){
					dynamic_min_val_ = v[i];
				} 
				if (v[i] > dynamic_max_val_){
					dynamic_max_val_ = v[i];
				} 
			}
			delete[] v;
		}
		value = (get_value() - dynamic_min_val_)/ (dynamic_max_val_ - dynamic_min_val_);
	} else {
		value = get_value();
	}
	glm::vec4 col = cmap(value);
	return col;
}

string Equation::get_label(){
  return label_;
}
string Equation::get_display_label(){
  return display_label_;
}

float * Equation::get_value_address(){
  return &cached_value;
}
