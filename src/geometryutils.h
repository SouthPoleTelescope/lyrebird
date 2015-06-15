#pragma once
#include <GL/glew.h>
#include "glm/glm.hpp"
#include <vector>  // Include STL vector class.
#include <string>


// triangulate a contour/polygon, places results in STL vector
// as series of triangles.
bool triangulate_polygon(const std::vector < glm::vec2 > &contour,
                    std::vector < glm::vec2 > &result);

// compute area of a contour/polygon
float get_contour_area(const std::vector < glm::vec2 > &contour);

// decide if point Px/Py is inside triangle defined by
// (Ax,Ay) (Bx,By) (Cx,Cy)
bool inside_triangle(float Ax, float Ay,
                    float Bx, float By,
                    float Cx, float Cy,
                    float Px, float Py);


void con_poly_to_tris(std::vector<std::vector<glm::vec2> > & polygons, 
		      std::vector< glm::vec4 > & polygon_colors,
		      std::vector < glm::vec2 > & out_vertices,
		      std::vector < glm::vec2 > & out_uvs,
		      std::vector < glm::vec4 > & out_color);


float dist_pt_seg(float x, float y, float px, float py, float qx, float qy);

void cubic_bez(float x1, float y1, float x2, float y2,
	      float x3, float y3, float x4, float y4,
	      float tol, int level, std::vector<glm::vec2> verts);


void con_svg_to_geo(std::string fn, float tol,
		    std::vector < glm::vec2 > & out_vertices,
		    std::vector < glm::vec2 > & out_uvs,
		    std::vector < glm::vec4 > & out_color
		    );


void con_svg_to_polys(std::string fn, float tol,
		      std::vector<std::vector<glm::vec2> > & polygons,
		      std::vector< glm::vec4 > & polygon_colors
		      );


glm::vec2 transform_vec2(glm::vec2 in, glm::mat4 trans);
