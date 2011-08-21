#include <tr1/functional>
#include <string>
#include <vector>

namespace cheaproute
{
  // Import common STL types into our namespace
  // This is the ONLY header file in the project where the using 
  // keyword can be used outside of a class or function
  using std::string;
  using std::vector;
  using std::tr1::function;
  using std::tr1::bind;
};