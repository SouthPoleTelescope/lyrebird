#include "equationmap.h"
#include "genericutils.h"

EquationMap::EquationMap(int number_of_equations, DataVals * data_vals) :
  data_vals_(data_vals), max_num_eqs_(number_of_equations), num_eqs_(0){
  eq_vec_ = std::vector<std::shared_ptr<Equation> >(number_of_equations, NULL);
}

void EquationMap::add_equation(equation_desc desc){
  if (num_eqs_ >= max_num_eqs_){
    print_and_exit("too many eqs trying to be added");
  }
  eq_vec_[num_eqs_] = std::shared_ptr<Equation>(new Equation);
  eq_vec_[num_eqs_]->set_equation(data_vals_,desc);
  ids_map_[desc.label] = num_eqs_;
  num_eqs_++;
}
Equation & EquationMap::get_eq(int i){
  return *(eq_vec_[i]);
}
int EquationMap::get_eq_index(std::string s){
  if (ids_map_.find(s) == ids_map_.end()) print_and_exit("could not find equation");

  return ids_map_[s];
}
