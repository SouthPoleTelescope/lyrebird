#include "dfmuxstreamer.h"
#include <unordered_set>

#include <iostream>
#include <string>

#include <list>
#include <vector>
#include <netdb.h>
#include <string.h>
#include <assert.h>

#define NMODULES 8
#define NCHANNELS 64

HkStreamer::HkStreamer( std::string tag, Json::Value hk_desc, DataVals * dv )
  :DataStreamer(tag, dv, 0, DSRT_REQUEST){
  if (hk_desc.isMember("hostnames")){
    unique_boards_ = std::vector<std::string>(hk_desc["hostnames"].size());
    Json::Value hns = hk_desc["hostnames"];
    for (int i=0; i < hns.size(); i++){
      unique_boards_[i] = hns[i].asString();
    }
  }

  // NEEDS TO LINE UP WITH CODE BELOW OR BAD THINGS WILL BEFALL YOU
  char name_buffer[128];
  for (auto b  = unique_boards_.begin(); b!=unique_boards_.end(); b++){
    for (int m=0; m < NMODULES; m++){
      snprintf(name_buffer, 127, "%s/%d:carrier_gain",(*b).c_str(),m);
      path_inds_.push_back(dv->get_ind(std::string(name_buffer)));

      snprintf(name_buffer, 127, "%s/%d:nuller_gain",(*b).c_str(),m);
      path_inds_.push_back(dv->get_ind(std::string(name_buffer)));

      for (int c=0; c < NCHANNELS; c++){
	snprintf(name_buffer, 127, "%s/%d/%d:carrier_amplitude",(*b).c_str(),m,c);
	path_inds_.push_back(dv->get_ind(std::string(name_buffer)));

	snprintf(name_buffer, 127, "%s/%d/%d:carrier_frequency",(*b).c_str(),m,c);
	path_inds_.push_back(dv->get_ind(std::string(name_buffer)));

      }
    }
  }
}

void HkStreamer::initialize(){
  hk_module = new HousekeepingModule(unique_boards_, 30000);
}

void HkStreamer::update_values(int v){
  assert(hk_module);
  std::map<std::string, HkBoardInfo > b_map;
  hk_module->get_housekeeping_structs(b_map);
  size_t i;
  for (auto b  = unique_boards_.begin(); b!=unique_boards_.end(); b++){
    HkBoardInfo & binfo = b_map[*b];
    for (int m=0; m < NMODULES; m++){
      data_vals->update_val(path_inds_[i], binfo.modules[m].carrier_gain); i++;
      data_vals->update_val(path_inds_[i], binfo.modules[m].nuller_gain); i++;
      for (int c=0; c < NCHANNELS; c++){
	data_vals->update_val(path_inds_[i], binfo.modules[m].channels[c].carrier_amplitude);i++;
	data_vals->update_val(path_inds_[i], binfo.modules[m].channels[c].carrier_frequency);i++;
      }
    }
  }
}

void HkStreamer::uninitialize(){
  delete hk_module;
}


void DfmuxStreamer::Process(G3FramePtr frame, std::deque<G3FramePtr> &out){
  //iterate through channels we are listening to and update their information
  /**
  for (int i = 0; i < p_board.size(); i++){
    float val = get_sample_from_dfmux_frame(frame, p_board[i], p_module[i], 
                                            p_channel[i], p_is_i[i]);
    data_vals->update_val(s_path_inds[i], val);
  }
  **/
}

void DfmuxStreamer::initialize(){
  //add this to a pipeline downstream of the dfmux buiilder so we can get the delicious samples
  /**
  dfmux_builder = boost::shared_ptr<DfMuxBuilder>(new  DfMuxBuilder(nboards, 10000));
  local_pipeline.Add(dfmux_builder);
  local_pipeline.Add(shared_from_this());
  local_pipeline.Run();
  **/
}
void DfmuxStreamer::uninitialize(){
  dfmux_builder.reset();
}
