#pragma once
#include <pthread.h>
#include <assert.h>
#include <vector>
#include <string>

#include "json/json.h" 

#include "datavals.h"
#include "genericutils.h"

#define DSRT_STREAMING 0
#define DSRT_REQUEST 1
#define DSRT_REQUEST_HISTORY 2
#define DSRT_CALLBACK 3



//request
//timed request
//history request
//callback based


/**

Types of data streamers:  

-sleep request sample
-block until main thread requests a sample
-purely callback

 **/

struct datastreamer_desc{
  std::string tag;
  std::string tp;
  Json::Value streamer_json_desc;
  int us_update_time;
};


class DataStreamer{
 public:
  DataStreamer(std::string tag, 
	       DataVals * dv, int us_update_time,
	       int data_source_request_type
	       );

  virtual ~DataStreamer(){}

  void start_recording();
  void die_gracefully();
  void bury_body();
  void thread_loop_auto();
  void thread_loop_request();
  void thread_loop_callback();

  void request_values(int ind);
  int get_request_type();
  std::string get_tag();

  //ind is the index if it is a REQUEST_HISTORY type
  virtual void update_values(int ind){std::cout<<"update DS"<<std::endl;}
  virtual int get_num_elements(){assert(0);return -1;}
 protected:
  virtual void initialize(){std::cout<<"init DS"<<std::endl;}
  virtual void uninitialize(){std::cout<<"uninit DS"<<std::endl;}
  //information for child
  std::string s_tag;
  int sleep_time;
  DataVals * data_vals;
 private:
  pthread_t d_thread;
  
  pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t count_threshold_cv = PTHREAD_COND_INITIALIZER;
  static pthread_mutex_t init_uninit_mutex_;

  //std::thread d_thread;
  bool should_live;
  int ds_req_type;
  int request_index;
};

DataStreamer * build_data_streamer(datastreamer_desc dd, DataVals * dvs   );
void *data_streamer_thread_func( DataStreamer * ds);



