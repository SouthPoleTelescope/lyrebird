#include "dfmuxstreamer.h"
#include <unordered_set>

#include <iostream>
#include <string>

#include <list>
#include <vector>
#include <netdb.h>
#include <string.h>

template <class T>
T split_string(std::string s, std::string delim){
  //T<std::string> out_list;
  T out_list;
  size_t last = 0; 
  size_t next = 0; 
  while ((next = s.find(delim, last)) != std::string::npos) { 
    out_list.push_back(s.substr(last, next-last)); 
    last = next + 1; 
  } 
  out_list.push_back(s.substr(last)); 
  return out_list;
}

void parse_channel_path(std::string path, 
			std::string & hostname_out,
			int & module_out,
			int & channel_out,
			int & is_i){
  std::vector<std::string> ss = split_string<std::vector< std::string > >(path, "/");

  module_out = -1;
  channel_out = -1;
  
  hostname_out = ss[0];
  if (ss.size() > 1)
    module_out = atoi(ss[1].c_str());
  if (ss.size() > 2)
    channel_out = atoi(ss[2].c_str());
  if (ss.size() > 3)
    channel_out = atoi(ss[2].c_str());
}

void parse_hk_streamer_path(std::string path, 
			    char & var_type_out,
			    std::string & var_name_out,
			    std::string & hostname_out,
			    int & module_out,
			    int & channel_out){
  std::vector<std::string> ss = split_string<std::vector < std::string > >(path, ":");
  int dummy;
  parse_channel_path(ss[0], hostname_out, module_out,channel_out, dummy);
  var_type_out = ss[1][0];
  var_name_out = ss[2];
}


int get_ip_addr(const char * hostname){
  struct hostent *h;
  h = gethostbyname(hostname);
  if (h == NULL){
    perror("ERROR, no such host");
    return 0;
  }
  int ret_val;
  assert(h->h_length <= sizeof(int));
  memcpy(&ret_val,h->h_addr,h->h_length);
  return ret_val;
}


HkStreamer::HkStreamer( std::string file, 
			std::vector< std::string > paths,  
			std::vector< std::string> ids,
			DataVals * dv, int us_update_time)
  :DataStreamer(file, paths, ids, dv, us_update_time, DSRT_REQUEST){}

void HkStreamer::initialize(){
  p_ip_addrs = std::vector<int>( s_paths.size());
  p_module = std::vector<int>( s_paths.size());
  p_channel = std::vector<int>( s_paths.size());
  p_var_name = std::vector<std::string>( s_paths.size());
  p_var_type = std::vector<char>( s_paths.size());

  std::unordered_set<std::string> hns;

  for (int i=0; i < s_paths.size(); i++){
    std::string hostname;
    parse_hk_streamer_path( s_paths[i], 
			    p_var_type[i],
			    p_var_name[i],
			    hostname,
			    p_module[i],
			    p_channel[i]);
    hns.insert( hostname );
    p_ip_addrs[i] = get_ip_addr(hostname.c_str());
  }  
  std::vector<std::string> tv( hns.begin(), hns.end());
  hk_module = new HousekeepingModule(tv, 30000);
}


void HkStreamer::update_values(int v){
  std::map< int, HkBoardInfo > b_map;
  hk_module->get_housekeeping_structs(b_map);
  //actually get values
  for (int i=0; i < p_ip_addrs.size(); i++){
    float val = b_map[p_ip_addrs[i]].get_member_value(p_var_name[i], p_var_type[i], 
						      p_module[i], p_channel[i]);
    data_vals->update_val(s_path_inds[i], val );
  }
}
void HkStreamer::uninitialize(){
  delete hk_module;
}



DfmuxStreamer::DfmuxStreamer( std::string file, 
			      std::vector< std::string > paths,  
			      std::vector< std::string> ids,
			      DataVals * dv, int us_update_time
			      ) : DataStreamer(file, paths, ids, dv, us_update_time, DSRT_CALLBACK){
}



float get_sample_from_dfmux_frame(G3FramePtr frame, 
				  int board, int module, int channel, int is_i){
  return 1.0f;
}


void DfmuxStreamer::Process(G3FramePtr frame, std::deque<G3FramePtr> &out){
  //iterate through channels we are listening to and update their information
  for (int i = 0; i < p_board.size(); i++){
    float val = get_sample_from_dfmux_frame(frame, 
					    p_board[i], p_module[i], 
					    p_channel[i], p_is_i[i]);
    data_vals->update_val(s_path_inds[i], val);
  }
}

void DfmuxStreamer::initialize(){
  int nboards = 0;
  p_board = std::vector<int>( s_paths.size());
  p_module = std::vector<int>( s_paths.size());
  p_channel = std::vector<int>( s_paths.size());
  p_is_i = std::vector<int>( s_paths.size());

  std::unordered_set<std::string> hns;

  for (int i=0; i < s_paths.size(); i++){
    std::string hostname;
    parse_channel_path( s_paths[i], 
			hostname,
			p_module[i],
			p_channel[i], 
			p_is_i[i]);
    hns.insert( hostname );
    p_board[i] = get_ip_addr(hostname.c_str());
  }

  //add this to a pipeline downstream of the dfmux buiilder so we can get the delicious samples
  dfmux_builder = boost::shared_ptr<DfMuxBuilder>(new  DfMuxBuilder(nboards, 10000));
  local_pipeline.Add(dfmux_builder);
  local_pipeline.Add(shared_from_this());
  local_pipeline.Run();


}
void DfmuxStreamer::uninitialize(){
  dfmux_builder.reset();
}



