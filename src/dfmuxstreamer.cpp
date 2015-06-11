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

	snprintf(name_buffer, 127, "%s/%d/%d:demod_frequency",(*b).c_str(),m,c);
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
  size_t i=0; 
  for (auto b  = unique_boards_.begin(); b!=unique_boards_.end(); b++){
    HkBoardInfo & binfo = b_map[*b];
    for (int m=0; m < NMODULES; m++){
      data_vals->update_val(path_inds_[i], binfo.modules[m].carrier_gain); i++;
      data_vals->update_val(path_inds_[i], binfo.modules[m].nuller_gain); i++;
      for (int c=0; c < NCHANNELS; c++){
	data_vals->update_val(path_inds_[i], binfo.modules[m].channels[c].carrier_amplitude);i++;
	data_vals->update_val(path_inds_[i], binfo.modules[m].channels[c].carrier_frequency);i++;
	data_vals->update_val(path_inds_[i], binfo.modules[m].channels[c].demod_frequency);i++;
      }
    }
  }
}

void HkStreamer::uninitialize(){
  delete hk_module;
}


int get_ip_addr(const char * hostname){
  struct hostent *h;
  h = gethostbyname(hostname);
  if (h == NULL){
    perror("ERROR, no such host");
    return 0;
  }
  int ret_val;
  assert((size_t)h->h_length <= sizeof(int));
  memcpy(&ret_val,h->h_addr,h->h_length);
  return ret_val;
}




DfmuxStreamer::DfmuxStreamer( std::string tag, Json::Value dfmux_desc,DataVals * dv )
  :DataStreamer(tag, dv, 0, DSRT_CALLBACK){
  //sample_hn/mod/channel/iq
  if (dfmux_desc.isMember("hostnames")){
    hostnames_ = std::vector<std::string>(dfmux_desc["hostnames"].size());
    num_boards_ = dfmux_desc["hostnames"].size();
    Json::Value hns = dfmux_desc["hostnames"];
    for (int i=0; i < hns.size(); i++){
      hostnames_[i] = hns[i].asString();
      int ip_addr = get_ip_addr(hostnames_[i].c_str());
      ip_addresses_.push_back(ip_addr);
    }
  }
  char name_buffer[128];
  for (auto b  = hostnames_.begin(); b!=hostnames_.end(); b++){
    for (int m=0; m < NMODULES; m++){
      for (int c=0; c < NCHANNELS; c++){
	snprintf(name_buffer, 127, "%s/%d/%d/I:dfmux_samples",(*b).c_str(),m, c);
	data_val_inds_.push_back(dv->get_ind(std::string(name_buffer)));

	snprintf(name_buffer, 127, "%s/%d/%d/Q:dfmux_samples",(*b).c_str(),m, c);
	data_val_inds_.push_back(dv->get_ind(std::string(name_buffer)));
      }
    }
  }
}

void DfmuxStreamer::Process(G3FramePtr frame, std::deque<G3FramePtr> &out){
  //iterate through channels we are listening to and update their information
  if (frame->type == G3Frame::Timepoint){
    if (frame->Has("DfMux")){
      DfMuxMetaSampleConstPtr samp = frame->Get<DfMuxMetaSample>("DfMux");

      int dv_ind = 0;
      for (auto ip = ip_addresses_.begin(); ip != ip_addresses_.end(); ip++){
	if (samp->find(*ip) == samp->end()) continue;
	const DfMuxBoardSamples & board_sample = samp->at(*ip);
	for (int m = 0; m < NMODULES; m++){
	  DfMuxSampleConstPtr mod_ptr = board_sample.at(m);
	  for (int c =0; c < NCHANNELS; c++){
	    data_vals->update_val(data_val_inds_[dv_ind], mod_ptr->at(c*2)); dv_ind++;
	    data_vals->update_val(data_val_inds_[dv_ind], mod_ptr->at(c*2+1)); dv_ind++;
	  }
	}
      }
      //frame['DfMux'][ipaddress][module_num][channel_num]//
    }
  }
}

void DfmuxStreamer::initialize(){
  //add this to a pipeline downstream of the dfmux buiilder so we can get the delicious samples
  dfmux_builder_ = boost::shared_ptr<DfMuxBuilder>(new  DfMuxBuilder(num_boards_, 10000));
  local_pipeline_.Add(dfmux_builder_);
  local_pipeline_.Add(shared_from_this());
  local_pipeline_.Run();
}

void DfmuxStreamer::uninitialize(){
  dfmux_builder_.reset();
}
