#include "genericutils.h"
#include <vector>
#include <list>
#include <fstream>
#include <streambuf>
#include <math.h>
#include <fnmatch.h>
#include <ctime>

using namespace std;


void print_and_exit(string msg){
  cout<<msg<<endl;
  exit(1);
}

vector<string>  get_unique_strings(vector<string> & arr, int nelems){
  vector<string> retv;
  for (int i=0; i < nelems; i++){
    bool is_unique = true;
    for (int j = i-1; j >= 0; j--){
      if (arr[i] == arr[j]){
	is_unique = false;
	break;
      }
    }
    if (is_unique) retv.push_back(arr[i]);
  }
  return retv;
}



string read_file(string fn){
  std::ifstream t(fn.c_str());
  std::string str((std::istreambuf_iterator<char>(t)),
		  std::istreambuf_iterator<char>());
  return str;
}

void print_string_vec( std::vector < std::string > v){
  for (size_t i = 0; i < v.size(); i++){
    cout << v[i] << ", ";
  }
  cout << endl;
}

bool sloppy_eq(float x, float y, float eps){
  return (fabs(x-y) > eps);
}


void write_string_to_file( std::string file_path, std::string output_string ){
  ofstream myfile;
  myfile.open (file_path);
  myfile << output_string;
  myfile.close();
}


bool file_exists(std::string path){
  ifstream f(path.c_str());
  if (f.good()) {
    f.close();
    return true;
  } else {
    f.close();
    return false;
  }   
}

int is_glob_match( const char * pattern, const std::string & str){
  return !fnmatch(pattern, str.c_str(), 0);
}

