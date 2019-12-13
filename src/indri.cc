#include <napi.h>

#include <indri/QueryEnvironment.hpp>
#include <indri/Parameters.hpp>
#include <indri/RMExpander.hpp>
#include <indri/SnippetBuilder.hpp>

#include "searcher.h"
#include "reader.h"
#include "scorer.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  Searcher::Init(env,exports);
  Reader::Init(env,exports);
  Scorer::Init(env,exports);
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)
