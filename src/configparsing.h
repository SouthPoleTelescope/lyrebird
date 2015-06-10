#pragma once
#include <vector>
#include <string>

#include "datastreamer.h"
#include "visualelement.h"
#include "equation.h"


void parse_config_file(std::string in_file, 

		       std::vector<dataval_desc> & dataval_descs,
		       std::vector<datastreamer_desc> & datastream_descs,
		       std::vector<equation_desc> & equation_descs,

		       std::vector<vis_elem_repr> & vis_elems,

		       std::vector<std::string> & svg_paths,
		       std::vector<std::string> & svg_ids,

		       std::vector<std::string> & displayed_global_equations,
		       std::vector<std::string> & modifiable_data_vals,

		       int & win_x_size,
		       int & win_y_size,
		       int & sub_sampling,
		       int & num_layers,
		       int & max_framerate,
		       int & max_num_plotted

		       );
