#pragma once
#include <list>
#include <vector>

#include "glm/glm.hpp"
#include <fftw3.h>

#include "visualelement.h"

class PlotBundler{
public:
	PlotBundler(int max_num_plots, int buffer_size,  std::vector<VisElemPtr> * vis_elems);
	~PlotBundler();
	
	int get_num_plots();  
	void update_plots(std::list<int> & pis, std::list<glm::vec3> & cis);
	//index is the plot num
	float * get_plot(int pnum, glm::vec3 & plot_color);
	float * get_psd(int pnum, glm::vec3 & plot_color);
	
	void get_plot_min_max(float & min, float & max);
	void get_psd_min_max(float & min, float & max);
	
	int get_psd_buffer_size();
	
	void get_psd_start_and_sep(float & start, float & sep);


private:
	PlotBundler(const PlotBundler&); //prevent copy construction      
	PlotBundler& operator=(const PlotBundler&); //prevent assignment
	std::vector<VisElemPtr> * vis_elems_;
	
	float * plot_vals;
	float * psd_vals;
	
	double * psd_tmp_buffer;
	double * psd_hann_buffer;

	glm::vec3 * color_vals;
	int * last_updated;
	int * previousVEInds;

	float * sample_rate_buffer;

	fftw_complex * fft_out;  
	int num_plots;  
	int max_num_plots_;
	int buffer_size_;
	
	int psd_buffer_size;
	
	float plot_min, plot_max;
	float psd_min, psd_max;
	
	fftw_plan fft_plan;
};
