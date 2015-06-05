#pragma once

#include "datastreamer.h"

#include <hdf5.h>
#include <hdf5_hl.h>

class HdfStreamer :public DataStreamer{
 public:
 HdfStreamer( std::string file, 
	       std::vector< std::string > paths,  
	       std::vector< std::string> ids,
	       DataVals * dv, int us_update_time
	       );
 ~HdfStreamer(){}
 int get_num_elements();

 protected:
 void initialize();
 void update_values(int v);
 void uninitialize();

 private:
 float * val_matrix_;
 int val_length_;
 hid_t file_id_;
};

/*
  std::string s_file;
  std::vector< std::string > s_paths;
  std::vector< int > s_path_inds;
  int sleep_time;
  DataVals * data_vals;
*/
