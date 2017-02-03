#pragma once

#include <GL/glew.h>
#include "glm/glm.hpp"
#include <string>
#include <vector>

class Plotter{
public:
	Plotter(int max_num_points);
	~Plotter();
	void prepare_plotting(glm::vec2 center, glm::vec2 size);
	void plotBG(glm::vec4 color);
	void plotFG(glm::vec4 color);
	
	void plot(float * vals, int n_elems, float min, float max, 
		  glm::vec4 color, int is_log_scale, 
		  float x_start, float x_sep
	  );
	void cleanup_plotting();

 private:
	GLuint programID;
	GLuint vertex_pos_shader_attrib_;
	GLuint vertex_color_shader_uniform_;
	GLuint view_matrix_uniform_;
	
	GLuint square_vert_buffer_;
	GLuint fg_vert_buffer_;

	GLuint line_vert_buffer_;
	
	GLuint simple_line_buffer_;
	
	
	int max_num_points_;
	std::vector<float> plot_buffer_;
	
	glm::mat4 view_transform_;
	
	int n_fg_lines_;
};
