#pragma once

// Ideally, this file should be set up as a precompiled header

#include "base/scoped_ptr.h"

#include <tr1/functional>
#include <tr1/memory>
#include <tr1/unordered_map>
#include <string>
#include <vector>
#include <deque>
#include <stack>
#include <set>
#include <cassert>
#include <stdint.h>
#include <stdio.h>

namespace cheaproute
{
  // Import common STL types into our namespace
  // This is the ONLY header file in the project where the using 
  // keyword should be used outside of a class or function
  using std::string;
  using std::vector;
  using std::tr1::function;
  using std::tr1::bind;
  using std::tr1::shared_ptr;
  using std::tr1::placeholders::_1;
  using std::tr1::enable_shared_from_this;
  using std::tr1::weak_ptr;
  using std::tr1::unordered_map;
  using std::deque;
  using std::stack;
  using std::set;
  using std::make_pair;
  
  void AbortWithMessage(const char* format_string, ...)
      __attribute__ ((format (printf, 1, 2)));
  
  void AbortWithPosixError(int err_no, const char* format_string, ...)
      __attribute__ ((format (printf, 2, 3)));
      
  void AbortWithPosixError(const char* format_string, ...)
      __attribute__ ((format (printf, 1, 2)));
      
  void CheckVoidPtrNotNull(void*, const char* name);
  
  template<typename T>
  T* CheckNotNull(T* value, const char* name) {
    CheckVoidPtrNotNull(value, name);
    return value;
  }
  
  template<typename T>
  void Append(vector<T>* vector, T* elements, size_t count) {
    vector->reserve(vector->size() + count);
    for (T* i = elements; i != elements + count; i++) {
      vector->push_back(*i);
    }
  }
  
  void CheckPosixPtrOp(const void* opResult, const char* description);
  
  template<typename T>
  T* CheckPosixOp(T* opResult, const char* description) {
    CheckPosixPtrOp(opResult, description);
    return opResult;
  }
  
  class StringPrinterInternals;
  class StringPrinter {
  public:
    StringPrinter();
    ~StringPrinter();
    
    void Printf(const char* format_string, ...)
        __attribute__ ((format (printf, 2, 3)));
    
    void VPrintf(const char* format_string, va_list argp);
        
    const string& str() const { return str_; }
    void Append(const char* data, size_t len) {
      str_.append(data, len);
    }
  private:
    string str_;
    scoped_ptr<StringPrinterInternals> internals_;
  };
  
  string StrPrintf(const char* format_string, ...)
      __attribute__ ((format (printf, 1, 2)));
      
  string VStrPrintf(const char* format_string, va_list argp);
    
  template <class T, size_t N>
  size_t ArrayLength(T (&x)[N]) { 
    return N;
  }
  
  void AppendVectorU8(vector<uint8_t>* destination, const void* data, size_t len);
  
  extern const char kHexChars[];
  extern const int8_t kHexValues[];
  
  string FormatHex(const void* ptr, size_t size);
  bool ParseHex(const char* str, vector<uint8_t>* destination);
}

