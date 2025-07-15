#include <string>
#include <iostream>

#define LOG(message) std::cout << message << '\n';
#define ERROR(message) std::cout << "Error: " << message << '\n';
#define EXCEPTION(message) std::cout << "Exception: " << message << '\n'; exit(-1);