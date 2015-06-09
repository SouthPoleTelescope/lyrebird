#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <unistd.h>
#include <AntTweakBar.h>
#include <map>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>

#include "plotter.h"
#include "plotbundler.h"
#include "cameracontrol.h"
#include "visualelement.h"
#include "genericutils.h"
#include "configparsing.h"
#include "shader.h"
#include "datastreamer.h"
#include "datavals.h"
#include "highlighter.h"
#include "equation.h"
#include "simplerender.h"

#include <list>
#include <memory>


#define SEARCH_STR_LEN 64
#define MIN_INT -10000

using namespace std;


/**
   General Notes:
   -Visual elements are all referenced by the index in a vector, so an integer
   
 **/



CameraControl * global_camera   = NULL;
Highlighter * global_highlighter = NULL;
TwBar * global_info_bar = NULL;
DataVals * global_data_vals = NULL;
bool global_mouse_is_handled = false;
double global_wheel_pos = 0;

void TW_CALL toggleDataValsPause(void * d){
  if (global_data_vals != NULL){
    global_data_vals->toggle_pause();
  }
}

void TW_CALL requestSamplesCallback(void * dsPointer){
  ((DataStreamer*) dsPointer)->update_values(-1);
}

static void error_callback(int error, const char* description){
  fputs(description, stderr);
}


inline void TwEventMouseButtonGLFW3(GLFWwindow* window, int button, int action, int mods){
  if (TwEventMouseButtonGLFW(button, action)) return;
  if (global_highlighter != NULL && global_camera != NULL){
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS ){ 
      
      int modPressed =  ( ( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS) ||
			  ( glfwGetKey(window, GLFW_KEY_LEFT_CONTROL ) == GLFW_PRESS) ||
			  ( glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT ) == GLFW_PRESS) ||
			  ( glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL ) == GLFW_PRESS));
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      glm::vec2 pos, pos2;
      pos.x = xpos;
      pos.y = ypos;
      pos2 = global_camera->con_screen_space_to_model_space(pos);
      global_highlighter->parse_click(pos2, modPressed);
    } else if (button == GLFW_MOUSE_BUTTON_2){
      double xpos, ypos;
      glfwGetCursorPos ( window, &xpos,&ypos);
      if (action == GLFW_PRESS){
	global_camera->register_move_on(xpos, ypos );
      }else if (action == GLFW_RELEASE){
	global_camera->register_move_off(xpos, ypos);
      }
    } 
  }
}

void EventScrollWheel(GLFWwindow * window, double x_offset, double y_offset){
  printf("got scroll %lf %lf\n", x_offset, y_offset);

  global_wheel_pos+=y_offset;
  if (TwMouseWheel(global_wheel_pos)) return;


  double xpos,ypos;
  glfwGetCursorPos ( window, &xpos, &ypos );
  global_camera->do_mouse_zoom( xpos,ypos, y_offset * -.1);
}


inline void TwEventMousePosGLFW3(GLFWwindow* window, double xpos, double ypos){
  if (global_camera && global_camera->is_mouse_moving()){
    global_camera->register_mouse_move(xpos, ypos);
    return;
  }


  if (TwMouseMotion(int(xpos), int(ypos))){
    global_mouse_is_handled = true;
  }else{
    TwMouseButton( TW_MOUSE_PRESSED, TW_MOUSE_LEFT);
    TwMouseButton( TW_MOUSE_RELEASED, TW_MOUSE_LEFT);
    global_mouse_is_handled = false;
  }
}

inline void TwEventMouseWheelGLFW3(GLFWwindow* window, double xoffset, double yoffset){TwEventMouseWheelGLFW(yoffset);}
inline void TwEventKeyGLFW3(GLFWwindow* window, int key, int scancode, int action, int mods){
  if (action == GLFW_REPEAT) action = GLFW_PRESS;
  TwEventKeyGLFW(key, action);
}

inline void TwEventCharGLFW3(GLFWwindow* window, int codepoint){TwEventCharGLFW(codepoint, GLFW_PRESS);}



// Callback function called by GLFW when window size changes                                                                                                                                          
void WindowSizeCB(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0,  width, height);

  if (global_camera != NULL){
    global_camera->set_window_size(width, height);
  }
  TwWindowSize(width, height);
}

int main(int argc, char * args[])
{


  int dv_buffer_size = 512;

  if (argc != 2){
    cout<<"Config file needs to be supplied and only that."<<endl;
    exit(1);
  }

  if ( !file_exists(string(args[1]))){
    cout<<"Config file: "<<args[1]<<" does not exist"<<endl;
    exit(1);
  }
  cout<<"Using config file: "<<args[1]<<endl;
  
  //init config file variables
  vector< vector<string> > ds_paths;
  vector< vector<string> > ds_ids;
  vector< vector<bool> > ds_buffered;
  vector<string>  ds_files;
  vector<string>  ds_types;
  vector<string>  ds_sampling_types;
  vector<string>  ds_tags;
  vector<string>  constant_ids;
  vector<float>  constant_vals;
  int win_x_size;
  int win_y_size;
  int sub_sampling;
  int num_layers;
  int max_framerate;
  int max_num_plotted;
  vector<vis_elem_repr> vis_elems;
  vector<string> svg_paths;
  vector<string> svg_ids;

  vector<string> modifiable_data_val_tags;
  vector<float> modifiable_data_vals;

  vector<equation_desc> glob_eq_descs;

  DCOUT("parsing config files", DEBUG_0);
  //parse the config file
  parse_config_file(args[1],
		    ds_paths, ds_ids, ds_buffered, ds_files, ds_types, ds_sampling_types, ds_tags,
		    modifiable_data_val_tags, modifiable_data_vals,

		    constant_ids, constant_vals,
		    glob_eq_descs,
		    vis_elems, svg_paths, svg_ids,
		    win_x_size, win_y_size, sub_sampling, 
		    num_layers, max_framerate, max_num_plotted
		    );



  //initialize all the data values which are circular buffers we dump floats into
  DCOUT("loading data vals", DEBUG_0);
  int num_data_vals = 0;
  for (int i = 0; i < ds_paths.size(); i++) num_data_vals += ds_paths[i].size();
  num_data_vals += constant_vals.size();
  num_data_vals += modifiable_data_vals.size();

  DataVals data_vals(num_data_vals + 1, dv_buffer_size);
  for (int i = 0; i < ds_ids.size(); i++){
    for (int j = 0; j < ds_ids[i].size(); j++){
      data_vals.add_data_val( ds_ids[i][j], 0.0, ds_buffered[i][j]  );
    }
  }
  for (int i=0; i < constant_ids.size(); i++){
    data_vals.add_data_val( constant_ids[i], constant_vals[i], 0  );    
  }
  for (int i=0; i < modifiable_data_vals.size(); i++){
    data_vals.add_data_val( modifiable_data_val_tags[i], modifiable_data_vals[i], 0  );    
  }




  global_data_vals = &data_vals;

  //create all the data streamers which write to the data vals
  DCOUT("spawning streamers"<<endl, DEBUG_0);
  vector<DataStreamer*> data_streamers;
  for (int i = 0; i < ds_files.size(); i++){
    DataStreamer * ds_tmp = NULL;
    ds_tmp = build_data_streamer( ds_types[i], ds_files[i], ds_paths[i], ds_ids[i], &data_vals,  10000 );
    if (ds_tmp == NULL) print_and_exit(ds_types[i]+"data streamer type not recognized");
    data_streamers.push_back(ds_tmp);
  }

  //spawn the data streamer threads
  for (int i=0; i < data_streamers.size(); i++){
    data_streamers[i]->start_recording();
  }


  //now we configure the window

  DCOUT("initializing glfw", DEBUG_0);
  int width = win_x_size;
  int height = win_y_size;
  GLFWwindow* window;
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  

  window = glfwCreateWindow(width, height, "Lyrebird", NULL, NULL);
  if (!window)
    {
      glfwTerminate();
      exit(EXIT_FAILURE);
    }

  glfwMakeContextCurrent(window);

  

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }
  glEnable(GL_DEPTH_TEST);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //initialize glfwCallbacks  
  glfwSetWindowSizeCallback(window, (GLFWwindowsizefun)WindowSizeCB);
  glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)TwEventMouseButtonGLFW3);
  glfwSetCursorPosCallback(window, (GLFWcursorposfun)TwEventMousePosGLFW3);
  glfwSetScrollCallback(window, TwEventMouseWheelGLFW3);
  glfwSetKeyCallback(window, (GLFWkeyfun)TwEventKeyGLFW3);
  glfwSetCharCallback(window, (GLFWcharfun)TwEventCharGLFW3);
  glfwSetScrollCallback(window, (GLFWscrollfun) EventScrollWheel);


  glClearColor( 0.2,0.2,0.2,1.0);

  DCOUT("creating simple renderer", DEBUG_0);
  //create the renderer
  SimpleRen sren;

  //load our geometry
  for (int i=0; i < svg_paths.size(); i++){
   sren.load_svg_file(svg_ids[i], svg_paths[i]);
  }



  DCOUT("loading visual elements", DEBUG_0);
  std::vector<VisElemPtr> visual_elements;  
  for (int i=0; i<vis_elems.size(); i++){
    visual_elements.emplace_back(new VisElem(&sren,  &data_vals, vis_elems[i]));
  }

  cout<<"done loading geometry"<<endl;

  //////////////////////////////////////////////
  //create the GUI


  DCOUT("initializing ant tweak bar", DEBUG_0);
  TwInit(TW_OPENGL_CORE, NULL);

  TwBar * info_bar = TwNewBar("Info");
  TwDefine("'Info' alpha=220 position='0 5000' size='200 300' visible=false");
  TwWindowSize(width, height);
  
  global_info_bar = info_bar;


  sren.precalc_ren();

  DCOUT("starting loop", DEBUG_0);

  double currentTime = glfwGetTime ();
  double lastTime = currentTime;
  double otherTime;
  
  //load the click geometry
  DCOUT("loading click geometry", DEBUG_0);
  Highlighter highlight(info_bar, &visual_elements);
  for (int i=0; i < svg_paths.size(); i++){
    highlight.add_shape_definition(svg_ids[i], svg_paths[i]);
  }
  cout<<"loading defined geometry"<<endl;

  for (int i=0; i < visual_elements.size(); i++){
    highlight.add_defined_shape( visual_elements[i]->get_geo_id(), 
			      visual_elements[i]->get_ms_transform(), 
			      i,
			      visual_elements[i]->get_layer()
			      );
  }
  global_highlighter = &highlight;
  
  glm::vec2 minAABB, maxAABB;
  highlight.get_AABB(minAABB, maxAABB);

  CameraControl camera(width, height, minAABB.x, maxAABB.x, minAABB.y, maxAABB.y);  
  global_camera = &camera;
  
  Plotter p = Plotter(dv_buffer_size);
  PlotBundler plotBundler( max_num_plotted, dv_buffer_size, &visual_elements);

  //adds the search bar
  TwBar * main_bar = TwNewBar("Main");
  TwDefine(" GLOBAL contained=true ");
  TwDefine("'Main' alpha=220 position='0 0' size='200 300'");


  //make the global equations
  vector<Equation> globEquations = vector<Equation>(glob_eq_descs.size());
  for (int i=0; i < glob_eq_descs.size(); i++){
    globEquations[i].set_equation( &data_vals, glob_eq_descs[i]);
  }
  for (int i=0; i < globEquations.size(); i++){
    TwAddVarRO(main_bar, globEquations[i].get_label().c_str(), TW_TYPE_FLOAT, 
	       globEquations[i].get_value_address(), " group='Global Params' ") ;
  }

  
  int numDSs = data_streamers.size();
  vector<int> ds_index_variables(numDSs, 0);
  vector<int> ds_index_variables_prev_state(numDSs, 0);



  ///Initialize a menu

  char prev_search_str[SEARCH_STR_LEN] = ""; // sizeof(search_str) is 64
  char search_str[SEARCH_STR_LEN] = ""; // sizeof(search_str) is 64
  TwAddVarRW(main_bar, "Search:", TW_TYPE_CSSTRING(sizeof(search_str)), search_str, NULL); // must pass search_str (not &search_str)
  
  bool found_streaming_streamer = false;
  for (int i=0; i < data_streamers.size(); i++){
    if (data_streamers[i]->get_request_type() == DSRT_STREAMING || data_streamers[i]->get_request_type() == DSRT_CALLBACK ){
      found_streaming_streamer = true;
      break;
    }
  }
  if (found_streaming_streamer)
    TwAddButton(main_bar, "Pause", toggleDataValsPause, NULL, NULL);

  TwAddSeparator(main_bar, "ds_sep", NULL);

  for (int i=0; i < data_streamers.size(); i++){
    int dataStreamerReqType = data_streamers[i]->get_request_type();
    if (dataStreamerReqType == DSRT_STREAMING) continue;
    else if (dataStreamerReqType == DSRT_CALLBACK) continue;
    else if (dataStreamerReqType == DSRT_REQUEST){
      TwAddButton(main_bar, data_streamers[i]->get_tag().c_str(), requestSamplesCallback, data_streamers[i], NULL); 
    }else if (dataStreamerReqType == DSRT_REQUEST_HISTORY){
      char def_string[100];
      int max_val = data_streamers[i]->get_num_elements();
      sprintf(def_string, "min=0 max=%d", max_val);
      TwAddVarRW(main_bar, data_streamers[i]->get_tag().c_str(), TW_TYPE_INT32, &(ds_index_variables[i]), def_string);
    }
  }


  TwAddSeparator(main_bar, "modifiable", NULL);

  for (int i=0; i < modifiable_data_vals.size(); i++){
    float * dv_addr = data_vals.get_addr(data_vals.get_ind( modifiable_data_val_tags[i] ));
    TwAddVarRW(main_bar, modifiable_data_val_tags[i].c_str(), TW_TYPE_FLOAT, dv_addr, "");
    //data_vals.add_data_val( modifiable_data_val_tags[i], modifiable_data_vals[i], 0  );    
  }
  //TwDefine("'Search' alpha=220 position='0 0' size='220 50' valueswidth=200 resizable=false movable=false fontresizable=false");
  //make the menu bar
  //data sources
  //
  while (!glfwWindowShouldClose(window)) {
    otherTime = glfwGetTime();
    
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    for (int i = 0; i<numDSs; i++){
      if ( ds_index_variables[i] != ds_index_variables_prev_state[i]){
	data_streamers[i]->request_values(ds_index_variables[i]);
      }
    }
    
    for (int i = 0; i<numDSs; i++){
      ds_index_variables_prev_state[i] = ds_index_variables[i];
    }
    
    for (int i=0; i < visual_elements.size(); i++) visual_elements[i]->update_color();
    sren.draw_ren_states(camera.get_view_mat());
    
    //handles the plotting
    list<int> plot_inds = highlight.get_plot_inds();
    list<glm::vec3> color_inds = highlight.get_plot_colors();
    if (plot_inds.size() > 0){
      plotBundler.update_plots(plot_inds, color_inds);
      int num_plots = plotBundler.get_num_plots();
      
      p.prepare_plotting(glm::vec2(.7, -.7), glm::vec2(.3,.3));
      p.plotBG(glm::vec4(0.0,0.0,0.0,0.9));
      for (int i=num_plots-1; i >= 0; i--){
	//cout<<"plotting regular"<<endl;
	  glm::vec3 plotColor;
	  float minp,maxp;
	  float * plotVals = plotBundler.get_plot(i, plotColor);
	  plotBundler.get_plot_min_max(minp, maxp);
	  p.plot(plotVals, dv_buffer_size, minp, maxp, glm::vec4(plotColor,1), 0);
      }
      p.plotFG(glm::vec4(1.0,1.0,1.0,1.0)); 
      
      
      p.prepare_plotting(glm::vec2(.7, -.1), glm::vec2(.3,.3));
      p.plotBG(glm::vec4(0.0,0.0,0.0,0.9));
      for (int i=num_plots-1; i >= 0; i--){
	glm::vec3 plotColor;
	float minp,maxp;
	float * plotVals = plotBundler.get_psd(i, plotColor);
	plotBundler.get_psd_min_max(minp, maxp);
	p.plot(plotVals, dv_buffer_size/2+1, minp, maxp, glm::vec4(plotColor,1), 1);
      }
      p.plotFG(glm::vec4(1.0,1.0,1.0,1.0)); 
      p.cleanup_plotting();
    }
    
    highlight.update_info_bar();
    
    //updates the main_bar
    for (int i=0; i < globEquations.size(); i++) globEquations[i].get_value();
    
    
    TwRefreshBar(main_bar);
    TwDraw();
    //cout<<"should draw"<<endl;
    //cout<<"other time "<<1.0/(glfwGetTime()-otherTime)<<endl;
    glfwSwapBuffers(window);
    glfwPollEvents();
    
    currentTime = glfwGetTime();
    double deltaTime = currentTime-lastTime;
    lastTime = currentTime;
    
    if (!global_mouse_is_handled){
      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS 
	  ){
	camera.move_up(deltaTime*2);
      }
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS
	  ){
	camera.move_down(deltaTime*2);
      }
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS	    
	  ){
	camera.move_left(deltaTime*2);
      }
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS 
	  ){
	camera.move_right(deltaTime*2);
      }
      if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS){
	camera.zoom(-1*deltaTime);
      }
      if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS){
	camera.zoom(1*deltaTime);
      }
      if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){ //scroll forward in history if we have that type of data streamer
	for (int i=0; i < numDSs; i++){ 
	  if (data_streamers[i]->get_request_type() == DSRT_REQUEST_HISTORY){
	    ds_index_variables[i] += 1;
	    if (ds_index_variables[i]  > data_streamers[i]->get_num_elements()) ds_index_variables[i] = data_streamers[i]->get_num_elements();
	  }
	}
      }
      if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){//scroll back in history if we have that type of data streamer
	for (int i=0; i < numDSs; i++){
	  if (data_streamers[i]->get_request_type() == DSRT_REQUEST_HISTORY){
	    ds_index_variables[i] -= 1;
	    if (ds_index_variables[i]  < 0) ds_index_variables[i] = 0;
	  }
	}
      }
    }
    //handle searching
    if (strcmp( search_str, prev_search_str)){
      highlight.run_search(search_str);
    }    
    strncpy ( prev_search_str, search_str, SEARCH_STR_LEN );      
    //cout<< "FPS: "<<1/deltaTime<<endl;
  }
  
  cout<<"Destorying window"<<endl;
  
  glfwDestroyWindow(window);
  glfwTerminate();
  
  cout<<"tell them to kill themselves"<<endl;
  for (int i=0; i < data_streamers.size(); i++){
    data_streamers[i]->die_gracefully();
  }
  cout<<"burying bodies "<< data_streamers.size() <<endl;
  for (int i=0; i < data_streamers.size(); i++){
    data_streamers[i]->bury_body();
    delete data_streamers[i];    
  }
  
  //clean out the graphics card memory
  sren.clean_out_buffers();
  exit(EXIT_SUCCESS);
}
