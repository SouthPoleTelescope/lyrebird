#include "plotbundler.h"
#include <assert.h>
#include <iostream>
#include <math.h>

using namespace std;


void PlotBundler::get_psd_start_and_sep(float & start, float & sep) {
	if (num_plots == 0) {
		start = 0;
		sep = 0;
	}
	float sample_rate = sample_rate_buffer[0];
	sep = sample_rate / buffer_size_;
	start = 0;
}


PlotBundler::PlotBundler(int max_num_plots, int buffer_size,  std::vector<VisElemPtr> * vis_elems){
  max_num_plots_ = max_num_plots;
  buffer_size_ = buffer_size;
  vis_elems_ = vis_elems;

  psd_buffer_size = buffer_size_/2+1;

  plot_vals = new float[max_num_plots_*buffer_size_];
  color_vals = new glm::vec3[max_num_plots_];
  last_updated = new int[max_num_plots_];
  previousVEInds = new int[max_num_plots_];

  psd_vals = new float[max_num_plots_*psd_buffer_size];
  fft_out = fftw_alloc_complex(psd_buffer_size); 
  psd_tmp_buffer = fftw_alloc_real(buffer_size_);
  psd_hann_buffer = new double[buffer_size_];

  sample_rate_buffer = new float[max_num_plots_];

  num_plots = 0;
  for (int i=0; i < max_num_plots_; i++){
    previousVEInds[i] = -1;
    last_updated[i] = -1;
  }

  fft_plan = fftw_plan_dft_r2c_1d(buffer_size_, psd_tmp_buffer, fft_out,FFTW_ESTIMATE);
  //fft_plan = ffts_init_1d_real(buffer_size_, 1);

  for (int i=0; i < buffer_size_; i++){
    psd_hann_buffer[i] = 0.5 * (1.0 - cosf( (2 * 3.14159 * i)/((float) buffer_size_  - 1.0f )));
  }
}

int PlotBundler::get_psd_buffer_size(){
  return psd_buffer_size;
}

PlotBundler::~PlotBundler(){
  delete [] plot_vals;
  delete [] psd_vals;
  delete [] color_vals;
  delete [] last_updated;
  delete [] previousVEInds;
  delete [] psd_hann_buffer;

  delete [] sample_rate_buffer;

  fftw_free(psd_tmp_buffer);
  fftw_free(fft_out);
  fftw_destroy_plan(fft_plan);
}


int PlotBundler::get_num_plots(){
  return num_plots;
}

// pis = plot indices,  cis = color indices
void PlotBundler::update_plots(std::list<int> & pis, std::list<glm::vec3> & cis){
  auto it1 = pis.begin();
  auto it2 = cis.begin();

  num_plots = 0;
  for(; it1 != pis.end() && it2 != cis.end(); ++it1, ++it2){
    color_vals[num_plots] = *it2;
    //cout<< "Plot "<< num_plots<< " r" << (*it2).r<<"g"<<(*it2).g<<"b"<<(*it2).b<<endl;
    (*vis_elems_)[*it1]->get_current_equation().get_bulk_value(&(plot_vals[num_plots * buffer_size_]));
    
    sample_rate_buffer[num_plots] = (*vis_elems_)[*it1]->get_current_equation().get_sample_rate();
    
    //check to see if the highlighted elements have changed
    if (previousVEInds[num_plots] != *it1){
	    previousVEInds[num_plots] = *it1;
	    last_updated[num_plots] = -1;
    }
    
    num_plots++;
    if (num_plots >= max_num_plots_) break;
  }

  /**
  for (int i=0; i < num_plots * buffer_size_; i++){
	  plot_vals[i] = isfinite(plot_vals[i]) ? plot_vals[i] : 0.0f;
  }
  **/
  plot_min = 1e23;
  plot_max = -1e23;
  for (int i=0; i < num_plots * buffer_size_; i++){
    if (plot_vals[i] < plot_min ) plot_min = plot_vals[i];
    if (plot_vals[i] > plot_max ) plot_max = plot_vals[i];
  }


  for (int i=0; i < num_plots; i++){
    if (last_updated[i] < 0 || last_updated[i] > max_num_plots_){
      //cout<<"updating i "<<i<<endl;
      //update the fft
	    float mean_val = 0;
	    for (int j=0; j<buffer_size_; j++) mean_val += plot_vals[i * buffer_size_ + j];
	    mean_val /= (float) buffer_size_;


	    for (int j=0; j<buffer_size_; j++) psd_tmp_buffer[j] = psd_hann_buffer[j] * (plot_vals[i * buffer_size_ + j] - mean_val);
	    fftw_execute(fft_plan);
	    for (int j=1; j < psd_buffer_size; j++){
		    psd_vals[i*psd_buffer_size + j - 1] = sqrt( fft_out[j][0]*fft_out[j][0] + 
								fft_out[j][1]*fft_out[j][1]);
	    }
	    //zeros the lowest bins because fuck it
	    for (int j=0; j < 1; j++)
		    psd_vals[i*psd_buffer_size+j] = 0;
	    psd_vals[i*psd_buffer_size+psd_buffer_size-1]  = 0;
	    
	    last_updated[i] = 0;
    }else{
	    last_updated[i]++;
    }
  }
  
  psd_min = 0;
  psd_max = -1e30;
  for (int i=0; i < num_plots * psd_buffer_size; i++){
    if (psd_vals[i] < psd_min ) psd_min = psd_vals[i];
    if (psd_vals[i] > psd_max ) psd_max = psd_vals[i];
  }
}

float * PlotBundler::get_plot(int index, glm::vec3 & plot_color){
  assert(index < num_plots);
  plot_color = color_vals[index];
  return plot_vals + buffer_size_ * index;
}

float * PlotBundler::get_psd(int index, glm::vec3 & plot_color){
  assert(index < num_plots);
  plot_color = color_vals[index];
  return psd_vals + psd_buffer_size * index;
}

void PlotBundler::get_plot_min_max(float & min, float & max){
  min = plot_min;
  max = plot_max;
}

void PlotBundler::get_psd_min_max(float & min, float & max){
  min = psd_min;
  max = psd_max;
}

