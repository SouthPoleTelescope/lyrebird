#include "simplerender.h"

#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <float.h>


#include "shader.h"
#include "geometryutils.h"
#include "genericutils.h"

using namespace std;


glm::mat4 get_m_transmat(float x_center, float y_center, 
			 float x_scale, float y_scale,
			 float rotation, int layer){
  glm::mat4 trans_matrix(1.0);
  trans_matrix[0][0] = x_scale*cosf(rotation);
  trans_matrix[0][1] = x_scale*sinf(rotation);
  trans_matrix[1][0] = y_scale*-1 *sinf(rotation);
  trans_matrix[1][1] = y_scale*cosf(rotation);

  trans_matrix[3][0] = x_center;
  trans_matrix[3][1] = y_center;
  trans_matrix[3][2] = 0.1+((float)layer);


  //glm::mat4 trans_matrix2 = glm::rotate(rotation, 0.0, 0.0, 1.0);
  //glm::rotate (detail::tmat4x4< T > const &m, T const &angle, detail::tvec3< T > const &v)
  //glm::scale (detail::tmat4x4< T > const &m, detail::tvec3< T > const &v)
  //glm::translate (detail::tmat4x4< T > const &m, detail::tvec3< T > const &v)

  //detail::tmat4x4< T > translate (T x, T y, T z)
  //detail::tmat4x4< T > scale (T x, T y, T z)
  //detail::tmat4x4< T > rotate (T angle, T x, T y, T z)
  return trans_matrix;
}


render_state get_empty_ren_state(){
  render_state rs = {0};
  rs.x_scale = 1.0;
  rs.y_scale = 1.0;
  rs.col_r = 1.0;
  rs.col_g = 1.0;
  rs.col_b = 1.0;
  rs.col_a = 1.0;
  rs.geo_index = -1;
  rs.layer = -1;
  return rs;
}


SimpleRen::SimpleRen(){

  string fragment_shader = R"(
#version 330 core
// Interpolated values from the vertex shaders
in vec4 fragColor;
// Ouput data
out vec4 color;
void main(){
    color = fragColor;
}
)";

  string vertex_shader = R"(
#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec4 vertexCol;
layout(location = 3) in vec4 ColVec;
layout(location = 4) in vec4 MM0;
layout(location = 5) in vec4 MM1;
layout(location = 6) in vec4 MM2;
layout(location = 7) in vec4 MM3;

out vec4 fragColor;

uniform mat4 MP;
 
void main(){ 
  mat4 MM = mat4(MM0,MM1,MM2,MM3);
  gl_Position = MP * MM * vec4(vertexPosition_modelspace,1);
  fragColor = ColVec * vertexCol;
}
)";

  s_progID = LoadShadersDef(vertex_shader,fragment_shader);

  s_vertMID   = glGetAttribLocation(s_progID, "vertexPosition_modelspace");
  s_vertUVID  = glGetAttribLocation(s_progID, "vertexUV");
  s_vert_colID = glGetAttribLocation(s_progID, "vertexCol");
  s_view_matID = glGetUniformLocation(s_progID, "MP");;
  s_mod_matID[0] = glGetAttribLocation(s_progID, "MM0");
  s_mod_matID[1] = glGetAttribLocation(s_progID, "MM1");
  s_mod_matID[2] = glGetAttribLocation(s_progID, "MM2");
  s_mod_matID[3] = glGetAttribLocation(s_progID, "MM3");

  s_uni_colID = glGetAttribLocation(s_progID, "ColVec");
  ren_precalced = false;

}



void SimpleRen::precalc_ren(){
  assert(!ren_precalced);
  cout<<"running precalc"<<endl;
  ren_precalced = true;
  n_ren_states = ren_wraps.size();


  for (int i = 0; i < 4; i++){
    transbuf[i] = vector<GLfloat>( 4 * n_ren_states, 0.0);
    glGenBuffers(1, &(elem_trans_gpu_buffer[i]));
    glBindBuffer(GL_ARRAY_BUFFER, elem_trans_gpu_buffer[i]);
    // Initialize with empty (NULL) buffer : it will be updated later, each frame.  
    glBufferData(GL_ARRAY_BUFFER, n_ren_states * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
  }


  colbuf   = vector<GLfloat>( 4  * n_ren_states, 0.0);
  glGenBuffers(1, &elem_color_gpu_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, elem_color_gpu_buffer);
  // Initialize with empty (NULL) buffer : it will be updated later, each frame.  
  glBufferData(GL_ARRAY_BUFFER, n_ren_states * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

  for (int i=0; i < n_ren_states; i++){
    bool isThere = false;
    for (int j = 0; j < unique_geos.size(); j++){
      if (unique_geos[j] == ren_wraps[i].rs.geo_index){
	isThere = true;
	break;
      }
    }
    if (!isThere){
      unique_geos.push_back(ren_wraps[i].rs.geo_index);
    }
  }
  cout<<"done precalc"<<endl;
}



int SimpleRen::add_ren_state(render_state rs){
  ren_wrap rw;
  rw.rs = rs;
  rw.is_drawn = 0;
  rw.m_transmat = get_m_transmat(rs.x_center, rs.y_center,
				 rs.x_scale, rs.y_scale,
				 rs.rotation, rs.layer);
  rw.color = glm::vec4(rs.col_r, rs.col_g, rs.col_b, rs.col_a );
  int ci = ren_wraps.size();
  ren_wraps.push_back(rw);
  return ci;
}


glm::mat4 SimpleRen::get_ms_transform(int ind){
  return ren_wraps[ind].m_transmat;
}

render_state SimpleRen::get_ren_state(int ind){
  return ren_wraps[ind].rs;
}
			    
void SimpleRen::set_drawn(int ind){
  ren_wraps[ind].is_drawn = true;
}

void SimpleRen::set_not_drawn(int ind){
    ren_wraps[ind].is_drawn = false;
}

void SimpleRen::set_color(int ind, glm::vec4 new_color){
  ren_wraps[ind].color = new_color;
  ren_wraps[ind].rs.col_r = new_color.r;
  ren_wraps[ind].rs.col_g = new_color.g;
  ren_wraps[ind].rs.col_b = new_color.b;
  ren_wraps[ind].rs.col_a = new_color.a;  
}


void SimpleRen::set_scale(int ind, float xscale, float yscale){
  ren_wraps[ind].rs.x_scale = xscale;
  ren_wraps[ind].rs.y_scale = yscale;

  ren_wraps[ind].m_transmat = get_m_transmat(ren_wraps[ind].rs.x_center,
					    ren_wraps[ind].rs.y_center,
					    ren_wraps[ind].rs.x_scale,
					    ren_wraps[ind].rs.y_scale,
					    ren_wraps[ind].rs.rotation,
					    ren_wraps[ind].rs.layer
					    );


}

void SimpleRen::draw_ren_states(glm::mat4 view_matrix){
  if (!ren_precalced){
    print_and_exit("after adding render state you need to cal precalcRen");
  }
  
  glUseProgram(s_progID);
  glUniformMatrix4fv(s_view_matID,  1, GL_FALSE, &view_matrix[0][0]);

  for (int i = 0; i < unique_geos.size(); i++){
    int curInd = 0;
    int cur_geo_id = unique_geos[i];

    for (int j = 0; j < ren_wraps.size(); j++){
      if (!ren_wraps[j].is_drawn) continue;
      if (ren_wraps[j].rs.geo_index == cur_geo_id){
	colbuf[4*curInd+0] = ren_wraps[j].color.r;
	colbuf[4*curInd+1] = ren_wraps[j].color.g;
	colbuf[4*curInd+2] = ren_wraps[j].color.b;
	colbuf[4*curInd+3] = ren_wraps[j].color.a;
	for (int k =0; k<4; k++)
	  for (int l=0; l<4; l++)
	    transbuf[k][4*curInd + l] = ren_wraps[j].m_transmat[k][l];
	curInd++;
      }
    }
    //bind the vertices
    int nverts =  nverts = bind_buffer( cur_geo_id );

    for (int taco = 0; taco<4; taco++){
      glBindBuffer(GL_ARRAY_BUFFER, elem_trans_gpu_buffer[taco]);
      //glBufferData(GL_ARRAY_BUFFER, n_ren_states * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. 
      glBufferSubData(GL_ARRAY_BUFFER, 0, curInd * sizeof(GLfloat) * 4, &(transbuf[taco][0]));
    }

    glBindBuffer(GL_ARRAY_BUFFER, elem_color_gpu_buffer);
    //glBufferData(GL_ARRAY_BUFFER, n_ren_states * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. 
    glBufferSubData(GL_ARRAY_BUFFER, 0, curInd * sizeof(GLfloat) * 4, &(colbuf[0]));


    for (int taco=0; taco<4; taco++){
      //cout<<"enabling vert arr "<< s_mod_matID[taco]<<endl;
      glEnableVertexAttribArray(s_mod_matID[taco]);
      glBindBuffer(GL_ARRAY_BUFFER, elem_trans_gpu_buffer[taco]);
      glVertexAttribPointer(
			    s_mod_matID[taco],
			    4,                  // size
			    GL_FLOAT,           // type
			    GL_FALSE,           // normalized?
			    0,                  // stride
			    (void*)0            // array buffer offset 
			    );
    }

    glEnableVertexAttribArray(s_uni_colID);
    glBindBuffer(GL_ARRAY_BUFFER, elem_color_gpu_buffer);
    glVertexAttribPointer(
			  s_uni_colID,
			  4,                  // size
			  GL_FLOAT,           // type
			  GL_FALSE,           // normalized?
			  0,                  // stride
			  (void*)0            // array buffer offset 
			  );

    glVertexAttribDivisor(s_vertMID, 0); 
    glVertexAttribDivisor(s_vertUVID, 0); 
    glVertexAttribDivisor(s_vert_colID, 0); 

    glVertexAttribDivisor(s_mod_matID[0], 1);
    glVertexAttribDivisor(s_mod_matID[1], 1);
    glVertexAttribDivisor(s_mod_matID[2], 1);
    glVertexAttribDivisor(s_mod_matID[3], 1);

    glVertexAttribDivisor(s_uni_colID, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, nverts, curInd);

  }	 
  unbind_buffer();
}



int SimpleRen::load_def_geo(std::string id,
				 std::vector < glm::vec2 > & in_vertices,
				 std::vector < glm::vec2 > & uv_coordinates,
				 std::vector < glm::vec4 > & color
				 ){

  if (in_vertices.size() != uv_coordinates.size() && in_vertices.size() != color.size() ){
    print_and_exit("SimpleRen::load_def_geo all vectors need to be the same size");
  }
  std::vector< GLfloat > vertex_positions;
  std::vector< GLfloat > uvs;
  std::vector < GLfloat > colors;
  std::vector < unsigned int > indices;
  unsigned int cur_index = 0;

  //Load the data into a convenient format
  //check for redundant vertices
  for(unsigned int i=0; i < in_vertices.size(); i++){
    int vert_ind = get_vertex_index(vertex_positions, uvs, colors, 
				    in_vertices[i], uv_coordinates[i],color[i]);

    if (vert_ind == -1){
      //cout<<"Xv: "<<in_vertices[i].x<<"  Yv: "<<in_vertices[i].y<<endl;

      vertex_positions.push_back( in_vertices[i].x);
      vertex_positions.push_back( in_vertices[i].y);
      vertex_positions.push_back( 0.0f);

      uvs.push_back(uv_coordinates[i].x);
      uvs.push_back(uv_coordinates[i].y);

      colors.push_back(color[i].r);
      colors.push_back(color[i].g);
      colors.push_back(color[i].b);
      colors.push_back(color[i].a);

      indices.push_back(cur_index);
      cur_index++;
    } else{
      indices.push_back(vert_ind);
    }
  }


  //Bind the buffer
  GLuint vertexArrayID;
  GLuint uvbuffer;
  GLuint vertexbuffer;
  GLuint colbuffer;
  GLuint elementbuffer;

  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  glGenBuffers(1, &elementbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertex_positions.size(), &vertex_positions.front(), GL_STATIC_DRAW);


  glGenBuffers(1, &uvbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * uvs.size(), &uvs.front(), GL_STATIC_DRAW);


  glGenBuffers(1, &colbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * colors.size(), &colors.front(), GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);


  //store the information for posterity
  geo_info_t store_info;
  store_info.vertex_arrayID = vertexArrayID;
  store_info.uvbuffer = uvbuffer;
  store_info.vertexbuffer = vertexbuffer;
  store_info.colbuffer = colbuffer;
  store_info.elementbuffer = elementbuffer;
  store_info.n_verts = indices.size();
    

  int return_ind = geo_info.size();
  geo_info.push_back(store_info);
  geo_ids.push_back(id);
  geo_is_valid.push_back(true);
  return return_ind;
}



int SimpleRen::bind_buffer(int ind){
  geo_info_t gi =   geo_info[ind];


  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gi.elementbuffer);
  glEnableVertexAttribArray(s_vertMID);
  glBindBuffer(GL_ARRAY_BUFFER, gi.vertexbuffer);
  glVertexAttribPointer(
			s_vertMID,
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset      
			);
  
  glEnableVertexAttribArray(s_vertUVID);
  glBindBuffer(GL_ARRAY_BUFFER, gi.uvbuffer);
  glVertexAttribPointer(
			s_vertUVID,
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset 
			);
  
  glEnableVertexAttribArray(s_vert_colID);
  glBindBuffer(GL_ARRAY_BUFFER, gi.colbuffer);
  glVertexAttribPointer(
			s_vert_colID,
			4,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized? 
			0,                  // stride
			(void*)0            // array buffer offset 
			);

  return gi.n_verts;

}

void SimpleRen::unbind_buffer(){
  glDisableVertexAttribArray(s_vertMID);
  glDisableVertexAttribArray(s_vertUVID);
  glDisableVertexAttribArray(s_vert_colID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void SimpleRen::delete_buffer(int ind)
{
  geo_info_t gi = geo_info[ind];
  glDeleteBuffers(1, &(gi.uvbuffer));
  glDeleteBuffers(1, &(gi.vertexbuffer));
  glDeleteBuffers(1, &(gi.colbuffer));
  glDeleteVertexArrays(1, &(gi.vertex_arrayID));
}





int SimpleRen::load_square(){
  std::vector< glm::vec4 > out_colors;
  std::vector< glm::vec2 > out_verts;
  std::vector< glm::vec2 > out_uvs;

  glm::vec4 color = glm::vec4(1.0,1.0,1.0,1.0);

  out_verts.push_back( glm::vec2(0, 0)  );
  out_verts.push_back( glm::vec2(1, 0)  );
  out_verts.push_back( glm::vec2(0, 1)  );
  out_verts.push_back( glm::vec2(1, 0)  );
  out_verts.push_back( glm::vec2(0, 1)  );
  out_verts.push_back( glm::vec2(1, 1)  );

  out_uvs.push_back( glm::vec2(0, 0)  );
  out_uvs.push_back( glm::vec2(1, 0)  );
  out_uvs.push_back( glm::vec2(0, 1)  );
  out_uvs.push_back( glm::vec2(1, 0)  );
  out_uvs.push_back( glm::vec2(0, 1)  );
  out_uvs.push_back( glm::vec2(1, 1)  );
  for (int i=0; i < 6; i++) out_colors.push_back( color  );
  
  return load_def_geo("square", out_verts, out_uvs, out_colors);  
 
}


int SimpleRen::get_vertex_index(vector<GLfloat> vertex_positions,
				       vector<GLfloat> uvs,
				       vector<GLfloat> colors,
				       glm::vec2 vert, glm::vec2 uv, glm::vec4 color){

  for (unsigned int i = 0; i < uvs.size()/2; i++){
    bool vert_eq = sloppy_eq( vert.x, vertex_positions[i*3]) && sloppy_eq( vert.y, vertex_positions[i*3+1]);
    bool uv_eq = sloppy_eq( uv.x, uvs[i*2]) && sloppy_eq( uv.y, uvs[i*2+1]);
    bool col_eq = ( sloppy_eq( color[0], colors[i*4]) && sloppy_eq( color[1], colors[i*4+1]) &&
		    sloppy_eq( color[2], colors[i*4+2]) && sloppy_eq( color[3], colors[i*4+3]));
    if (vert_eq && uv_eq && col_eq) return i;
  }
  return -1;
}


int SimpleRen::load_svg_file(std::string id, std::string path){
  std::vector < glm::vec2 > out_vertices;
  std::vector < glm::vec2 > out_uvs;
  std::vector < glm::vec4 > out_color;
  con_svg_to_geo(path, 0.1,out_vertices,out_uvs, out_color );
  return load_def_geo(id, out_vertices, out_uvs, out_color);
}


int SimpleRen::get_geo_index(std::string id){
  for (int i = 0; i<geo_ids.size(); i++)
    if (geo_ids[i] == id) return i;
  return -1;

}
