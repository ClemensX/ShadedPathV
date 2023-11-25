// A simple program that computes the square root of a number
#include <cmath>
#include <iostream>
#include <string>
#include "tutorialConfig.h"
#include "mymath.h"

// c++ 20 feature:
consteval int GetInt(int x){
    return x;
}

int main(int argc, char* argv[])
{
    std::cout << "shadedpathv " << VERSION_MAJOR << "." << VERSION_MINOR << std::endl;
  if (argc < 2) {
    // TODO 12: Create a print statement using Tutorial_VERSION_MAJOR
    //          and Tutorial_VERSION_MINOR
    std::cout << "UsageX: " << argv[0] << " number" << std::endl;
    return 1;
  }


  // convert input to double
  // TODO 4: Replace atof(argv[1]) with std::stod(argv[1])
  const double inputValue = std::stod(argv[1]);

  // calculate square root
  const double outputValue = mathfunctions::sqrt(inputValue);
  std::cout << "The square root of " << inputValue << " is " << outputValue
            << std::endl;
  return 0;
}