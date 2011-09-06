
#include "base/common.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

namespace cheaproute
{
 
void AbortWithMessage(const char* format_string, ...) {
  va_list args;
  va_start (args, format_string);
  vfprintf (stderr, format_string, args);
  va_end (args);
  fprintf(stderr, "\n");
  abort();
}

void AbortWithPosixError(const char* format_string, ...) {
  va_list args;
  va_start (args, format_string);
  vfprintf (stderr, format_string, args);
  va_end (args);
  fprintf(stderr, ": ");
  perror("");
  abort();
}
void CheckVoidPtrNotNull(void* ptr, const char* name) {
  if (!ptr) {
    AbortWithMessage("Fatal error: %s is null", name);
  }
}

void CheckPosixPtrOp(const void* opResult, const char* description) {
  if (!opResult) {
    AbortWithPosixError("%s", description);
  }
}

struct StringPrinterInternals {
  FILE* file;
};

static ssize_t StringPrinter_Write(void* cookie, const char* buf, size_t nBytes) {
  static_cast<StringPrinter*>(cookie)->Append(buf, nBytes);
  return nBytes;
}

StringPrinter::StringPrinter() {
  cookie_io_functions_t functions;
  memset(&functions, 0, sizeof(functions));
  functions.write = StringPrinter_Write;
  
  internals_.reset(new StringPrinterInternals);
  internals_->file = CheckPosixOp(fopencookie(this, "w", functions),
                                  "creating custom io stream for StringPrinter");
}

StringPrinter::~StringPrinter() {
  fclose(internals_->file);
}

void StringPrinter::Printf(const char* format_string, ...) {
  va_list args;
  va_start (args, format_string);
  vfprintf (internals_->file, format_string, args);
  va_end (args);
  fflush(internals_->file);
}
void StringPrinter::VPrintf(const char* format_string, va_list args) {
  
  vfprintf (internals_->file, format_string, args);
  fflush(internals_->file);
}

string StrPrintf(const char* format_string, ...) {
  StringPrinter str_printer;
  va_list args;
  va_start(args, format_string);
  str_printer.VPrintf(format_string, args);
  va_end(args);
  return str_printer.str();
}
string VStrPrintf(const char* format_string, va_list args) {
  StringPrinter str_printer;
  str_printer.VPrintf(format_string, args);
  return str_printer.str();
}

void AppendVectorU8(vector<uint8_t>* destination, const void* data, size_t len) {
  size_t old_size = destination->size();
  destination->resize(old_size + len);
  memcpy(&(*destination)[old_size], data, len);
}

const int8_t kHexValues[] = { 
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
   0,  1,  2,  3,  4,  5,  6,  7,    8,  9, -1, -1, -1, -1, -1, -1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1
};

const char kHexChars[] = {'0', '1', '2', '3', '4', '5', '6', '7', 
                        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

string FormatHex(const void* ptr, size_t size) {
  string result;
  
  const uint8_t* p = static_cast<const uint8_t*>(ptr);
  const uint8_t* end = p + size;
  
  int line_position = 0;
  for(; p < end; p++) {
    result.push_back(kHexChars[(*p & 0xf0) >> 4]);
    result.push_back(kHexChars[(*p & 0x0f) >> 0]);
    if (p + 1 < end) {
      line_position++;
      if (line_position == 8) {
        result.push_back(' ');
      }
      if (line_position == 16) {
        result.push_back('\n');
        line_position = 0;
      } else {
        result.push_back(' ');
      }
    }

  }
  return result;
}

bool ParseHex(const char* str, vector<uint8_t>* destination) {
  
  for(const char* p = str; *p; p++) {
    if (*p == '\t' || *p == '\r' || *p == '\n' || *p == ' ')
      continue;
    
    int nibble_1 = kHexValues[static_cast<int>(*p)];
    if (!*++p)
      return false;
    int nibble_2 = kHexValues[static_cast<int>(*p)];
    if (nibble_1 == -1 || nibble_2 == -1)
      return false;
    
    destination->push_back(static_cast<uint8_t>((nibble_1 << 4) | nibble_2));
  }
  return true;
}

}