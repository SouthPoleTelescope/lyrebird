#include "teststreamer.h"
#include <iostream>

using namespace std;


TestStreamer::TestStreamer(Json::Value streamer_json_desc,
	       std::string tag, DataVals * dv, int us_update_time  )
  : DataStreamer(tag, dv, us_update_time, DSRT_STREAMING)
{
  val = 0;
  s_path_inds = std::vector<int>(streamer_json_desc.size());
  for (unsigned int i=0; i < streamer_json_desc.size(); i++){
    s_path_inds[i] = data_vals->get_ind(streamer_json_desc[i].asString());
  }
}

void TestStreamer::initialize(){std::cout<<"Init test streamer"<<std::endl;}
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
