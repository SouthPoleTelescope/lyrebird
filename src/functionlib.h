#pragma once
#include <string>

#define FUNC_LIB_MAX_ARGS 5

typedef float (* fl_func)(float * inputs[FUNC_LIB_MAX_ARGS]);

fl_func get_fl_func(std::string, int * nargs);
