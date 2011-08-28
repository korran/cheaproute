
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
    AbortWithPosixError(description);
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

}