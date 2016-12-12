#include "geometryutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

//#include <glm/glm.hpp>

#include "nanosvg.h"
#include "genericutils.h"
#include "logging.h"

static const float EPSILON=0.0000000001f;

using namespace std;


glm::vec2 transform_vec2(glm::vec2 in, glm::mat4 trans){
  glm::vec2 out;
  out[0] = in.x*trans[0][0] + in.y*trans[1][0] + trans[3][0];
  out[1] = in.x*trans[0][1] + in.y*trans[1][1] + trans[3][1];
  return out;
}


float get_contour_area(const std::vector < glm::vec2 > &contour)
{

  int n = contour.size();

  float A=0.0f;

  for(int p=n-1,q=0; q<n; p=q++)
  {
    A+= contour[p].x*contour[q].y - contour[q].x*contour[p].y;
  }
  return A*0.5f;
}

   /*
     inside_triangle decides if a point P is Inside of the triangle
     defined by A, B, C.
   */
bool inside_triangle(float Ax, float Ay,
                      float Bx, float By,
                      float Cx, float Cy,
                      float Px, float Py)

{
  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = Cx - Bx;  
  ay = Cy - By;
  bx = Ax - Cx;  
  by = Ay - Cy;
  cx = Bx - Ax;  
  cy = By - Ay;
  apx= Px - Ax;  
  apy= Py - Ay;
  bpx= Px - Bx;  
  bpy= Py - By;
  cpx= Px - Cx;  
  cpy= Py - Cy;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};

bool _snip(const std::vector < glm::vec2 > &contour,int u,int v,int w,int n,int *V)
{
  int p;
  float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = contour[V[u]].x;
  Ay = contour[V[u]].y;

  Bx = contour[V[v]].x;
  By = contour[V[v]].y;

  Cx = contour[V[w]].x;
  Cy = contour[V[w]].y;

  if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) ) return false;

  for (p=0;p<n;p++)
  {
    if( (p == u) || (p == v) || (p == w) ) continue;
    Px = contour[V[p]].x;
    Py = contour[V[p]].y;
    if (inside_triangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) return false;
  }

  return true;
}

bool triangulate_polygon(const std::vector < glm::vec2 > &contour,std::vector < glm::vec2 > &result)
{
  /* allocate and initialize list of Vertices in polygon */

  int n = contour.size();
  if ( n < 3 ) return false;

  int *V = new int[n];
  /* we want a counter-clockwise polygon in V */
  if ( 0.0f < get_contour_area(contour) )
    for (int v=0; v<n; v++) V[v] = v;
  else
    for(int v=0; v<n; v++) V[v] = (n-1)-v;
  
  int nv = n;
  
  /*  remove nv-2 Vertices, creating 1 triangle every time */
  int count = 2*nv;   /* error detection */

  for(int m=0, v=nv-1; nv>2; )
  {
    /* if we loop, it is probably a non-simple polygon */
    if (0 >= (count--))
    {
      //** Triangulate: ERROR - probable bad polygon!
      return false;
    }

    /* three consecutive vertices in current polygon, <u,v,w> */
    int u = v  ; if (nv <= u) u = 0;     /* previous */
    v = u+1; if (nv <= v) v = 0;     /* new v    */
    int w = v+1; if (nv <= w) w = 0;     /* next     */

    if ( _snip(contour,u,v,w,nv,V) )
    {
      int a,b,c,s,t;

      /* true names of the vertices */
      a = V[u]; b = V[v]; c = V[w];

      /* output Triangle */
      result.push_back( contour[a] );
      result.push_back( contour[b] );
      result.push_back( contour[c] );

      m++;

      /* remove v from remaining polygon */
      for(s=v,t=v+1;t<nv;s++,t++) 
	      V[s] = V[t]; 
      nv--;

      /* resest error detection counter */
      count = 2*nv;
    }
  }

  delete [] V;

  return true;
}



void con_poly_to_tris(std::vector<std::vector<glm::vec2> > & polygons, 
		      std::vector< glm::vec4 > & polygon_colors,
		      std::vector < glm::vec2 > & out_vertices,
		      std::vector < glm::vec2 > & out_uvs,
		      std::vector < glm::vec4 > & out_color){
  for (size_t i=0; i < polygons.size(); i++){
    std::vector < glm::vec2 > triang_poly = std::vector < glm::vec2 >();
    std::vector < glm::vec4 > triang_color = std::vector < glm::vec4 >();
    
    triangulate_polygon(polygons[i], triang_poly);
    for (size_t j=0; j < triang_poly.size(); j++) triang_color.push_back(polygon_colors[i]);
    out_vertices.insert(out_vertices.end(), triang_poly.begin(), triang_poly.end());
    out_color.insert(out_color.end(), triang_color.begin(), triang_color.end());
  }
  
  float min_value = 0;
  float max_value = 0;
  for (size_t j=0; j < out_vertices.size(); j++){
    out_uvs.push_back(glm::vec2(out_vertices[j].x, out_vertices[j].y));
    if (out_vertices[j].x < min_value) min_value = out_vertices[j].x;
    if (out_vertices[j].y < min_value) min_value = out_vertices[j].y;
    if (out_vertices[j].x > max_value) max_value = out_vertices[j].x;
    if (out_vertices[j].y > max_value) max_value = out_vertices[j].y;
  }
  float delta = max_value - min_value;
  for (size_t j=0; j < out_uvs.size(); j++){
    out_uvs[j].x = (out_uvs[j].x  - min_value)/ delta;
    out_uvs[j].y = (out_uvs[j].y  - min_value)/ delta;;
  }
}

float dist_pt_seg(float x, float y, float px, float py, float qx, float qy)
{
  float pqx, pqy, dx, dy, d, t;
  pqx = qx-px;
  pqy = qy-py;
  dx = x-px;
  dy = y-py;
  d = pqx*pqx + pqy*pqy;
  t = pqx*dx + pqy*dy;
  if (d > 0) t /= d;
  if (t < 0) t = 0;
  else if (t > 1) t = 1;
  dx = px + t*pqx - x;
  dy = py + t*pqy - y;
  return dx*dx + dy*dy;
}


void cubic_bez(float x1, float y1, float x2, float y2,
	      float x3, float y3, float x4, float y4,
	      float tol, int level,  vector<glm::vec2> * verts)
{
  float x12,y12,x23,y23,x34,y34,x123,y123,x234,y234,x1234,y1234;
  float d;
  if (level > 12) return;
  x12 = (x1+x2)*0.5f;
  y12 = (y1+y2)*0.5f;
  x23 = (x2+x3)*0.5f;
  y23 = (y2+y3)*0.5f;
  x34 = (x3+x4)*0.5f;
  y34 = (y3+y4)*0.5f;
  x123 = (x12+x23)*0.5f;
  y123 = (y12+y23)*0.5f;
  x234 = (x23+x34)*0.5f;
  y234 = (y23+y34)*0.5f;
  x1234 = (x123+x234)*0.5f;
  y1234 = (y123+y234)*0.5f;
  d = dist_pt_seg(x1234, y1234, x1,y1, x4,y4);
  if (d > tol*tol) {
    cubic_bez(x1,y1, x12,y12, x123,y123, x1234,y1234, tol, level+1, verts);
    cubic_bez(x1234,y1234, x234,y234, x34,y34, x4,y4, tol, level+1, verts);
  } else {
    verts->push_back(glm::vec2(x4, y4));
  }
}

glm::vec4 con_nanosvg_color_to_glm_vec4(unsigned int color){
    unsigned int mask = 0xff;

    // color is ARGB
    //printf("got color %x\n", color);
    glm::vec4 val(0,0,0,0);
    val[0] = ((float)((color >> 0) & mask))/255.0f;  // R
    val[1] = ((float)((color >> 8) & mask))/255.0f;   // G
    val[2] = ((float)((color >> 16) & mask))/255.0f;       // B
    val[3] = ((float)((color >> 24) & mask))/255.0f;  // A
    //printf("converted to R %f  G  %f B  %f  A %f\n", val[0], val[1], val[2], val[3]);
    return val;
}




void con_svg_to_polys(string fn, float tol,
		      std::vector<std::vector<glm::vec2> > & polygons,
		      std::vector< glm::vec4 > & polygon_colors
		      ){
  NSVGimage* g_image = NULL;
  NSVGshape* shape;
  NSVGpath* path;
  

  l3_assert(file_exists(fn));
  g_image = nsvgParseFromFile(fn.c_str(), "px", 96.0f);
  
  for (shape = g_image->shapes; shape != NULL; shape = shape->next) {
    //DCOUT("parsing verts\n", DEBUG_0);
    for (path = shape->paths; path != NULL; path = path->next) {
      vector<glm::vec2> poly_verts;
      if (shape->fill.type == NSVG_PAINT_COLOR){
	//cout<<"is paint colo"<<endl;
	polygon_colors.push_back(con_nanosvg_color_to_glm_vec4(shape->fill.color));
	//polygon_colors.push_back(glm::vec4(1.0,1.0,1.0,0.5));

      }
      else if (shape->fill.type == NSVG_PAINT_NONE){
	continue;
      }
      else {
	polygon_colors.push_back(glm::vec4(1.0,1.0,1.0,0.5));
      }

      for (int i = 0; i < path->npts-1; i+=3){
	float* p = &(path->pts[i*2]);
	cubic_bez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7], tol, 0, &poly_verts);
      }
      polygons.push_back(poly_verts);
    }
  }
  

}





//verts to include
//units
//resolution
void con_svg_to_geo(string fn, float tol,
		    std::vector < glm::vec2 > & out_vertices,
		    std::vector < glm::vec2 > & out_uvs,
		    std::vector < glm::vec4 > & out_color
		    ){
  NSVGimage* g_image = NULL;
  NSVGshape* shape;
  NSVGpath* path;
  l3_assert(file_exists(fn));
  g_image = nsvgParseFromFile(fn.c_str(), "px", 96.0f);
  
  //converts the svg files to a vec of vertices and colors
  std::vector<std::vector<glm::vec2> > polygons;
  std::vector<glm::vec4 > polygon_colors;
  for (shape = g_image->shapes; shape != NULL; shape = shape->next) {
    for (path = shape->paths; path != NULL; path = path->next) {
      vector<glm::vec2> poly_verts;
      if (shape->fill.type == NSVG_PAINT_COLOR){
	polygon_colors.push_back(con_nanosvg_color_to_glm_vec4(shape->fill.color));
      }
      else if (shape->fill.type == NSVG_PAINT_NONE){
	continue;
      }
      else {
	polygon_colors.push_back(glm::vec4(1.0,1.0,1.0,0.5));
      }

      for (int i = 0; i < path->npts-1; i+=3){
	float* p = &(path->pts[i*2]);
	cubic_bez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7], tol, 0, &poly_verts);
      }

      polygons.push_back(poly_verts);

    }
  }

  std::reverse(polygons.begin(), polygons.end());
  std::reverse(polygon_colors.begin(), polygon_colors.end());

  //convert the polys to triangles
  con_poly_to_tris( polygons, polygon_colors,
		    out_vertices,out_uvs,out_color);
  
}



