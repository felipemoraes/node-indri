#include <nan.h>
#include "searcher.h"
#include "reader.h"

using namespace Nan;


void InitIndri(v8::Local<v8::Object> exports) {
  Searcher::Init(exports);
  Reader::Init(exports);
}


NODE_MODULE(indri, InitIndri)
