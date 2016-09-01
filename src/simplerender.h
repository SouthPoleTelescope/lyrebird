#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include <GL/glew.h>
#include "glm/glm.hpp"

//stores all the addresses on the graphics card for things
struct geo_info_t{
  GLuint vertex_arrayID;
  GLuint uvbuffer;
  GLuint vertexbuffer;
  GLuint colbuffer;
  GLuint elementbuffer;
  GLuint n_verts;
};


//a simple representation of what we want drawn
struct render_state{
  float x_center;
  float y_center;
  
  float x_scale;
  float y_scale;
  
  float rotation;  
  
  float col_r;
  float col_g;
  float col_b;
  float col_a;
  
  int geo_index;
  
  int layer;  
};

typedef struct render_state render_state;


// a wrapper around render state that does some calculations to make it faster to render
struct ren_wrap{
  render_state rs;
  int layer_ind;
  int is_drawn;
  glm::mat4 m_transmat;
  glm::vec4 color;
};
typedef struct ren_wrap ren_wrap;

render_state get_empty_ren_state();

class SimpleRen{
 public:
  SimpleRen();

  int add_ren_state(render_state rs);

  render_state get_ren_state(int ind);
  
  void set_drawn(int ind);
  void set_not_drawn(int ind);
  bool is_drawn(int ind);
  bool is_still_valid();
  glm::mat4 get_ms_transform(int ind);

  void draw_ren_states(glm::mat4 view_matrix);
  void set_color(int ind, glm::vec4 new_color);
  void set_scale(int ind, float xscale, float yscale);
  void precalc_ren();



  int get_geo_index(std::string id);
  int bind_buffer(int ind);
  void unbind_buffer();
  void delete_buffer(int ind);
  void clean_out_buffers();




  int load_def_geo(std::string id,
		 std::vector < glm::vec2 > & in_vertices,
		 std::vector < glm::vec2 > & uv_coordinates,
		 std::vector < glm::vec4 > & colors);
  int load_square();
  int load_svg_file(std::string id, std::string path);


 private:
  SimpleRen(const SimpleRen&); //prevent copy construction      
  SimpleRen& operator=(const SimpleRen&); //prevent assignment


  std::vector<ren_wrap> ren_wraps;

  int get_vertex_index(std::vector<GLfloat> vertex_positions,
		       std::vector<GLfloat> uvs,
		       std::vector<GLfloat> colors,
		       glm::vec2 vert, glm::vec2 uv, glm::vec4 color);
  std::vector<geo_info_t> geo_info;
  std::vector<std::string> geo_ids;
  std::vector<bool> geo_is_valid;

  bool ren_precalced;

  GLuint s_progID;
  GLuint s_vertMID, s_vertUVID, s_vert_colID;
  GLuint s_view_matID, s_mod_matID[4], s_uni_colID;

  GLuint elem_trans_gpu_buffer[4];
  GLuint elem_color_gpu_buffer;
       
  int n_ren_states;
  std::vector<GLfloat> colbuf;
  std::vector<GLfloat> transbuf[4];
  std::vector<int> unique_geos;

};


