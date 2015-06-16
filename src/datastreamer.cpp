#include "datastreamer.h"

#include <unistd.h>
#include <stdio.h>

#include "teststreamer.h"
#include "genericutils.h"

//#include "dfmuxstreamer.h"
#include "logging.h"

using namespace std;

pthread_mutex_t DataStreamer::init_uninit_mutex_ = PTHREAD_MUTEX_INITIALIZER;

void *data_streamer_thread_func( void * ds){
  //printf("calling datastreamer thread func\n");
  int dsrt = ((DataStreamer*)ds)->get_request_type();
  if (dsrt == DSRT_STREAMING)
    ((DataStreamer*)ds)->thread_loop_auto();
  else if (  (dsrt == DSRT_REQUEST_HISTORY) 
	     ||  (dsrt == DSRT_REQUEST))
    ((DataStreamer*)ds)->thread_loop_request();
  else if ( dsrt == DSRT_CALLBACK)
    ((DataStreamer*)ds)->thread_loop_callback();
  return NULL;
}

DataStreamer * build_data_streamer(datastreamer_desc dd , DataVals * dvs    ){
  if (dd.tp == "test_streamer") return new TestStreamer( dd.streamer_json_desc, dd.tag, dvs, dd.us_update_time);
  //else if (tp == "hdf_history") return new HdfStreamer( file, paths, ids, dv, us_update_time);
  else if (dd.tp == "housekeeping")  return new HkStreamer( dd.tag, dd.streamer_json_desc, dvs);
  else if (tp == "dfmux")return new DfmuxStreamer( file, paths, ids, dv, us_update_time);
  else{
    log_fatal("Requested streamer type %s and I don't know what this is", dd.tp.c_str() );
    return NULL;
  }
}

DataStreamer::DataStreamer(std::string tag, 
			   DataVals * dv, int us_update_time,
			   int data_source_request_type  ){
  s_tag = tag;
  
  should_live = false;
  sleep_time = us_update_time;
  
  data_vals = dv;
  
  ds_req_type = data_source_request_type;
  request_index = -1;
}


void DataStreamer::thread_loop_auto(){
  pthread_mutex_lock(&init_uninit_mutex_);
  initialize();
  pthread_mutex_unlock(&init_uninit_mutex_);
  while (should_live){
    update_values(-1);
    usleep(sleep_time);    
  }

  pthread_mutex_lock(&init_uninit_mutex_);
  uninitialize();
  pthread_mutex_unlock(&init_uninit_mutex_);

}


void DataStreamer::thread_loop_request(){

  pthread_mutex_lock(&init_uninit_mutex_);
  initialize();
  pthread_mutex_unlock(&init_uninit_mutex_);

  while (should_live){

    while(request_index < 0 && should_live){
      pthread_mutex_lock(&count_mutex);
      pthread_cond_wait( &count_threshold_cv, &count_mutex);
      pthread_mutex_unlock(&count_mutex);
    }
    update_values(request_index);
    request_index = -1;
  }
  pthread_mutex_lock(&init_uninit_mutex_);
  uninitialize();
  pthread_mutex_unlock(&init_uninit_mutex_);
}




void DataStreamer::thread_loop_callback(){
  pthread_mutex_lock(&init_uninit_mutex_);
  initialize();
  pthread_mutex_unlock(&init_uninit_mutex_);
  while (should_live){
    pthread_mutex_lock(&count_mutex);
    pthread_cond_wait( &count_threshold_cv, &count_mutex);
    pthread_mutex_unlock(&count_mutex);
  }
  pthread_mutex_lock(&init_uninit_mutex_);
  uninitialize();
  pthread_mutex_unlock(&init_uninit_mutex_);
}




void DataStreamer::die_gracefully(){
  if (!should_live) return;
  should_live = false;

  if (ds_req_type == DSRT_REQUEST || ds_req_type == DSRT_REQUEST_HISTORY){
    pthread_mutex_lock(&count_mutex);
    pthread_cond_signal( &count_threshold_cv);
    pthread_mutex_unlock(&count_mutex);
  }
}

void DataStreamer::bury_body(){

  pthread_join(d_thread, NULL);
}

void DataStreamer::start_recording(){
  should_live = true;
  if (pthread_create( &d_thread, NULL, data_streamer_thread_func, (void*)this)){
    log_fatal("Trouble with spawning a thread");
  }
}


int DataStreamer::get_request_type(){
  return ds_req_type;
}

std::string DataStreamer::get_tag(){
  return s_tag;
}


void DataStreamer::request_values(int ind){
  pthread_mutex_lock(&count_mutex);
  request_index = ind;
  pthread_cond_signal( &count_threshold_cv );
  pthread_mutex_unlock(&count_mutex);
}
