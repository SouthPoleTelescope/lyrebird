#include "teststreamer.h"
#include <iostream>

using namespace std;


TestStreamer::TestStreamer(
			   std::string file, 
			   std::vector< std::string > paths,  
			   std::vector< std::string> ids,
			   DataVals * dv, int us_update_time
			   ):DataStreamer(file, paths, ids, dv, us_update_time, DSRT_STREAMING){
  val = 0;
}

void TestStreamer::initialize(){std::cout<<"init test"<<std::endl;}
void TestStreamer::uninitialize(){std::cout<<"uninit test"<<std::endl;}

void TestStreamer::update_values(int ind){
  val += ( (double)sleep_time)/1e6;
  for (int i=0; i < s_path_inds.size(); i++){
    data_vals->update_val(s_path_inds[i], val * (i%123+200)/50.0);
  }
}

int TestStreamer::get_num_elements(){
  return 100;
}
