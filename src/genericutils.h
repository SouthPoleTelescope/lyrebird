#pragma once

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>


#define DEBUG_0 1
#define DEBUG_1 2
#define DEBUG_2 4
#define DEBUG_3 8
#define DEBUG_4 16
#define DEBUG_5 32
#define DEBUG_6 64
#define DEBUG_7 128

#define VALID_DEBUG (DEBUG_0 | DEBUG_1 | DEBUG_2)
#define DCOUT(msg,lvl) if ( VALID_DEBUG & lvl) (std::cout<<__FILE__ << ":"<<__LINE__<<" " << msg << endl)



void print_and_exit(std::string msg);

std::vector<std::string>  get_unique_strings(std::vector<std::string> & arr, int nelems);  

std::string read_file(std::string fn);

void print_string_vec( std::vector < std::string > v); 
bool sloppy_eq(float x, float y, float eps=0.0005);
void write_string_to_file( std::string file_path, std::string output_string );
bool file_exists(std::string path);
int is_glob_match( const char * pattern, const std::string & str);


