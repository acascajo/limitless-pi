#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>

using namespace std;

std::vector<string> read_directory(const std::string & path);

vector <string> split(string str, char delimeter);

/**
    Overloading the output stream operator for printing the contents of the vector

*/
template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
  if ( !v.empty() ) {
    out << '[';
    std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
    out << "\b\b]";
  }
  return out;
}

#endif