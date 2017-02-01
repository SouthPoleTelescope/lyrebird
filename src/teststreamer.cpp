#include "teststreamer.h"
#include <iostream>

using namespace std;

TestStreamer::TestStreamer(Json::Value streamer_json_desc,
	       std::string tag, DataVals * dv, int us_update_time  )
	: DataStreamer(tag, dv, us_update_time, DSRT_STREAMING),
	  streamer_json_desc_(streamer_json_desc)
{
	// lets the datavals object know how many we will be adding
  dv->register_data_source(streamer_json_desc.size());
  val = 0;
}

void TestStreamer::initialize(){std::cout<<"Init test streamer"<<std::endl;
  s_path_inds = std::vector<int>(streamer_json_desc_.size());
  for (unsigned int i=0; i < streamer_json_desc_.size(); i++){
	  //adds our datavals
	  s_path_inds[i] = data_vals->add_data_val(streamer_json_desc_[i].asString(), 0, true, 0);
  }
}
void TestStreamer::uninitialize(){std::cout<<"Uninit test streamer"<<std::endl;}
void TestStreamer::update_values(int ind){
  //printf("told to update\n");
  val += ( (double)sleep_time)/5e5;
  for (unsigned int i=0; i < s_path_inds.size(); i++){
	  data_vals->update_val(s_path_inds[i], val * (i%123+200)/50.0);
  }
}

int TestStreamer::get_num_elements(){
  return 100;
}
