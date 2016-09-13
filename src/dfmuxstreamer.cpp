#include "dfmuxstreamer.h"
#include <unordered_set>
#include <list>
#include <vector>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <deque>

#include <iostream>
#include <string>

#include <G3Frame.h>
#include <G3Pipeline.h>
#include <stdlib.h>
#include <stdio.h>
#include <deque>


#include <core/G3Units.h>

std::string get_physical_id(int board_serial, int crate_serial, int board_slot,
                            int module = 0, int channel = 0) {
	std::stringstream ss;
	if (board_slot <= 0) ss << board_serial;
        else ss << crate_serial << "_" << board_slot;
        if (module > 0) {
                ss << "/" << module;
                if (channel > 0) ss << "/" << channel;
        }
        return ss.str();
}


G3FramePtr get_frame(G3NetworkReceiver & fun){
	std::deque<G3FramePtr> out;
	fun.Process(G3FramePtr(NULL), out);
	return out.front();
}



G3DataStreamer::G3DataStreamer(Json::Value desc,
			       std::string tag, DataVals * dv, int us_update_time )
	: DataStreamer(tag, dv, us_update_time, DSRT_STREAMING),
	  frame_grabbing_function_( desc["network_streamer_hostname"].asString(),
				    desc["network_streamer_port"].asInt())
{
	has_id_map_ = false;
	n_boards_ = desc["board_list"].size();
	board_list_ = std::vector<std::string>(n_boards_);
	for (int i=0; i < n_boards_; i++){
		board_list_[i] = desc["board_list"][i].asString();
	}
	hostname_ = desc["network_streamer_hostname"].asString();
	port_ = desc["network_streamer_port"].asInt();
	streamer_type_ = desc["streamer_type"].asInt();

	log_debug("streamer type %d", streamer_type_);

	if (streamer_type_ == G3DataStreamer::HK) {
		do_hk_ = true;
		do_tp_ = false;
	} else if (streamer_type_ == G3DataStreamer::TP) {
		do_hk_ = false;
		do_tp_ = true;
	} else {
		do_hk_ = true;
		do_tp_ = true;
	}

	dvs_ = dv;

	if (do_tp_) dv->register_data_source(get_num_dfmux_values());
	if (do_hk_) dv->register_data_source(get_num_hk_values());
}

void G3DataStreamer::update_values(int n){
	G3FramePtr frame = get_frame(frame_grabbing_function_);
	if (frame->type == G3Frame::Wiring){
		DfMuxWiringMapConstPtr wm = frame->Get<DfMuxWiringMap>("WiringMap");
		for (auto b = wm->begin(); b != wm->end(); b++){
			const DfMuxChannelMapping & cmap = b->second;
			std::string phys_id = get_physical_id(cmap.board_serial,
							      cmap.crate_serial,
							      cmap.board_slot);
			id_to_serial_map_[phys_id] = cmap.board_serial;
		}
		has_id_map_ = true;
	} else if (frame->type == G3Frame::Timepoint){
		if (! do_tp_) return;
		DfMuxMetaSampleConstPtr ms = frame->Get<DfMuxMetaSample>("DfMux");
		if (has_id_map_) update_dfmux_values(*ms);
	} else if (frame->type == G3Frame::Housekeeping){
		if (! do_hk_) return;
		log_debug("updating hk");
		DfMuxHousekeepingMapConstPtr bi = frame->Get<DfMuxHousekeepingMap>("DfMuxHousekeeping");
		if (has_id_map_) update_hk_values(*bi);
	}
}

void G3DataStreamer::initialize(){
	if (do_hk_) initialize_hk_values();
	if (do_tp_) initialize_dfmux_values();
}

void G3DataStreamer::uninitialize(){
	keep_getting_data_ = false;
}

int G3DataStreamer::get_num_hk_values(){
	return n_boards_ * 1 + n_boards_ * NUM_MODULES * 10 + n_boards_ * NUM_MODULES * NUM_CHANNELS * 11;
}

int G3DataStreamer::get_num_dfmux_values(){
	return n_boards_ * NUM_MODULES * NUM_CHANNELS * 2;
}

//  int add_data_val(std::string id, float val, int is_buffered); 
void G3DataStreamer::initialize_hk_values(){
	char name_buffer[128];
	for (auto b  = board_list_.begin(); b!=board_list_.end(); b++){
		snprintf(name_buffer, 127, "%s:fir_stage",(*b).c_str());
		hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));

		for (int m=1; m < NUM_MODULES+1; m++){
			snprintf(name_buffer, 127, "%s/%d:carrier_gain",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:nuller_gain",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));

			snprintf(name_buffer, 127, "%s/%d:carrier_railed",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:nuller_railed",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:demod_railed",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));


			snprintf(name_buffer, 127, "%s/%d:squid_flux_bias",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:squid_current_bias",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:squid_stage1_offset",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:squid_feedback",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			snprintf(name_buffer, 127, "%s/%d:routing_type",(*b).c_str(),m);
			hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
			
			for (int c=1; c < NUM_CHANNELS+1; c++){
				snprintf(name_buffer, 127, "%s/%d/%d:carrier_amplitude",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				
				snprintf(name_buffer, 127, "%s/%d/%d:carrier_frequency",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				
				snprintf(name_buffer, 127, "%s/%d/%d:demod_frequency",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));


				snprintf(name_buffer, 127, "%s/%d/%d:dan_accumulator_enable",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				snprintf(name_buffer, 127, "%s/%d/%d:dan_feedback_enable",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				snprintf(name_buffer, 127, "%s/%d/%d:dan_streaming_enable",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				snprintf(name_buffer, 127, "%s/%d/%d:dan_gain",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));
				snprintf(name_buffer, 127, "%s/%d/%d:dan_railed",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));

				snprintf(name_buffer, 127, "%s/%d/%d:rnormal",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));

				snprintf(name_buffer, 127, "%s/%d/%d:rlatched",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));

				snprintf(name_buffer, 127, "%s/%d/%d:res_conversion_factor",(*b).c_str(),m,c);
				hk_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, false));

			}
		}
	}
}

void G3DataStreamer::initialize_dfmux_values(){
	char name_buffer[128];
	for (auto b  = board_list_.begin(); b != board_list_.end(); b++){
		for (int m=1; m < NUM_MODULES + 1; m++){
			for (int c=1; c < NUM_CHANNELS + 1; c++){
				snprintf(name_buffer, 127, "%s/%d/%d/I:dfmux_samples",(*b).c_str(),m, c);
				dfmux_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, true));
		
				snprintf(name_buffer, 127, "%s/%d/%d/Q:dfmux_samples",(*b).c_str(),m, c);
				dfmux_path_inds_.push_back(dvs_->add_data_val(std::string(name_buffer), 0, true));
			}
		}
	}	
}


void G3DataStreamer::update_hk_values(const DfMuxHousekeepingMap & b_map){
	//log_debug("update hk");

	size_t i=0; 
	for (auto b  = board_list_.begin(); b != board_list_.end(); b++){
		if (id_to_serial_map_.find(*b) == id_to_serial_map_.end()){
			log_fatal("Specified board to record data on was not in the wiring map");
		}
		int serial = id_to_serial_map_[*b];
		if (b_map.find(serial) == b_map.end()) {
			//log_debug("couldn't find board %s %d", b->c_str(), serial);
			//for (auto it = b_map.begin(); it != b_map.end(); it++)log_debug("SERIALs in hk %d", it->first);
		       

			i += get_num_hk_values()/n_boards_;
			continue;
		}
		const HkBoardInfo & binfo = b_map.at(serial);

		dvs_->update_val(hk_path_inds_[i], binfo.fir_stage); i++;

		for (int m=0; m < NUM_MODULES; m++){
			auto mod_info = binfo.mezz.at(1 + m/4).modules.at(1 + m%4);
			
			dvs_->update_val(hk_path_inds_[i], mod_info.carrier_gain); i++;
			dvs_->update_val(hk_path_inds_[i], mod_info.nuller_gain); i++;

			dvs_->update_val(hk_path_inds_[i], mod_info.carrier_railed ? 1 : 0); i++;
			dvs_->update_val(hk_path_inds_[i], mod_info.nuller_railed ? 1 : 0); i++;
			dvs_->update_val(hk_path_inds_[i], mod_info.demod_railed ? 1 : 0); i++;


			dvs_->update_val(hk_path_inds_[i], mod_info.squid_flux_bias); i++;
			dvs_->update_val(hk_path_inds_[i], mod_info.squid_current_bias); i++;
			dvs_->update_val(hk_path_inds_[i], mod_info.squid_stage1_offset); i++;

			float fb = 0;
			dvs_->update_val(hk_path_inds_[i], fb); i++;
			float routing = 0;
			dvs_->update_val(hk_path_inds_[i], routing); i++;

			for (int c=0; c < NUM_CHANNELS; c++){
				//log_debug("chans");
				auto chan_info = mod_info.channels.at(c+1);
				dvs_->update_val(hk_path_inds_[i], chan_info.carrier_amplitude);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.carrier_frequency / G3Units::Hz);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.demod_frequency / G3Units::Hz);i++;


				dvs_->update_val(hk_path_inds_[i], chan_info.dan_accumulator_enable ?1:0);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.dan_feedback_enable ?1:0);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.dan_streaming_enable ?1:0);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.dan_gain);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.dan_railed ? 1 : 0);i++;

				dvs_->update_val(hk_path_inds_[i], chan_info.rnormal);i++;
				dvs_->update_val(hk_path_inds_[i], chan_info.rlatched);i++;

				dvs_->update_val(hk_path_inds_[i], chan_info.res_conversion_factor);i++;
			}
		}
	}
}

void G3DataStreamer::update_dfmux_values(const DfMuxMetaSample & samp){
	int dv_ind = 0;
	for (auto board = board_list_.begin(); board != board_list_.end(); board++){
		if (id_to_serial_map_.find(*board) == id_to_serial_map_.end()){
			for (auto a = id_to_serial_map_.begin(); a != id_to_serial_map_.end(); a++) std::cout<< a->first << " " << a->second << std::endl;
			log_fatal("Specified board to record data on was not in the wiring map %s", board->c_str());
		}
		int serial = id_to_serial_map_[*board];
		if (samp.find(serial) == samp.end()) {
			//for (auto it = samp.begin(); it != samp.end(); it++)log_debug("SERIALs in dfm %d", it->first);
			
			//log_debug("samp for board %s missing", board->c_str());
			dv_ind += NUM_MODULES * NUM_CHANNELS * 2;
			continue;
		}
		const DfMuxBoardSamples & board_sample = samp.at(serial);
		for (int m = 0; m < NUM_MODULES; m++){
			if (board_sample.find(m) == board_sample.end()){ 
				dv_ind += NUM_CHANNELS * 2;
				continue;
			}
			DfMuxSampleConstPtr mod_ptr = board_sample.at(m);
			for (int c = 0; c < NUM_CHANNELS; c++){
				dvs_->update_val(dfmux_path_inds_[dv_ind], (float) mod_ptr->at(c*2)); dv_ind++;
				dvs_->update_val(dfmux_path_inds_[dv_ind], (float) mod_ptr->at(c*2+1)); dv_ind++;
			}
		}
	}
}










