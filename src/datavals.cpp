#include "datavals.h"
#include <assert.h>
#include <ctime>
#include <iostream>

#include <GLFW/glfw3.h>


#include "genericutils.h"
#include "equation.h"
#include "logging.h"
using namespace std;



DataVals::DataVals(int n_vals, int buffer_size){
	//vals = new float[n_vals];
	buffer_size_ = buffer_size;
	buffer_size_full_ = buffer_size_ + 1;
	array_size_ = n_vals;
}



void DataVals::initialize(){
	ring_indices_ = new int[array_size_];
	ring_buffers_ = std::vector< std::vector<float> >( array_size_, std::vector<float> (1, 0));;//new float[array_size_ * (buffer_size_full_)];
	
	is_buffered_ = new int[array_size_];

	is_mean_filtered_ = new int[array_size_];
	mean_val_ = new float[array_size_];
	mean_decay_ = new float[array_size_];

	n_vals_ = new int[array_size_];
	start_times_ = new float[array_size_];
	
	n_current_ = 0;
	for (int i=0; i < array_size_; i++){
		//vals[i] = 0;
		ring_buffers_[ i ][0] = 0;
		ring_indices_[i] = -1;
		is_buffered_[i] = 0;
		is_mean_filtered_[i] = 0;

		mean_val_[i] = 0;
		mean_decay_[i] = 0;
		
		n_vals_[i] = 0;
		start_times_[i] = 0;
		
	}
	//create read write lock

	if( pthread_rwlock_init( &rwlock_, NULL)) log_fatal("rwlock_ init failed");
	is_paused_ = false;
}


void DataVals::register_data_source(int n_vals){
	array_size_ += n_vals;
}


DataVals::~DataVals(){
  //delete [] vals;
  delete [] ring_indices_;
  delete [] is_buffered_;
  delete [] is_mean_filtered_;
 
  delete [] mean_val_;
  
  delete [] mean_decay_;

  delete [] n_vals_;
  delete [] start_times_;

}

int DataVals::get_ind(std::string id){
  if (id_mapping_.find(id) == id_mapping_.end()){
    log_warn("ID %s not found\n", id.c_str());
    return -1;
  }  else
    return id_mapping_[id];
}


bool DataVals::has_id(std::string id){
	return id_mapping_.find(id) != id_mapping_.end();
}


int DataVals::add_data_val(std::string id, float val, int is_buffered, float mean_decay){
	if (n_current_ >= array_size_) 
		log_fatal("Adding too many datavals.");

	int index = n_current_;
	if ( id_mapping_.find(id) != id_mapping_.end() ) {
		log_fatal( "%s already in DataVals when adding", id.c_str() );
	}
	
	if (is_buffered) {
		ring_buffers_[index] = std::vector<float>(buffer_size_full_, val);
	} else {
		ring_buffers_[index][0] = val;
	}
	l3_assert(mean_decay >= 0 && mean_decay < 1);

	n_current_++;
	id_mapping_[id] = index;
	is_buffered_[index] = is_buffered;
	is_mean_filtered_[index] = mean_decay != 0;
	mean_decay_[index] = mean_decay;

	return index;
}


void DataVals::update_val(int index, float val){
	if (is_paused_) return;
	if (index >= array_size_) log_fatal("Attempting to access index out of range");
	if (index < 0) return;
	
	
	if (is_mean_filtered_[index]) {
		if (mean_val_[index] == 0){
			mean_val_[index] = val;
		}
		float cached_mean_val = mean_val_[index];
		mean_val_[index] = mean_val_[index] * (1.0 - mean_decay_[index]) + val * mean_decay_[index];
		val -= cached_mean_val;
	}
	
	//grab read lock
	//vals[index] = val;
	ring_buffers_[index][0] = val;
	
	n_vals_[index] += 1;
	if (start_times_[index] == 0) 
		start_times_[index] = glfwGetTime();
	
	if (is_buffered_[index]){
		pthread_rwlock_rdlock (&rwlock_);
		if (ring_indices_[index] < 0) {
			ring_indices_[index] = 0;
			for (int i=0; i < buffer_size_full_; i++){
				ring_buffers_[index][i] = val;
			}
		}
		ring_buffers_[index][ ring_indices_[index] + 1] = val;
		ring_indices_[index]++;
		ring_indices_[index] = ring_indices_[index] % buffer_size_;
		pthread_rwlock_unlock (&rwlock_);
	} 
}


double DataVals::get_sample_rate(int index) {
	if (n_vals_[index] == 0) return 0.0;
	else return n_vals_[index] / (glfwGetTime() - start_times_[index]);
}




float * DataVals::get_addr(int index){
	if (index < 0 || index >= n_current_){
		print_and_exit("attempting to get non existent index from DataVals");
	}
	return &(ring_buffers_[index][0]);
}



/**
struct PPToken{
  pp_func func;
  int arg_num;
  float val;
  float * val_addr;
  int dv_index;
};
float evaluate_tokenized_equation_or_die(PPStack<PPToken> * token_stack){
  PPStack<float> eval_stack;
  eval_stack.size = 0;
  for (int i = token_stack->size-1; i >= 0; i--){
    token_stack->items[i].func(&eval_stack, 
			       token_stack->items[i].val_addr == NULL ? &(token_stack->items[i].val) : token_stack->items[i].val_addr, 
			       0 );
  }
  return eval_stack.items[0];
}
 **/


void DataVals::apply_bulk_func(PPStack<PPToken> * token_stack, float * vals){
  PPStack<float> eval_stack;
  pthread_rwlock_wrlock(&rwlock_ );
  for (int j = 0; j < buffer_size_; j++){
    eval_stack.size = 0;
    //does the pp calculation

    /**
       val_addr is the address fo the data val
       val is the value stored in the token if it is storing a numeric val
     **/
    for (int i = token_stack->size-1; i >= 0; i--){
      token_stack->items[i].func(&eval_stack, 
				 token_stack->items[i].val_addr == NULL ? 
  				   &(token_stack->items[i].val) : 
				   token_stack->items[i].val_addr, //value to plug in
				 ((ring_indices_[token_stack->items[i].dv_index] + j) % buffer_size_ + 1));
                                //offset (the buffer are all the values after the 0'th one so we need the +1.
      
    }
    //stores the value
    vals[j] = eval_stack.items[0];
  }
  pthread_rwlock_unlock(&rwlock_);
}


void DataVals::toggle_pause(){
  is_paused_ = !is_paused_;
}

int DataVals::get_buffer_size(){
  return buffer_size_;
}

int DataVals::is_buffered(int index){
  return is_buffered_[index];
}
