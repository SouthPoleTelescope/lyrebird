#pragma once
#include <string>
#include <vector>
#include <map>
#include <list>

#include "glm/glm.hpp"
#include <AntTweakBar.h>

#include "polygon.h"
#include "visualelement.h"
#include "sockethelper.h"


/**
   All of the shapes are taken to be in model space not in View Space.  
 **/



class Highlighter{
public:
	Highlighter(TwBar * info_bar, std::vector<VisElemPtr> * vis_elems, size_t num_info_bar_elems);
  
	//code for handling shape geometry
	int get_clicked_elem(glm::vec2 click_point);
	void add_shape_definition(std::string id, std::string svg_path);
	void add_defined_shape(std::string id, glm::mat4 transform, int elem_id, int layer);
	
	void set_AABB();
	void get_AABB(glm::vec2 & min_aabb, glm::vec2 & max_aabb);
	
	
	//code for parsing inputs
	void parse_click(glm::vec2 pos, int mod_key);
	void run_search(const char * search_str, bool no_send = false, bool no_clear = false, bool quick = false);
	void clear_hls();
	void add_hl(int index, bool no_send = false);
	void update_info_bar();
	
	std::list<int> get_plot_inds();
	std::list<glm::vec3> get_plot_colors();
	glm::vec3 get_hl_color(int ind);
	
	void fill_info_bar();
	
	void check_socket();
private:
	//used for shape geometry
	glm::vec2 min_AABB_;
	glm::vec2 max_AABB_;
	std::vector<Polygon> geo_polys_;
	std::vector<int> geo_ids_;
	std::vector<int> geo_layer_;
	
	std::map< std::string, std::vector< Polygon > >  shape_polys_;
	
	std::list<int> hl_inds_;
	std::list<glm::vec3> hl_colors_;
	
	std::vector<uint32_t> atb_colors_;
	std::vector<VisElemPtr> * vis_elems_;
  
	TwBar * info_bar_;
	int info_bar_index;
	int info_bar_is_visible_;
	const size_t num_info_bar_elems_;
	int listen_socket_;
	int send_socket_;
	sockaddr_in send_addr_;
	
};

