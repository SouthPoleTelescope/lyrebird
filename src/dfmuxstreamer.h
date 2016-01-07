#include "datastreamer.h"
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>

#include <G3Pipeline.h>
#include <G3Module.h>

#include <dfmux/DfMuxBuilder.h>
#include <dfmux/HardwareMap.h>
#include <hk/hkgetter.h>

#include <boost/python.hpp>

#include "json/json.h"


#define NUM_MODULES 8
#define NUM_CHANNELS 64

// desc needs: ip/port to connect to.
// list of things to report on
class G3DataStreamer : public DataStreamer {
public:
	//desc needs a list of board ids, ip, port
	//
	G3DataStreamer(Json::Value streamer_json_desc,
		       std::string tag, DataVals * dv, int us_update_time  );
	void initialize(); 
	void uninitialize();
	void update_values(int n);

	void initialize_hk_values();
	void initialize_dfmux_values();

	void update_hk_values(const G3MapBoardInfo & board_info);
	void update_dfmux_values(const DfMuxMetaSample & ms);

	int get_num_hk_values();
	int get_num_dfmux_values();
private:

	std::map<std::string, int> id_to_ip_map_;
	bool has_id_map_;

	std::vector<std::string>  board_list_;
	int n_boards_;
	std::string hostname_;
	int port_;
	bool keep_getting_data_;
	
	boost::python::object frame_grabbing_function_;
	DataVals * dvs_;
	
	std::vector<int> hk_path_inds_;
	std::vector<int> dfmux_path_inds_;
};


