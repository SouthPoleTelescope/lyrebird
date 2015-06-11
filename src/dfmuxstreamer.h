#include "datastreamer.h"
#include <iostream>
#include <string>
#include <memory>
#include <list>

#include "G3Pipeline.h"
#include "G3Module.h"
#include "DfMuxBuilder.h"

#include "hkgetter.h"
#include <boost/enable_shared_from_this.hpp>
#include "json/json.h"

class HkStreamer :public DataStreamer{
public:
  HkStreamer( std::string tag, Json::Value hk_desc, DataVals * dv );
  ~HkStreamer(){}

  void initialize();
  void update_values(int v);
  void uninitialize();

private:
  HousekeepingModule * hk_module;
  std::vector<std::string> unique_boards_;
  std::vector<int> path_inds_;
};


/**
   Instantiates a builder, pipeline, and makes itself the last module in the pipe

 **/


class DfmuxStreamer :public DataStreamer, public G3Module, public boost::enable_shared_from_this<DfmuxStreamer>{
public:
  DfmuxStreamer( std::string tag, 
		 
		 DataVals * dv
		 );
  ~DfmuxStreamer(){}

  //void Process();
  void Process(G3FramePtr frame, std::deque<G3FramePtr> &out);
  
protected:
  void initialize();
  void update_values(int v){}
  void uninitialize();
private:
  std::vector <int> p_board;
  std::vector <int> p_module;
  std::vector <int> p_channel;
  std::vector <int> p_is_i;

  G3Pipeline local_pipeline;
  boost::shared_ptr<DfMuxBuilder> dfmux_builder;
  boost::shared_ptr<DfmuxStreamer> self_pointer;
  
  DfmuxStreamer(const DfmuxStreamer&); //prevent copy construction      
  DfmuxStreamer& operator=(const DfmuxStreamer&); //prevent assignment

};
