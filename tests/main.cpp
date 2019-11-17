#include <clocale>
#include <iostream>
#include "main.h"

int main(int argc, char **argv){
  if(!setlocale(LC_ALL, "")){
    std::cerr << "Coudln't set locale based on user preferences!" << std::endl;
    return EXIT_FAILURE;
  }
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
