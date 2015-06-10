#include "configparsing.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

#include "json/json.h"
#include "genericutils.h"



using namespace std;

//return {"function":eq_func, "eq_vars":eq_vars, "cmap": eq_color_map, "label": eq_label}
equation_desc parse_equation_desc(Json::Value & eqjson){
  equation_desc desc;
  desc.eq =   eqjson["function"].asString();
  desc.cmap_id = eqjson["cmap"].asString();
  desc.label = eqjson["label"].asString();
  return desc;
}

dataval_desc parse_dataval_desc(Json::Value & dvjson){
  //printf("parse_dataval_desc\n");
  dataval_desc dvdesc;
  dvdesc.id = dvjson[0].asString();
  dvdesc.init_val = dvjson[1].asFloat();
  dvdesc.is_buffered = dvjson[2].asBool();
  return dvdesc;
}

datastreamer_desc parse_datastreamer_desc(Json::Value & dsjson){
  //printf("parse_datastramer_desc\n");
  datastreamer_desc dd;
  dd.tag = dsjson["tag"].asString();
  dd.tp = dsjson["ds_type"].asString();
  dd.streamer_json_desc = dsjson["desc"];
  dd.us_update_time = dsjson["update_time"].asInt();
  return dd;
}

void parse_config_file(string in_file, 
		       vector<dataval_desc> & dataval_descs,
		       vector<datastreamer_desc> & datastream_descs,
		       vector<equation_desc> & equation_descs,
		       vector<vis_elem_repr> & vis_elems,
		       vector<string> & svg_paths,
		       vector<string> & svg_ids,
		       int & win_x_size,
		       int & win_y_size,
		       int & sub_sampling,
		       int & num_layers,
		       int & max_framerate,
		       int & max_num_plotted

		       ){
  string read_in_file = read_file(in_file);

  Json::Value root; 
  Json::Value data_stream_v; 
  Json::Reader reader;
  bool parsingSuccessful = reader.parse( read_in_file, root );
  if ( !parsingSuccessful ) {
    print_and_exit("Failed to parse configuration \n"+reader.getFormattedErrorMessages());
  }
  
  //////////////////////////////
  // Parse the general config //
  //////////////////////////////
  win_x_size = 640;
  win_y_size = 480;
  sub_sampling = 2;
  max_framerate = -1;

  num_layers = 1;

  if (root.isMember("general_settings")){
    Json::Value v = root["general_settings"];
    if (v.isMember("win_x_size")){
      if (v["win_x_size"].isInt()){
	win_x_size = v["win_x_size"].asInt();
      }else {
	print_and_exit("general_settings/win_x_size supplied but is not integer");
      }
    }

    if (v.isMember("win_y_size")){
      if (v["win_y_size"].isInt()){
	win_y_size = v["win_y_size"].asInt();
      }else {
	print_and_exit("general_settings/win_y_size supplied but is not integer");
      }
    }

    if (v.isMember("sub_sampling")){
      if (v["sub_sampling"].isInt()){
	sub_sampling = v["sub_sampling"].asInt();
      }else {
	print_and_exit("general_settings/sub_sampling supplied but is not integer");
      }
    }

    if (v.isMember("max_framerate")){
      if (v["max_framerate"].isInt()){
	max_framerate = v["max_framerate"].asInt();
      }else {
	print_and_exit("general_settings/max_framerate supplied but is not integer");
      }
    }      

    if (v.isMember("max_num_plotted")){
      if (v["max_num_plotted"].isInt()){
	max_num_plotted = v["max_num_plotted"].asInt();
      }else {
	print_and_exit("general_settings/max_num_plotted supplied but is not integer");
      }
    }      
  }
  
  /**
  if (root.isMember("modifiable_dvs")){
    Json::Value mod_dvs = root["modifiable_dvs"]; 
    for (int i=0; i < mod_dvs.size(); i++){
      if (!mod_dvs[i].isMember("tag") ) print_and_exit("tag not in modifiable_dvs");
      if (!mod_dvs[i].isMember("val") ) print_and_exit("val not in modifiable_dvs");
      modifiable_data_val_tags.push_back(mod_dvs[i]["tag"].asString());
      modifiable_data_vals.push_back(mod_dvs[i]["val"].asFloat());
    }
  }
  **/

  ////////////////////////////////
  //First parse the data streams
  ////////////////////////////////
  if (root.isMember("data_vals")){
    for (int i=0; i < root["data_vals"].size(); i++){ 
      dataval_descs.push_back(parse_dataval_desc(root["data_vals"][i]));
    }
  } else{
    print_and_exit("data_vals not found");
  }

  if (root.isMember("data_sources")){
    for (int i=0; i < root["data_sources"].size(); i++){ 
      datastream_descs.push_back(parse_datastreamer_desc(root["data_sources"][i]));
    }
  }
  
  //parse the equations
  if ( root.isMember("equations")){
    for (int i=0; i < root["equations"].size(); i++){
      equation_descs.push_back(parse_equation_desc( root["equations"][i]));
    }
  }


    ///////////////////////////////////
   //Parse the geometry description //
  ///////////////////////////////////
  if (!root.isMember("visual_elements")){
    print_and_exit("visual_elements needs to be in config file");
  }
  Json::Value visElemsJSON = root["visual_elements"];

  int n_vis_elems = visElemsJSON.size();
  if ( n_vis_elems == 0){
    print_and_exit("visual_elements is empty\n");
  }
  vis_elems = vector < vis_elem_repr >(n_vis_elems);

  
  vector<string> full_svg_paths;
  vector<string> full_svg_ids;
  for (int i=0; i < n_vis_elems; i++){
    Json::Value v = visElemsJSON[i];
    if (!v.isMember("x_center")  ) print_and_exit("x_center not found  in visual_element\n");
    if (!v.isMember("y_center") ) print_and_exit("y_center not found  in visual_element\n");
    if (!v.isMember("x_scale")  ) print_and_exit("x_scale not found  in visual_element\n");
    if (!v.isMember("y_scale")  ) print_and_exit("y_scale not found  in visual_element\n");
    if (!v.isMember("rotation") ) print_and_exit("rotation not found  in visual_element\n");
    if (!v.isMember("layer")  ) print_and_exit("layer not found  in visual_element\n");
    if ( !v["x_center"].isNumeric() ) print_and_exit("x_center  not valid in visual_element\n");
    if ( !v["y_center"].isNumeric() ) print_and_exit("y_center  not valid in visual_element\n");
    if ( !v["x_scale"].isNumeric() ) print_and_exit("x_scale  not valid in visual_element\n");
    if ( !v["y_scale"].isNumeric() ) print_and_exit("y_scale  not valid in visual_element\n");
    if ( !v["rotation"].isNumeric() ) print_and_exit("rotation  not valid in visual_element\n");
    if ( !v["layer"].isInt() ) print_and_exit("layer  not valid in visual_element\n");


    if (!v.isMember("svg_id") ) print_and_exit("svg_id not found  in visual_element\n");
    if (!v.isMember("svg_path") ) print_and_exit("svg_path not found  in visual_element\n");

    if (!v.isMember("highlight_svg_id") ) print_and_exit("highlight_svg_id not found  in visual_element\n");
    if (!v.isMember("highlight_svg_path") ) print_and_exit("highlight_svg_path not found  in visual_element\n");


    if (!v.isMember("labels") ) print_and_exit("labels not found  in visual_element\n");
    if (!v.isMember("equations") ) print_and_exit("equations not found  in visual_element\n");
    if (!v.isMember("group") ) print_and_exit("group not found  in visual_element\n");
    if (!v.isMember("labelled_data") ) print_and_exit("labelled_data not found  in visual_element\n");


    

    //parse geometric data
    vis_elems[i].x_center = v["x_center"].asFloat();
    vis_elems[i].y_center = v["y_center"].asFloat();
    vis_elems[i].x_scale = v["x_scale"].asFloat();
    vis_elems[i].y_scale = v["y_scale"].asFloat();
    vis_elems[i].rotation = v["rotation"].asFloat();
    vis_elems[i].layer = v["layer"].asInt();

    if (vis_elems[i].layer + 1 > num_layers){
      num_layers = vis_elems[i].layer + 1;
    }


    //parse the visual elements
    vis_elems[i].geo_id = v["svg_id"].asString();
    vis_elems[i].svg_path = v["svg_path"].asString();

    vis_elems[i].highlight_geo_id = v["highlight_svg_id"].asString();
    vis_elems[i].highlight_svg_path = v["highlight_svg_path"].asString();

    string svg_id = v["svg_id"].asString();
    string svg_path= v["svg_path"].asString();
    full_svg_ids.push_back(svg_id);
    full_svg_paths.push_back(svg_path);

    svg_id = v["highlight_svg_id"].asString();
    svg_path= v["highlight_svg_path"].asString();
    full_svg_ids.push_back(svg_id);
    full_svg_paths.push_back(svg_path);
    
    //parse tagging information
    for (int j=0; j< v["labels"].size(); j++){
      vis_elems[i].labels.push_back(v["labels"][j].asString());
    }
    
    vis_elems[i].group = v["group"].asString();
    for (int j = 0; j < v["labelled_data"].size(); j++){
      vis_elems[i].labelled_data.push_back( v["labelled_data"][0].asString() );
      vis_elems[i].labelled_data_vs.push_back( v["labelled_data"][1].asString() );
    }

    ////////////////////////
    //Parse the equations //
    ////////////////////////


    Json::Value eqv =  v["equations"];
    for (int j=0; j < eqv.size(); j++){
      vis_elems[i].equations.push_back( eqv[j].asString() );
    }

  }  
  //filter the svg ids and paths to be unique so we don't spend forever loading them
  for (int i=0; i < full_svg_ids.size(); i++){
    bool is_unique = true;
    for (int j = 0; j < svg_ids.size(); j++)
      if (full_svg_ids[i] == svg_ids[j]){
	is_unique = false;
	break;
      }
    if (is_unique){
      svg_ids.push_back(full_svg_ids[i]);
      svg_paths.push_back(full_svg_paths[i]);
    }
  }
}
