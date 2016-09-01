#include "numberlineart.h"
#include <stdio.h>
void get_number_lines(char num, std::vector<glm::vec2> & out_lines, float x_offset) {
	out_lines.clear();
	switch (num) {
	case -1:
		out_lines.push_back( glm::vec2( 0.5 + x_offset, 0.0));
		out_lines.push_back( glm::vec2( 0.5 + x_offset, 0.25));
		return;
		break;
	case 0:
		out_lines.push_back(glm::vec2(0 + x_offset, 0));
		out_lines.push_back(glm::vec2(1 + x_offset, 0));

		out_lines.push_back(glm::vec2(1 + x_offset, 0));
		out_lines.push_back(glm::vec2(1 + x_offset, 2));

		out_lines.push_back(glm::vec2(1 + x_offset, 2));
		out_lines.push_back(glm::vec2(0 + x_offset, 2));

		out_lines.push_back(glm::vec2(0 + x_offset, 2));
		out_lines.push_back(glm::vec2(0 + x_offset, 0));

		return;
		break;
	case 1:
		out_lines.push_back(glm::vec2(0.5 + x_offset, 0));
		out_lines.push_back(glm::vec2(0.5 + x_offset, 2));
		return;
		break;
	}
}

void get_power_of_ten_number_lines(int pow, 
				   float scale_factor,
				   float x_offset,
				   float y_offset,
				   float z_offset,
				   std::vector<float> & out_vals ) {
	std::vector<glm::vec2> out_lines;
	float scaling = 1.3;
	if (pow >= 0) {
		get_number_lines(1, out_lines, 0);
		std::vector<glm::vec2> tmp_lines;
		for (int i=0; i < pow; i++) {
			get_number_lines(0, tmp_lines, (i+1)*scaling);
			out_lines.insert(out_lines.end(), tmp_lines.begin(), tmp_lines.end());
		}
	} else {
		get_number_lines(0, out_lines, 0);
		std::vector<glm::vec2> tmp_lines;
		get_number_lines(-1, tmp_lines, scaling);
		out_lines.insert(out_lines.end(), tmp_lines.begin(), tmp_lines.end());

		for (int i=0; i < (pow * -1) -1; i++) {
			get_number_lines(0, tmp_lines, 2 * scaling + i*scaling);
			out_lines.insert(out_lines.end(), tmp_lines.begin(), tmp_lines.end());
		}

		get_number_lines(1, tmp_lines, scaling * (pow * -1 + 1));
		out_lines.insert(out_lines.end(), tmp_lines.begin(), tmp_lines.end());
	}

	out_vals = std::vector<float>(out_lines.size() * 3, 0);
	for (size_t i=0; i<out_lines.size(); i++) { 
		out_vals[i*3 + 0 ] = out_lines[i].x * scale_factor + x_offset;
		out_vals[i*3 + 1 ] = out_lines[i].y * scale_factor + y_offset;
		out_vals[i*3 + 2 ] = z_offset;
	}
}
