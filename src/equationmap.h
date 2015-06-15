#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "equation.h"

class EquationMap{
 public:
  EquationMap(int number_of_equations, DataVals * data_vals);
  void add_equation(equation_desc desc);
  Equation & get_eq(int i);
  int get_eq_index(std::string s);
 private:
  std::unordered_map<std::string, int> ids_map_;
  std::vector<std::shared_ptr<Equation> > eq_vec_;
  DataVals * data_vals_;
  int max_num_eqs_;
  int num_eqs_;
};
