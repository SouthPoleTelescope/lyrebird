#include "datastreamer.h"

#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "teststreamer.h"
#include "hdfstreamer.h"
#include "dfmuxstreamer.h"


#include "genericutils.h"


using namespace std;

pthread_mutex_t DataStreamer::init_uninit_mutex_ = PTHREAD_MUTEX_INITIALIZER;

void *data_streamer_thread_func( void * ds){
  printf("calling datastreamer thread func\n");
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

DataStreamer * build_data_streamer(std::string tp,
				   std::string file, 
				   std::vector< std::string > paths,  
				   std::vector< std::string> ids,
				   DataVals * dv, int us_update_time
				   ){
  if (tp == "test_streamer") return new TestStreamer( file, paths, ids, dv, us_update_time);
  else if (tp == "hdf_history") return new HdfStreamer( file, paths, ids, dv, us_update_time);
  //else if (tp == "hk_request")  return new HkStreamer( file, paths, ids, dv, us_update_time);
  //else if (tp == "dfmux_streamer")return new DfmuxStreamer( file, paths, ids, dv, us_update_time);
  else{
    cout<<"Requested streamer type " << tp << endl;
    print_and_exit("I don't know what this is");
    return NULL;
  }



}



DataStreamer::DataStreamer(std::string file, 
			   std::vector< std::string > paths,  
			   std::vector< std::string> ids,
			   DataVals * dv, int us_update_time,
			   int data_source_request_type
			   ){

  s_file = file;
  s_paths = paths;
  s_path_inds = vector< int >(ids.size());
  for (int i = 0; i < ids.size(); i++) s_path_inds[i] = dv->get_ind(ids[i]);
  for (int i = 0; i < ids.size(); i++) assert(s_path_inds[i]!=-1);
  



  should_live = false;
  sleep_time = us_update_time;

  data_vals = dv;

  ds_req_type = data_source_request_type;
  request_index = -1;
}


void DataStreamer::thread_loop_auto(){
  cout<<"calling threadloop"<<endl;
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
  cout<<"returning from threadloop"<<endl;
}


void DataStreamer::thread_loop_request(){
  cout<<"calling threadloop"<<endl;

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
  cout<<"returning from threadloop"<<endl;
}




void DataStreamer::thread_loop_callback(){
  cout<<"calling threadloop"<<endl;

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
  cout<<"returning from threadloop"<<endl;
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
  //if (d_thread.joinable()) d_thread.join();
}

void DataStreamer::start_recording(){
  cout<<"recording "<<endl;
  should_live = true;
  //auto threadF = [this] { thread_loop(); };
  int iret2 = pthread_create( &d_thread, NULL, data_streamer_thread_func, (void*)this);
  printf("pthread_create gave me %d\n", iret2);
  //d_thread = thread(  [this] { thread_loop(); } ); //closures, weird
}


int DataStreamer::get_request_type(){
  return ds_req_type;
}

std::string DataStreamer::get_tag(){
  return s_file;
}


void DataStreamer::request_values(int ind){
  pthread_mutex_lock(&count_mutex);
  request_index = ind;
  pthread_cond_signal( &count_threshold_cv );
  pthread_mutex_unlock(&count_mutex);
}
