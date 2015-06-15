#include "cameracontrol.h"
#include <assert.h>
#include <iostream>
using namespace glm;
using namespace std;


CameraControl::CameraControl(int width, int height, 
			     double x_min, double x_max, 
			     double y_min, double y_max):width_(width),height_(height){
  
  aspect_ratio_ = ((float)height)/((float)width);
  center_ = vec2( (x_min+x_max)/2.0, (y_min+y_max)/2.0);

  mouse_moving_ = false;

  float delta_x = x_max-x_min;
  float delta_y = y_max-y_min;

  if (delta_x * aspect_ratio_ > delta_y){
    delta_y = delta_x * aspect_ratio_;
  }else{
    delta_x = delta_y / aspect_ratio_;
  }
  half_span_ = vec2( (delta_x)/2.0, (delta_y)/2.0);
}


glm::mat4 CameraControl::get_view_mat(){
  return  glm::ortho(center_.x-half_span_.x, center_.x+half_span_.x, 
		     center_.y-half_span_.y,  center_.y+half_span_.y, 
		     -15.0f, 15.0f);
    
}

glm::mat4 CameraControl::get_view_mat_inverse(){
  //glm::mat4 trans = get_view_mat();
  return glm::inverse(get_view_mat());
}

void CameraControl::set_center(glm::vec2 center){
  center_ = center;
}

void CameraControl::move_up(float amount){
  center_.y += amount* half_span_.y;
}

void CameraControl::move_down(float amount){
  center_.y -= amount* half_span_.y;
}

void CameraControl::move_left(float amount){
  center_.x -= amount* half_span_.x;
}

void CameraControl::move_right(float amount){
  center_.x += amount * half_span_.x;
}

void CameraControl::zoom(float amount){
  half_span_ *= (1+amount);
}



void CameraControl::set_window_size(int new_width, int new_height){
  width_ = new_width;
  height_ = new_height;

  aspect_ratio_ = ((float)height_)/((float)width_);

  if (half_span_.x * aspect_ratio_ > half_span_.y){
    half_span_.y = half_span_.x * aspect_ratio_;
  }else{
    half_span_.x = half_span_.y / aspect_ratio_;
  }
}

 
glm::vec2 CameraControl::con_screen_space_to_model_space(glm::vec2 in){
  glm::vec2 out;
  glm::mat4 trans = get_view_mat_inverse();
  in.x  = (in.x * 2.0/width_)-1;
  in.y  = ((height_-in.y) * 2.0/height_)-1;
  out[0] = (in.x*trans[0][0] + in.y*trans[1][0]) + trans[3][0];
  out[1] = (in.x*trans[0][1] + in.y*trans[1][1]) + trans[3][1];
  return out;
}


glm::vec2 CameraControl::get_mouse_move_trans( double x_pos, double y_pos){
  glm::vec2 in;
  glm::vec2 out;
  glm::mat4 trans = get_view_mat_inverse();
  in.x  = (x_pos * 2.0/width_)-1;
  in.y  = ((height_-y_pos) * 2.0/height_)-1;
  out[0] = (in.x*trans[0][0] + in.y*trans[1][0]);
  out[1] = (in.x*trans[0][1] + in.y*trans[1][1]);
  return out;
}

void CameraControl::register_move_on(double x_pos, double y_pos){
  mouse_moving_ = true;
  glm::vec2 mouse_in(x_pos,y_pos);
  model_space_center_ = get_mouse_move_trans( x_pos,y_pos);
  original_model_space_center_ = center_;
}

void CameraControl::register_mouse_move(double x_pos, double y_pos){
  assert(mouse_moving_);
  glm::vec2 mouse_in(x_pos,y_pos);
  glm::vec2 new_model_space = get_mouse_move_trans( x_pos,y_pos);
  center_ = original_model_space_center_ - (new_model_space - model_space_center_);
}

void CameraControl::register_move_off(double x_pos, double y_pos){
  mouse_moving_ = false;
}

bool CameraControl::is_mouse_moving(){
  return mouse_moving_;
}

void CameraControl::do_mouse_zoom(double x_pos, double y_pos, double zoom_amount){
  glm::vec2 model_space_mouse_pos = con_screen_space_to_model_space(glm::vec2(x_pos,y_pos));
  half_span_ *= (1+zoom_amount);
  glm::vec2 new_model_space_mouse_pos = con_screen_space_to_model_space(glm::vec2(x_pos,y_pos));
  center_ -= new_model_space_mouse_pos-model_space_mouse_pos;
}
