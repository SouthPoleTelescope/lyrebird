#include "hdfstreamer.h"
#include <iostream>


using namespace std;

HdfStreamer::HdfStreamer(
			   std::string file, 
			   std::vector< std::string > paths,  
			   std::vector< std::string> ids,
			   DataVals * dv, int us_update_time
			   ):DataStreamer(file, paths, ids, dv, us_update_time, DSRT_REQUEST_HISTORY){

}


//we are going to assume that all the data sets have the same dimensions
void HdfStreamer::initialize(){
  assert(s_paths.size() > 0);
  assert(s_paths.size() == s_path_inds.size());

  herr_t status;
  cout<<"loading hdf streamer of file " << s_file << endl;
  file_id_ = H5Fopen (s_file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  

  hsize_t dimsize = 0;
  int ndims = 0;
  

  status = H5LTget_dataset_ndims ( file_id_, s_paths[0].c_str(), &ndims);
  assert(ndims == 1);
  status = H5LTget_dataset_info(file_id_, s_paths[0].c_str(), &dimsize, NULL,NULL);


  //check that all the data sets are the same size
  for (int i=1; i < s_paths.size(); i++){
    hsize_t cdimsize = 0;
    int cndims = 0;

    status = H5LTget_dataset_ndims ( file_id_, s_paths[0].c_str(), &cndims);
    assert(cndims == 1);
    status = H5LTget_dataset_info(file_id_, s_paths[0].c_str(), &cdimsize, NULL,NULL);
    assert(dimsize == cdimsize);
  }
  
  //allocate our buffer
  val_length_ = dimsize;
  val_matrix_ = new float[s_paths.size() * val_length_];
  
  //load the information
  for (int i=0; i < s_paths.size(); i++){
    status = H5LTread_dataset_float(file_id_,s_paths[i].c_str(), val_matrix_ + i * val_length_);
  }
}


void HdfStreamer::uninitialize(){
  std::cout<<"uninit test"<<std::endl;
  H5Fclose(file_id_);
  delete [] val_matrix_;
}

void HdfStreamer::update_values(int ind){
  for (int i=0; i < s_path_inds.size(); i++){
    data_vals->update_val(s_path_inds[i], val_matrix_[i * val_length_ + ind]);
  }
}

int HdfStreamer::get_num_elements(){
  return val_length_;
}
