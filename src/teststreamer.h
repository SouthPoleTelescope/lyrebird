#pragma once

#include "datastreamer.h"


class TestStreamer :public DataStreamer{
 public:
 TestStreamer( std::string file, 
	       std::vector< std::string > paths,  
	       std::vector< std::string> ids,
	       DataVals * dv, int us_update_time
	       );
 ~TestStreamer(){}
 int get_num_elements();
 protected:
 void initialize();
 void update_values(int v);
 void uninitialize();

 private:
  double val;
 
};

