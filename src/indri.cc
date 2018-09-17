#include <nan.h>
#include "searcher.h"
#include "reader.h"
#include "scorer.h"

using namespace Nan;


void InitIndri(v8::Local<v8::Object> exports) {
  Searcher::Init(exports);
  Reader::Init(exports);
  Scorer::Init(exports);
}


NODE_MODULE(indri, InitIndri)
