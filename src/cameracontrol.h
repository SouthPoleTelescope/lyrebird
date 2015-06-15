#pragma once
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class CameraControl{
 public: 
  CameraControl(int width, int height, 
		double x_min, double x_max, 
		double y_min, double y_max);

  void move_up(float amount);
  void move_down(float amount);
  void move_left(float amount);
  void move_right(float amount);
  void zoom(float amount);

  void set_window_size(int width, int height);

  void set_center(glm::vec2 cent);
  glm::vec2 con_screen_space_to_model_space(glm::vec2 v);

  glm::mat4 get_view_mat();
  glm::mat4 get_view_mat_inverse();


  void register_move_on(double x_pos, double y_pos);
  void register_mouse_move(double x_pos, double y_pos);
  void register_move_off(double x_pos, double y_pos);
  bool is_mouse_moving();
  glm::vec2 get_mouse_move_trans( double x_pos, double y_pos);
  void do_mouse_zoom(double x_pos, double y_pos, double zoom_amount);


 private:
  //glm::mat4 view_mat;
  float width_,height_;  
  glm::vec2 center_;
  glm::vec2 half_span_;
  float aspect_ratio_;

  bool mouse_moving_;
  glm::vec2 model_space_center_;
  glm::vec2 original_model_space_center_;

};
