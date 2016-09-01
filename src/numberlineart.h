#pragma once
#include "glm/glm.hpp"
#include <vector>
#include <GL/glew.h>
void get_power_of_ten_number_lines(int pow, 
				   float scale_factor,
				   float x_offset,
				   float y_offset,
				   float z_offset,
				   std::vector<GLfloat> & out_vals );

