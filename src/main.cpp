#define GLM_FORCE_RADIANS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <unistd.h>
#include <AntTweakBar.h>
#include <map>
#include <vector>
#include <unordered_set>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
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
#include "logging.h"

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

void TW_CALL toggle_data_vals_pause(void * d){
  int * ps = (int*)d;
  if (global_data_vals != NULL){
    global_data_vals->toggle_pause();
  }

  if (*ps){
    TwDefine("Main/Pause label=Pause");
    *ps = 0;
  }else{
    TwDefine("Main/Pause label=Unpause");
    *ps = 1;
  }
}


void TW_CALL toggle_plot_scale(void * d){
  int * ps = (int*)d;
  if (*ps){
    TwDefine("Main/pscaling label='Switch to Fixed Scale Plot'");
    *ps = 0;
  }else{
    TwDefine("Main/pscaling label='Switch to Auto Scale Plot'");
    *ps = 1;
  }
}



void TW_CALL run_external_command(void * c){
	char * command = (char *)c;
	system(command);
}


void TW_CALL request_samples_callback(void * dsPointer){
  ((DataStreamer*) dsPointer)->request_values(0);
}



struct VisibilityInfo{
  int is_visible;
  std::string name;
  std::vector<VisElemPtr> * visual_elements_ptr;
};

void TW_CALL visiblity_button_callback(void *vis_info){
  VisibilityInfo * vi = (VisibilityInfo*) vis_info;
  if (vi->is_visible){
    std::string label_command = std::string("Main/Vis")+vi->name+std::string(" label='Show ")+vi->name+std::string("'");
    TwDefine(label_command.c_str());
    vi->is_visible = 0;
    for (size_t i=0; i < vi->visual_elements_ptr->size(); i++)
      if ( vi->visual_elements_ptr->at(i)->get_group() == vi->name)
	vi->visual_elements_ptr->at(i)->set_not_drawn();
  } else{
    std::string label_command = std::string("Main/Vis")+vi->name+std::string(" label='Hide ")+vi->name+std::string("'");
    TwDefine(label_command.c_str());
    vi->is_visible = 1;
    for (size_t i=0; i < vi->visual_elements_ptr->size(); i++)
      if ( vi->visual_elements_ptr->at(i)->get_group() == vi->name)
	vi->visual_elements_ptr->at(i)->set_drawn();
  }

  global_highlighter->clear_hls();

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
  GetRootLogger()->SetLogLevel(L3LOG_DEBUG);

  int dv_buffer_size = 512;

  std::string config_file;
  if (argc == 1) {
	  config_file = "lyrebird_config_file.json";
  }else if (argc == 2){
	  config_file = args[1];
  } else if (argc > 2){
    cout<<"Too many arguments to lyrebird"<<endl;
    exit(1);
  }

  if ( !file_exists( config_file )){
    cout<<"Config file: "<< config_file <<" does not exist"<<endl;
    exit(1);
  }
  cout<<"Using config file: "<< config_file <<endl;
  
  //init config file variables
  int win_x_size;
  int win_y_size;
  int sub_sampling;
  int num_layers;
  int max_framerate;
  double frame_time;
  int max_num_plotted;
  vector<vis_elem_repr> vis_elems;
  vector<string> svg_paths;
  vector<string> svg_ids;

  vector<string> displayed_eq_labels;

  std::vector<std::string> displayed_global_equations;
  std::vector<std::string> modifiable_data_vals;
  //std::vector<std::string> displayed_global_equations;

  std::vector<datastreamer_desc> datastream_descs;
  std::vector<dataval_desc> dataval_descs;
  std::vector<equation_desc> eq_descs;

  std::vector<std::string> command_lst;
  std::vector<std::string> command_label;

  //parse the config file
  parse_config_file(config_file.c_str(), dataval_descs, datastream_descs, eq_descs, 
		    vis_elems, svg_paths, svg_ids,
		    displayed_global_equations, modifiable_data_vals,
		    command_lst, command_label,
		    win_x_size, win_y_size, sub_sampling, 
		    num_layers, max_framerate, max_num_plotted,
		    dv_buffer_size,
		    displayed_eq_labels
		    );
  log_debug("done parse_config_file");

  //create the window
  int width = win_x_size;
  int height = win_y_size;
  GLFWwindow* window;
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);

  //initialize all the data values which are circular buffers we dump floats into

  //create all the data streamers which write to the data vals

  DataVals data_vals(dataval_descs.size() + 1, dv_buffer_size);
  vector<std::shared_ptr< DataStreamer> >data_streamers;

  log_debug("creating data_streamers");
  for (size_t i = 0; i < datastream_descs.size(); i++){
	  std::shared_ptr< DataStreamer> ds_tmp  = NULL;
	  ds_tmp = build_data_streamer(datastream_descs[i], &data_vals);
	  if (ds_tmp == NULL) print_and_exit("data streamer type not recognized");
	  data_streamers.push_back(ds_tmp);
  }
  
  data_vals.initialize();

  for (size_t i=0; i < dataval_descs.size(); i++){
	  data_vals.add_data_val(dataval_descs[i].id,
				 dataval_descs[i].init_val, 
				 dataval_descs[i].is_buffered);
  }
  global_data_vals = &data_vals;
  
  //spawn the data streamer threads
  log_debug("spawning streamer threads");
  for (size_t i=0; i < data_streamers.size(); i++){
    data_streamers[i]->start_recording();
  }

  //now we configure the window
  log_debug("setting up glfw");
  glfwWindowHint(GLFW_SAMPLES, sub_sampling);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  if (max_framerate <= 0) max_framerate = 60;
  frame_time = 1.0/max_framerate;

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


  glClearColor( 0.1,0.1,0.1,1.0);

  //create the renderer
  SimpleRen sren;
  log_debug("loading geometry");
  //load our geometry
  for (size_t i=0; i < svg_paths.size(); i++){
   sren.load_svg_file(svg_ids[i], svg_paths[i]);
  }

  log_debug("adding equations");  
  EquationMap equation_map(eq_descs.size()+1, &data_vals);
  for (size_t i=0; i < eq_descs.size(); i++){
    equation_map.add_equation(eq_descs[i]);
  }
  

  log_debug("adding visual elements");  
  std::vector<VisElemPtr> visual_elements;  
  for (size_t i=0; i<vis_elems.size(); i++){
    visual_elements.emplace_back(new VisElem(&sren,  &equation_map, vis_elems[i]));
  }
  sren.precalc_ren();

 

  //////////////////////////////////////////////
  //create the GUI

  log_debug("starting ant tweak bar thingy");  
  
  TwInit(TW_OPENGL_CORE, NULL);

  TwBar * info_bar = TwNewBar("Info");
  TwDefine("'Info' alpha=220 position='0 5000' size='200 300' visible=false");
  TwWindowSize(width, height);
  
  global_info_bar = info_bar;


  double current_time = glfwGetTime ();
  double last_time = current_time;
  
  //load the click geometry

  log_debug("setting up highlighter");  
  Highlighter highlight(info_bar, &visual_elements, max_num_plotted);
  for (size_t i=0; i < svg_paths.size(); i++){
    highlight.add_shape_definition(svg_ids[i], svg_paths[i]);
  }

  for (size_t i=0; i < visual_elements.size(); i++){
    highlight.add_defined_shape( visual_elements[i]->get_geo_id(), 
			      visual_elements[i]->get_ms_transform(), 
			      i,
			      visual_elements[i]->get_layer()
			      );
  }
  global_highlighter = &highlight;
  
  glm::vec2 minAABB, maxAABB;
  highlight.get_AABB(minAABB, maxAABB);


  float x_slop = 0.3 * ( maxAABB.x - minAABB.x);
  float y_slop = 0.3 * ( maxAABB.y - minAABB.y);
  log_debug("setting up camera");  
  CameraControl camera(width, height, 
		       minAABB.x - x_slop, maxAABB.x + x_slop, 
		       minAABB.y - y_slop, maxAABB.y + y_slop);  
  global_camera = &camera;
  
  log_debug("setting up plotter");  
  Plotter p = Plotter(dv_buffer_size);
  PlotBundler plotBundler( max_num_plotted, dv_buffer_size, &visual_elements);

  //adds the search bar


  log_debug("more ant tweak bar thing");
  TwBar * main_bar = TwNewBar("Main");
  TwDefine(" GLOBAL contained=true ");
  TwDefine("'Main' alpha=220 position='0 0' size='200 300'");

  //make the global equations
  log_debug("global eqs");
  for (size_t i=0; i < displayed_global_equations.size(); i++){
    Equation & eq = equation_map.get_eq(equation_map.get_eq_index( displayed_global_equations[i] ));
    TwAddVarRO(main_bar, eq.get_label().c_str(),
	       TW_TYPE_FLOAT, eq.get_value_address(), " group='Global Params' ") ;
  }
  
  int num_data_sources = data_streamers.size();
  vector<int> ds_index_variables(num_data_sources, 0);
  vector<int> ds_index_variables_prev_state(num_data_sources, 0);


  ///Initialize a menu
  log_debug("search");
  char prev_search_str[SEARCH_STR_LEN] = ""; // sizeof(search_str) is 64
  char search_str[SEARCH_STR_LEN] = ""; // sizeof(search_str) is 64
  TwAddVarRW(main_bar, "Search:", TW_TYPE_CSSTRING(sizeof(search_str)), search_str, NULL); // must pass search_str (not &search_str)
  

  int is_paused = 0;  
  int is_fixed_scaled = 0;

  float min_plot_val = 0;
  float max_plot_val = 1;

  TwAddSeparator(main_bar, "ds_sep", NULL);
  TwAddButton(main_bar, "Pause", toggle_data_vals_pause, &is_paused, NULL);
  TwAddButton(main_bar, "pscaling", toggle_plot_scale, &is_fixed_scaled, "label='Switch to Fixed Scale Plot'");

  TwAddVarRW(main_bar, "Min Plot Val", TW_TYPE_FLOAT, &min_plot_val, "group='Fixed Plot Bounds' step=0.01");
  TwAddVarRW(main_bar, "Max Plot Val", TW_TYPE_FLOAT, &max_plot_val, "group='Fixed Plot Bounds' step=0.01");

  TwAddSeparator(main_bar, "pasue_sep", NULL);

  log_debug("displayed eqs");

  unsigned int displayed_eq = 0;
  unsigned int prev_eq_val = 0;
  int max_val;
  size_t display_buffer_size = 0;
  char * displayed_name;
  if (displayed_eq_labels.size() > 0){
	  max_val = displayed_eq_labels.size() -1;
	  for (size_t i=0; i < displayed_eq_labels.size(); i++) {
		  size_t s = displayed_eq_labels[i].size() + 1;
		  if (s > display_buffer_size) {
			  display_buffer_size = s;
		  }
	  }
	  displayed_name = new char[display_buffer_size];
	  strncpy(displayed_name, displayed_eq_labels[0].c_str(), display_buffer_size); 
	  
	  TwAddVarRW(main_bar, "Displayed Equation",
		     TW_TYPE_UINT32, &displayed_eq, " min=0") ;
	  TwSetParam(main_bar, "Displayed Equation", "max", TW_PARAM_INT32, 1, &max_val);
	  
	  TwAddVarRW(main_bar, "Eq Label:", TW_TYPE_CSSTRING(display_buffer_size),
		     displayed_name, NULL);
	  
	  TwAddSeparator(main_bar, "label_sep", NULL);
  
  }


  log_debug("external commands");
  for (size_t i=0; i < command_lst.size(); i++){
	  TwAddButton(main_bar, command_label[i].c_str(), 
		      run_external_command, const_cast<char*>( command_lst[i].c_str()),
		      NULL); 
  }


  TwAddSeparator(main_bar, "command_sep", NULL);

  //char test_command[] = "echo hello";
  //TwAddButton(main_bar, "Run", run_external_command, test_command, NULL);


  log_debug("streamer buttons");
  for (size_t i=0; i < data_streamers.size(); i++){
    int dataStreamerReqType = data_streamers[i]->get_request_type();
    if (dataStreamerReqType == DSRT_STREAMING) continue;
    else if (dataStreamerReqType == DSRT_CALLBACK) continue;
    else if (dataStreamerReqType == DSRT_REQUEST){
      TwAddButton(main_bar, data_streamers[i]->get_tag().c_str(), 
		  request_samples_callback, &(*(data_streamers[i])), NULL); 
    }else if (dataStreamerReqType == DSRT_REQUEST_HISTORY){
      char def_string[100];
      int max_val = data_streamers[i]->get_num_elements();
      sprintf(def_string, "min=0 max=%d", max_val);
      TwAddVarRW(main_bar, data_streamers[i]->get_tag().c_str(), TW_TYPE_INT32, &(ds_index_variables[i]), def_string);
    }
  }

  TwAddSeparator(main_bar, "modifiable", NULL);
  for (size_t i=0; i < modifiable_data_vals.size(); i++){
    float * dv_addr = data_vals.get_addr(data_vals.get_ind( modifiable_data_vals[i] ));
    TwAddVarRW(main_bar, modifiable_data_vals[i].c_str(), TW_TYPE_FLOAT, dv_addr, "");
  }


  TwAddSeparator(main_bar, "vis_sep", NULL);
  /// Code for setting things visible
  std::unordered_set<std::string> visual_element_groups;
  for (size_t i=0; i < visual_elements.size(); i++){
    std::string vgroup = visual_elements[i]->get_group();
    visual_element_groups.insert(vgroup);
  }  
  std::vector<VisibilityInfo> vis_info;  
  for (auto it=visual_element_groups.begin(); it!= visual_element_groups.end(); it++){
    VisibilityInfo vi;
    vi.is_visible = 1;
    vi.name = *it;
    vi.visual_elements_ptr = & visual_elements;
    vis_info.push_back(vi);
  }
  int visibility_index=0;
  for (auto it=visual_element_groups.begin(); it!= visual_element_groups.end(); it++){
    TwAddButton(main_bar, (std::string("Vis") + (*it)).c_str(), 
		visiblity_button_callback, 
		&(vis_info[visibility_index]), (std::string("label='Hide ") + (*it) + std::string("'")).c_str());
    visibility_index++;
  }

  log_debug("starting loop");
  //actual loop//
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    usleep(10);

    //update the equations if possible
    if (prev_eq_val != displayed_eq) {
	    prev_eq_val = displayed_eq;
	    for (size_t i=0; i < visual_elements.size(); i++){
		    visual_elements[i]->set_eq_ind( prev_eq_val );
	    }
	    strncpy(displayed_name, 
		    displayed_eq_labels[prev_eq_val].c_str(), 
		    display_buffer_size); 
    }

    //other things
    
    highlight.check_socket();
    for (int i = 0; i<num_data_sources; i++){
      if ( ds_index_variables[i] != ds_index_variables_prev_state[i]){
	data_streamers[i]->request_values(ds_index_variables[i]);
      }
    }
    
    for (int i = 0; i<num_data_sources; i++){
      ds_index_variables_prev_state[i] = ds_index_variables[i];
    }
    
    for (size_t i=0; i < visual_elements.size(); i++) visual_elements[i]->update_color();
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
	  glm::vec3 plotColor;
	  float minp,maxp;
	  float * plotVals = plotBundler.get_plot(i, plotColor);
	  plotBundler.get_plot_min_max(minp, maxp);

	  if (is_fixed_scaled) {
		  minp = min_plot_val; 
		  maxp = max_plot_val;
	  }

	  p.plot(plotVals, dv_buffer_size, minp, maxp, glm::vec4(plotColor,1), 0,
		 1,1 );
      }
      p.plotFG(glm::vec4(1.0,1.0,1.0,1.0)); 
      
      p.prepare_plotting(glm::vec2(.7, -.1), glm::vec2(.3,.3));
      p.plotBG(glm::vec4(0.0,0.0,0.0,0.9));
      
      float psd_0point;
      float psd_sep;
      plotBundler.get_psd_start_and_sep(psd_0point, psd_sep);
      for (int i=num_plots-1; i >= 0; i--){
	glm::vec3 plotColor;
	float minp,maxp;
	float * plotVals = plotBundler.get_psd(i, plotColor);
	plotBundler.get_psd_min_max(minp, maxp);
	p.plot(plotVals, dv_buffer_size/2+1, minp, maxp, glm::vec4(plotColor,1), 1,
	       psd_sep, psd_sep);
      }
      //p.plotFG(glm::vec4(1.0,1.0,1.0,1.0)); 
      p.cleanup_plotting();
    }

    //updates the info bar
    highlight.update_info_bar();
    
    //updates the main_bar
    for (size_t i=0; i < displayed_global_equations.size(); i++) {
	    equation_map.get_eq(equation_map.get_eq_index( displayed_global_equations[i])).get_value();
    }

    
    
    TwRefreshBar(main_bar);
    TwDraw();

    glfwSwapBuffers(window);
    glfwPollEvents();
    
    current_time = glfwGetTime();
    double delta_time = current_time-last_time;


    // code for doing frame limitting
    if (delta_time < frame_time)
      usleep( (frame_time - delta_time) * 1e6);
    current_time = glfwGetTime();
    delta_time = current_time-last_time;

    last_time = current_time;
    
    if (!global_mouse_is_handled){
      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS 
	  ){
	camera.move_up(delta_time*2);
      }
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS
	  ){
	camera.move_down(delta_time*2);
      }
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS	    
	  ){
	camera.move_left(delta_time*2);
      }
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
	  glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS 
	  ){
	camera.move_right(delta_time*2);
      }
      if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS){
	camera.zoom(-1*delta_time);
      }
      if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS){
	camera.zoom(1*delta_time);
      }
      if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){ //scroll forward in history if we have that type of data streamer
	for (int i=0; i < num_data_sources; i++){ 
	  if (data_streamers[i]->get_request_type() == DSRT_REQUEST_HISTORY){
	    ds_index_variables[i] += 1;
	    if (ds_index_variables[i]  > data_streamers[i]->get_num_elements()) ds_index_variables[i] = data_streamers[i]->get_num_elements();
	  }
	}
      }
      if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){//scroll back in history if we have that type of data streamer
	for (int i=0; i < num_data_sources; i++){
	  if (data_streamers[i]->get_request_type() == DSRT_REQUEST_HISTORY){
	    ds_index_variables[i] -= 1;
	    if (ds_index_variables[i]  < 0) ds_index_variables[i] = 0;
	  }
	}
      }
    }

    //animates the highlighting
    for (size_t i=0; i < visual_elements.size(); i++)
	    visual_elements[i]->animate_highlight(delta_time);
    


    //handle searching
    if (strcmp( search_str, prev_search_str)){
      highlight.run_search(search_str);
    } 

    strncpy ( prev_search_str, search_str, SEARCH_STR_LEN );      
  }
  
  
  glfwDestroyWindow(window);
  glfwTerminate();
  

  log_debug("Telling data_streamers to die_gracefully");
  for (size_t i=0; i < data_streamers.size(); i++){
    data_streamers[i]->die_gracefully();
  }
  log_debug("Telling data_streamers to bury_body");
  for (size_t i=0; i < data_streamers.size(); i++){
    data_streamers[i]->bury_body();
  }
  if (displayed_eq_labels.size() > 0) {
	  delete [] displayed_name;
  }  
  log_debug("Cleaning graphics card memory");
  sren.clean_out_buffers();
  exit(EXIT_SUCCESS);
}
