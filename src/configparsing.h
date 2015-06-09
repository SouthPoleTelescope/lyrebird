#pragma once
#include <vector>
#include <string>

#include "visualelement.h"
#include "equation.h"


void parse_config_file(std::string in_file, 

		       std::vector< std::vector<std::string> > & data_source_paths, 
		       std::vector< std::vector<std::string> > & data_source_ids, 
		       std::vector< std::vector<bool> > & data_source_buffered, 
		       std::vector< std::string > & data_source_files,
		       std::vector< std::string > & data_source_types,
		       std::vector< std::string > & data_source_sampling_type,
		       std::vector< std::string > & data_source_tags,

		       std::vector< std::string > & modifiable_data_val_tags,
		       std::vector< float > & modifiable_data_vals,
		       


		       std::vector< std::string > & const_data_ids,
		       std::vector< float > & const_data_vals,

		       std::vector<equation_desc> & global_equation_descs,

		       std::vector<vis_elem_repr> & vis_elems,

		       std::vector<std::string> & svg_paths,
		       std::vector<std::string> & svg_ids,

		       int & win_x_size,
		       int & win_y_size,
		       int & sub_sampling,
		       int & num_layers,
		       int & max_framerate,
		       int & max_num_plotted

		       );
