#include "functionlib.h"

#include "genericutils.h"
#include <math.h>


float fl_cos_2(float * inputs[FUNC_LIB_MAX_ARGS]){
  return cos(*(inputs[0])) * cos(*(inputs[0]));
}

float fl_identity(float * inputs[FUNC_LIB_MAX_ARGS]){
  return *inputs[0];
}

float fl_add(float * inputs[FUNC_LIB_MAX_ARGS]){
  return *(inputs[0]) + *(inputs[1]);
}

float fl_sub(float * inputs[FUNC_LIB_MAX_ARGS]){
  return *(inputs[0]) - *(inputs[1]);
}

float fl_mul(float * inputs[FUNC_LIB_MAX_ARGS]){
  return *(inputs[0]) * *(inputs[1]);
}

float fl_div(float * inputs[FUNC_LIB_MAX_ARGS]){
  return *(inputs[0]) / *(inputs[1]);
}


fl_func get_fl_func(std::string id, int * nargs){

  if (id == "cos(a)^2"){
    *nargs = 1;
    return fl_cos_2;
  } else if (id == "a"){
    *nargs = 1;
    return fl_identity;
  }else{
    print_and_exit("Requesting unknown function");
    return fl_identity;
  }
}
