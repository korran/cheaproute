
// Copyright 2011 Kor Nielsen

#include "base/common.h"
#include "base/event_loop.h"

#include <stdio.h>
#include <unistd.h>

namespace cheaproute
{

class Program
{
public:
  Program() {
    some_string_ = "hello";
    loop_.Schedule(1.0, bind(&Program::DoSomething, this));
  }
  
  void DoSomething() { 
    printf("Something %s\n", some_string_.c_str());
  }
  
  void Run() { loop_.Run(); }
  
private:
  Program(const Program& other);
  EventLoop loop_;
  string some_string_;
};


int main(int argc, const char* const argv[]) {
  Program program;
  program.Run();
}

}




int main(int argc, const char* const argv[]) {
  return cheaproute::main(argc, argv);
}
