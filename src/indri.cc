#include <nan.h>
#include "searcher.h"

using namespace Nan;

  
void InitIndri(v8::Local<v8::Object> exports) {
  Searcher::Init(exports);
}


NODE_MODULE(indri, InitIndri)