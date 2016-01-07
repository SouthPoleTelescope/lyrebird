#pragma once

#include "datastreamer.h"


class TestStreamer :public DataStreamer{
 public:
  TestStreamer(Json::Value streamer_json_desc,
	       std::string tag, DataVals * dv, int us_update_time
	       );
 ~TestStreamer(){}
 int get_num_elements();
 protected:
 void initialize();
 void update_values(int v);
 void uninitialize();
 
 private:
  double val;
  std::vector<int> s_path_inds;

	Json::Value streamer_json_desc_;
};

