#pragma once
#include <string>
#include <vector>
#include <pthread.h>
#include <unordered_map>


struct dataval_desc{
  std::string id;
  float init_val;
  bool is_buffered;
};

template <class T> struct PPStack;
struct PPToken;

class DataVals {
 public:
  DataVals(int n_vals, int buffer_size);
  ~DataVals();


  void initialize();
  void register_data_source(int n_vals);

  //get the index of a variable with name id
  // if not found returns -1
  int get_ind(std::string id); 

  bool has_id(std::string id); 

  //adds a data val.  Exits if you have too many
  int add_data_val(std::string id, float val, int is_buffered); 

  //returns null if not found
  float * get_addr(int index); 

  //update index with val
  void update_val(int index, float val);
  
  //return a buffer of values for data val at index
  std::vector<float> get_buffer_vals(int index);  
  
  void apply_bulk_func(PPStack<PPToken> * pp_stack, float * vals);  
  
  void toggle_pause();
  
  int get_buffer_size();
  int is_buffered(int index);

	double get_sample_rate(int index);

  
 private:
  DataVals(const DataVals&); //prevent copy construction      
  DataVals& operator=(const DataVals&); //prevent assignment
  
  int n_current_;
  //float * vals;
  
  int buffer_size_;
  int buffer_size_full_;
  
  int * is_buffered_;
  int * ring_indices_;

	std::vector<std::vector<float> > ring_buffers_;


  int * n_vals_;
  float * start_times_;

  pthread_rwlock_t  rwlock_;
  bool is_paused_;

  int array_size_;
  
  std::unordered_map<std::string, int> id_mapping_;
};
