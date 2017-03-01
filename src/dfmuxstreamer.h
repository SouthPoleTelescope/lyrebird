#include "datastreamer.h"
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>

#include <G3Pipeline.h>
#include <G3Module.h>

#include <core/G3Reader.h>
#include <dfmux/DfMuxBuilder.h>
#include <dfmux/HardwareMap.h>
#include <dfmux/Housekeeping.h>

#include "json/json.h"


#define NUM_MODULES 8
#define NUM_CHANNELS 64


/**
   Things to add:
   
   amp freq freq
   gains

   dan_accumulator_enable
   dan_feedback_enable
   dan_streaming_enable
   dan_gain
   dan_railed

   bool carrier_railed;
   bool nuller_railed;
   bool demod_railed;
   
   double squid_flux_bias;
   double squid_current_bias;
   double squid_stage1_offset;
   
   std::string squid_feedback;
   std::string routing_type;

   int fir_stage;
 **/

// desc needs: ip/port to connect to.
// list of things to report on
class G3DataStreamer : public DataStreamer {
public:
	enum DfmuxType {
		BOTH = 0,
		HK = 1,
		TP = 2,
	};
	//desc needs a list of board ids, ip, port
	//
	G3DataStreamer(Json::Value streamer_json_desc,
		       std::string tag, DataVals * dv, int us_update_time);
	void initialize(); 
	void uninitialize();
	void update_values(int n);

	void initialize_hk_values();
	void initialize_dfmux_values();

	void update_hk_values(const DfMuxHousekeepingMap & board_info,
			      G3MapDoubleConstPtr vbias, G3MapDoubleConstPtr iconv);
	void update_dfmux_values(const DfMuxMetaSample & ms);

	int get_num_hk_values();
	int get_num_dfmux_values();
private:
	std::string reader_str;

	std::map<std::string, int> id_to_serial_map_;
	bool has_id_map_;

	std::vector<std::string>  board_list_;

	std::vector<std::string>  bolo_list_;

	int n_boards_;
	std::string hostname_;
	int port_;
	bool keep_getting_data_;
	
        G3ReaderPtr frame_grabbing_function_;
	DataVals * dvs_;
	
	std::vector<int> hk_path_inds_;
	std::vector<int> dfmux_path_inds_;

	int streamer_type_;
	bool do_hk_;
	bool do_tp_;

	float mean_decay_factor_;

};


