#pragma once
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

 private:
  //glm::mat4 view_mat;
  float width_,height_;  
  glm::vec2 center_;
  glm::vec2 half_span_;
  float aspect_ratio_;

};
