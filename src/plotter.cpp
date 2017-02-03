#include "plotter.h"

#include <assert.h>
#include <math.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "numberlineart.h"

#include <math.h>

#include "shader.h"

using namespace std;


Plotter::Plotter(int max_num_points){
  max_num_points_ = max_num_points;
  plot_buffer_ = vector<float>(max_num_points * 3, 0.0);

  string fragment_shader = R"(#version 330 core
    in vec4 fragColor;
    out vec4 color;
    void main(){
    color = fragColor;
    }
)";

  string vertex_shader = R"(#version 330 core
layout(location = 0) in vec3 vertexPosition;
out vec4 fragColor;

uniform mat4 MP;
uniform vec4 color;
 
void main(){
    gl_Position = MP * vec4(vertexPosition,1);
    fragColor = color;
}
)";

  //compile the shaders
  programID = LoadShadersDef(vertex_shader,fragment_shader);

  //load attributes and uniforms
  vertex_pos_shader_attrib_ = glGetAttribLocation(programID, "vertexPosition");
  vertex_color_shader_uniform_ = glGetUniformLocation(programID, "color");
  view_matrix_uniform_ = glGetUniformLocation(programID, "MP");
  

  glGenBuffers(1, &line_vert_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, line_vert_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 4 * max_num_points_ * sizeof(float), NULL, GL_STREAM_DRAW);
  

  float squareVerts[] = {-1,-1,-0.95, 1,-1,-0.95, -1,1,-0.95, 
			 1,-1,-0.95, -1,1,-0.95, 1,1,-0.95 };
  glGenBuffers(1, &square_vert_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, square_vert_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), squareVerts, GL_STATIC_DRAW);


  glGenBuffers(1, &simple_line_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, simple_line_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), squareVerts, GL_STREAM_DRAW);

  
  float fgVerts[] = {-1,-1,-0.95,  -1,1,-0.95,
		     -1,1,-0.95,  1,1,-0.95,
		     1,1,-0.95, 1,-1,-0.95,
		     1,-1,-0.95, -1,-1,-0.95,

		     0.,-1,-0.95, 0., -0.9,-0.95,

		     0.5,-1,-0.95, 0.5, -0.925,-0.95,
		     -0.5,-1,-0.95, -0.5, -0.925,-0.95,

		     -0.25,-1,-0.95, -0.25, -0.95,-0.95,
		     -0.75,-1,-0.95, -0.75, -0.95,-0.95,
		     0.25,-1,-0.95, 0.25, -0.95,-0.95,
		     0.75,-1,-0.95, 0.75, -0.95,-0.95,

		     0.,1,-0.95, 0., 0.9,-0.95,

		     0.5,1,-0.95, 0.5, 0.925,-0.95,
		     -0.5,1,-0.95, -0.5, 0.925,-0.95,

		     -0.25,1,-0.95, -0.25, 0.95,-0.95,
		     -0.75,1,-0.95, -0.75, 0.95,-0.95,
		     0.25,1,-0.95, 0.25, 0.95,-0.95,
		     0.75,1,-0.95, 0.75, 0.95,-0.95,
  };

  n_fg_lines_ = 18;
  glGenBuffers(1, &fg_vert_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, fg_vert_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 6 * n_fg_lines_ * sizeof(float), fgVerts, GL_DYNAMIC_DRAW);

}


Plotter::~Plotter() {
	glDeleteBuffers(1, &square_vert_buffer_);
	glDeleteBuffers(1, &simple_line_buffer_);
	glDeleteBuffers(1, &line_vert_buffer_);
	glDeleteBuffers(1, &fg_vert_buffer_);
}


void Plotter::prepare_plotting(glm::vec2 center, glm::vec2 size){
	view_transform_ = glm::mat4(1);
	view_transform_[0][0] = size.x;
	view_transform_[1][1] = size.y;
	view_transform_[3][0] = center.x;
	view_transform_[3][1] = center.y;
	
	glUseProgram(programID);
  
}

void Plotter::plotBG(glm::vec4 color){

	glUniformMatrix4fv(view_matrix_uniform_,  1, GL_FALSE, &view_transform_[0][0]);
	glEnableVertexAttribArray(vertex_pos_shader_attrib_);
	glBindBuffer(GL_ARRAY_BUFFER, square_vert_buffer_);
	glVertexAttribPointer(vertex_pos_shader_attrib_,
			      3,                  // size
			      GL_FLOAT,           // type
			      GL_FALSE,           // normalized?
			      0,                  // stride
			      (void*)0            // array buffer offset      
		);
	
	glUniform4fv(vertex_color_shader_uniform_, 1, &( color[0]  ));
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(vertex_pos_shader_attrib_);
}





void Plotter::plotFG(glm::vec4 color){
  glUniformMatrix4fv(view_matrix_uniform_,  1, GL_FALSE, &view_transform_[0][0]);
  glEnableVertexAttribArray(vertex_pos_shader_attrib_);
  glBindBuffer(GL_ARRAY_BUFFER, fg_vert_buffer_);
  glVertexAttribPointer(vertex_pos_shader_attrib_,
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset      
			);
  
  glUniform4fv(vertex_color_shader_uniform_, 1, &( color[0]  ));
  glDrawArrays(GL_LINES, 0, n_fg_lines_*2);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(vertex_pos_shader_attrib_);
  
}

void Plotter::plot(float * vals, int n_elems, 
		   float min, float max, 
		   glm::vec4 color, 
		   int is_log_scale,
		   float x_start, float x_sep

	){
	assert(n_elems <= max_num_points_);

	#ifdef DEBUG_LIKE_MAD
	if (! is_log_scale) {
		for (int i=0; i < n_elems; i++) {
			printf("%d %f\n", i, vals[i]);
		}
	}
	#endif

	int low_val = ceilf(logf(x_start)/logf(10));
	int high_val = ceilf(logf(x_start + x_sep * n_elems)/logf(10));
	int n_mags = high_val - low_val + 1;
	
	
	int n_vlines = 10 * (1 + high_val-low_val);
	float * vlines = NULL;
	float * vline_height = NULL;
	std::vector<float> vline_tmps(n_vlines, 0);
		
	if (is_log_scale) {
		vlines = new float[n_vlines];
		vline_height = new float[n_vlines];
		for (int j = 0; j < high_val-low_val + 1; j++){
			for (int i=0; i<10; i++) {
				vlines[i + j * 10] = (i + 1)  * powf(10.0, j + low_val);
				vline_height[i + j * 10] =  0.5/ ((float) ( 1 + (i) % 10));
			}
		}
		for (int i=0; i < n_vlines; i++) {
			vline_tmps[i] = (vlines[i] - x_start) / x_sep;
		}
	}

	if (!is_log_scale){
		//cout<<"in plotter min: "<< min << " max: "<< max <<endl;
		for (int i=0; i < n_elems; i++){
			plot_buffer_[i*3+1] = (vals[i]-min)/(max-min)*2 -1;
			if (plot_buffer_[i*3+1] < -1) plot_buffer_[i*3+1] = -1;
			if (plot_buffer_[i*3+1] > 1) plot_buffer_[i*3+1] = 1;
			plot_buffer_[i*3] = ((float)i)/((float)n_elems) * 2 -1;
			plot_buffer_[i*3 + 2] = -0.97;
		}
	}else{
		for (int i=0; i < n_elems; i++){
			//plot_buffer_[i*3+1] = (vals[i]-min)/(max-min);
			plot_buffer_[i*3+1] = (vals[i]-min)/(max-min)*2 -1;
			//plot_buffer_[i*3+1] = 2*(vals[i]/(max-min) + min) -1;
			if (plot_buffer_[i*3+1] < -1) plot_buffer_[i*3+1] = -1;
			if (plot_buffer_[i*3+1] > 1) plot_buffer_[i*3+1] = 1;
			plot_buffer_[i*3] = log2((float)i+1.0f) /(log2(n_elems) - log2(1)) * 2 - 1;
			plot_buffer_[i*3 + 2] = -0.97;
		}
		
		for (int i=0; i < n_vlines; i++) {
			vline_tmps[i] = log2((float)vline_tmps[i]+1.0f) /(log2(n_elems) - log2(1)) * 2 - 1; 
		}
	}
	
	glUniformMatrix4fv(view_matrix_uniform_,  1, GL_FALSE, &view_transform_[0][0]);
	glUniform4fv(vertex_color_shader_uniform_, 1, &( color[0]  ));
	
	glEnableVertexAttribArray(vertex_pos_shader_attrib_);  
	glBindBuffer(GL_ARRAY_BUFFER, line_vert_buffer_);
	glBufferSubData(GL_ARRAY_BUFFER, 0, n_elems * 3 * sizeof(GLfloat), &( plot_buffer_[0]));
	glVertexAttribPointer(vertex_pos_shader_attrib_,
			      3,                  // size
			      GL_FLOAT,           // type
			      GL_FALSE,           // normalized?
			      0,                  // stride
			      (void*)0            // array buffer offset      
		);
	
	glDrawArrays(GL_LINE_STRIP, 0, n_elems);
	
	if (is_log_scale) {
	
		for (int i=0; i < n_vlines; i++) {
			float line_buff[6];
			line_buff[0] = vline_tmps[i];
			line_buff[3] = vline_tmps[i];
			line_buff[1] = -1 + vline_height[i];
			line_buff[4] = -1;
			line_buff[2] = -0.99;
			line_buff[5] = -0.99;
			
			glm::vec4 col_line(1,1,1,1);
			
			glUniform4fv(vertex_color_shader_uniform_, 1, &( col_line[0]  ));
			glBindBuffer(GL_ARRAY_BUFFER, simple_line_buffer_);
			glEnableVertexAttribArray(vertex_pos_shader_attrib_);
			glVertexAttribPointer(vertex_pos_shader_attrib_, 3,GL_FLOAT,GL_FALSE,	
					      0, (void*)0  );
			glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(GLfloat), line_buff);
			glDrawArrays(GL_LINE_STRIP, 0, 2);
		}


		for (int pow = low_val; pow < high_val; pow++) {
			float y_pos = -0.5;
			float x_pos = powf(10, pow);
			x_pos = (x_pos - x_start) / x_sep;
			x_pos =  log2((float)x_pos+1.0f) /(log2(n_elems) - log2(1)) * 2 - 1;
			
			std::vector<GLfloat> vs;
			get_power_of_ten_number_lines(pow, 0.05, x_pos,y_pos,-0.99, vs);
			glm::vec4 col_line(1,1,1,1);

			GLuint number_buffer;
			glGenBuffers(1, &number_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, number_buffer);
			glBufferData(GL_ARRAY_BUFFER, 3 * vs.size() * sizeof(GLfloat), 
				     &(vs[0]), GL_STREAM_DRAW);
			glUniform4fv(vertex_color_shader_uniform_, 1, &( col_line[0]  ));
			glEnableVertexAttribArray(vertex_pos_shader_attrib_);
			glVertexAttribPointer(vertex_pos_shader_attrib_, 3,GL_FLOAT,GL_FALSE,	0, (void*)0  );
			glBindBuffer(GL_ARRAY_BUFFER, number_buffer);
			glDrawArrays(GL_LINES, 0, vs.size()/3 );
			glDeleteBuffers(1, &number_buffer);
		}
	}
	
	
	if (is_log_scale) {
		delete [] vlines;
		delete [] vline_height;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(vertex_pos_shader_attrib_);
	
	glLineWidth(1);
}

void Plotter::cleanup_plotting(){
	glUseProgram(0);
}
