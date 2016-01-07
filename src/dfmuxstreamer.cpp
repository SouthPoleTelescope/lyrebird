#include "dfmuxstreamer.h"
#include <unordered_set>
#include <list>
#include <vector>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <deque>

#include <hk/dfmuxcalculations.h>

#include <iostream>
#include <string>

#include <G3Frame.h>
#include <G3Pipeline.h>
#include <stdlib.h>
#include <stdio.h>

using namespace boost::python;

object setup_network_streamer(std::string hostname, int port)
{ 
	Py_Initialize();
	// Retrieve the main module.
	object main = import("__main__");
	// Retrieve the main module's namespace
	object global(main.attr("__dict__"));

	char * command = (char *)malloc( sizeof(char) * 4096);
	sprintf(command, 
		"from spt3g import core, dfmux, hk, networkstreamer\n"
		"my_network_listener = networkstreamer.NetworkReceiver(\"%s\", %d)\n"
		"def get_data():\n"
		"    global my_network_listener\n"
		"    return my_network_listener(None)\n",
		hostname.c_str(), port);
	// Define greet function in Python.
	object result = exec( command, global, global);
	object fun = global["get_data"];
	return fun;
}

G3FramePtr get_frame(object fun){
	return extract<G3FramePtr>(fun());
}

void cleanup_network_streamer(){
	Py_Finalize();	
}


G3DataStreamer::G3DataStreamer(Json::Value desc,
			       std::string tag, DataVals * dv, int us_update_time )
	: DataStreamer(tag, dv, us_update_time, DSRT_STREAMING)
{
	has_id_map_ = false;
	n_boards_ = desc["board_list"].size();
	board_list_ = std::vector<std::string>(n_boards_);
	for (int i=0; i < n_boards_; i++){
		board_list_[i] = desc["board_list"][i].asString();
	}
	hostname_ = desc["network_streamer_hostname"].asString();
	port_ = desc["network_streamer_port"].asInt();

	dvs_ = dv;
	dv->register_data_source(get_num_dfmux_values());
	dv->register_data_source(get_num_hk_values());

}

void G3DataStreamer::update_values(int n){
	G3FramePtr frame = boost::python::extract<G3FramePtr>(frame_grabbing_function_());
	if (frame->type == G3Frame::Wiring){
		DfMuxWiringMapConstPtr wm = frame->Get<DfMuxWiringMap>("WiringMap");
		for (auto b = wm->begin(); b != wm->end(); b++){
			const DfMuxChannelMapping & cmap = b->second;
			std::string phys_id = get_physical_id(cmap.crate_serial,
							      cmap.board_serial,
							      cmap.board_slot);
			id_to_ip_map_[phys_id] = cmap.board_ip;
		}
		has_id_map_ = true;
	} else if (frame->type == G3Frame::Timepoint){
		DfMuxMetaSampleConstPtr ms = frame->Get<DfMuxMetaSample>("DfMux");
		if (has_id_map_) update_dfmux_values(*ms);
	} else if (frame->type == G3Frame::Housekeeping){
		G3MapBoardInfoConstPtr bi = frame->Get<G3MapBoardInfo>("HkBoardInfo");
		if (has_id_map_) update_hk_values(*bi);
	}
}

void G3DataStreamer::initialize(){
	initialize_hk_values();
	initialize_dfmux_values();
	frame_grabbing_function_ = setup_network_streamer(hostname_, port_);
}

void G3DataStreamer::uninitialize(){
	keep_getting_data_ = false;
	cleanup_network_streamer();
}

int G3DataStreamer::get_num_hk_values(){
	return n_boards_ * NUM_MODULES * 2 + n_boards_ * NUM_MODULES * NUM_CHANNELS * 3;
}

int G3DataStreamer::get_num_dfmux_values(){
	return n_boards_ * NUM_MODULES * NUM_CHANNELS * 2;
}

//  int add_data_val(std::string id, float val, int is_buffered); 
void G3DataStreamer::initialize_hk_values(){
	char name_buffer[128];
	for (auto b  = board_list_.begin(); b!=board_list_.end(); b++){
		for (int m=0; m < NUM_MODULES; m++){
			snprintf(name_buffer, 127, "%s/%d:carrier_gain",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			
			snprintf(name_buffer, 127, "%s/%d:nuller_gain",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			
			for (int c=0; c < NUM_CHANNELS; c++){
				snprintf(name_buffer, 127, "%s/%d/%d:carrier_amplitude",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				
				snprintf(name_buffer, 127, "%s/%d/%d:carrier_frequency",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				
				snprintf(name_buffer, 127, "%s/%d/%d:demod_frequency",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			}
		}
	}
}

void G3DataStreamer::initialize_dfmux_values(){
	char name_buffer[128];
	for (auto b  = board_list_.begin(); b != board_list_.end(); b++){
		for (int m=0; m < NUM_MODULES; m++){
			for (int c=0; c < NUM_CHANNELS; c++){
				snprintf(name_buffer, 127, "%s/%d/%d/I:dfmux_samples",(*b).c_str(),m, c);
				dfmux_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, true));
		
				snprintf(name_buffer, 127, "%s/%d/%d/Q:dfmux_samples",(*b).c_str(),m, c);
				dfmux_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, true));
			}
		}
	}	
}


void G3DataStreamer::update_hk_values(const G3MapBoardInfo & b_map){
	size_t i=0; 
	for (auto b  = board_list_.begin(); b != board_list_.end(); b++){
		if (id_to_ip_map_.find(*b) == id_to_ip_map_.end()){
			log_fatal("Specified board to record data on was not in the wiring map");
		}
		int ip = id_to_ip_map_[*b];
		if (b_map.find(ip) == b_map.end()) {
			i += NUM_MODULES * 2 + NUM_MODULES * NUM_CHANNELS * 3;
		}
		const HkBoardInfo & binfo = b_map.at(ip);
		for (int m=0; m < NUM_MODULES; m++){
			dvs_->update_val(hk_path_inds_[i], binfo.modules[m].carrier_gain); i++;
			dvs_->update_val(hk_path_inds_[i], binfo.modules[m].nuller_gain); i++;
			for (int c=0; c < NUM_CHANNELS; c++){
				dvs_->update_val(hk_path_inds_[i], binfo.modules[m].channels[c].carrier_amplitude);i++;
				dvs_->update_val(hk_path_inds_[i], binfo.modules[m].channels[c].carrier_frequency);i++;
				dvs_->update_val(hk_path_inds_[i], binfo.modules[m].channels[c].demod_frequency);i++;
			}
		}
	}
	
}

void G3DataStreamer::update_dfmux_values(const DfMuxMetaSample & samp){
	int dv_ind = 0;
	for (auto board = board_list_.begin(); board != board_list_.end(); board++){
		if (id_to_ip_map_.find(*board) == id_to_ip_map_.end()){
			for (auto a = id_to_ip_map_.begin(); a != id_to_ip_map_.end(); a++) std::cout<< a->first << " " << a->second << std::endl;
			log_fatal("Specified board to record data on was not in the wiring map");
		}
		int ip = id_to_ip_map_[*board];
		if (samp.find(ip) == samp.end()) {
			dv_ind += NUM_MODULES * NUM_CHANNELS * 2;
			continue;
		}
		const DfMuxBoardSamples & board_sample = samp.at(ip);
		for (int m = 0; m < NUM_MODULES; m++){
			if (board_sample.find(m) == board_sample.end()){ 
				dv_ind += NUM_CHANNELS * 2;
				continue;
			}
			DfMuxSampleConstPtr mod_ptr = board_sample.at(m);
			for (int c =0; c < NUM_CHANNELS; c++){
				dvs_->update_val(dfmux_path_inds_[dv_ind], (float) mod_ptr->at(c*2)); dv_ind++;
				dvs_->update_val(dfmux_path_inds_[dv_ind], (float) mod_ptr->at(c*2+1)); dv_ind++;
			}
		}
	}
}










