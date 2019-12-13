#ifndef READER_H
#define READER_H

#include <napi.h>

#include <indri/Repository.hpp>
#include <indri/CompressedCollection.hpp>


#include <cmath>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <iostream>

typedef struct  {
    std::unordered_map<std::string,std::string> fields;
    std::string text;
    int docid;
} Document;

class Reader : public Napi::ObjectWrap<Reader> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Reader(const Napi::CallbackInfo& info);
 private:
  static Napi::FunctionReference constructor;
  Napi::Value GetDocument(const Napi::CallbackInfo& info);

  indri::collection::CompressedCollection* collection_;
  int docid_;
  Document document_;
  
};


Document get_document(indri::collection::CompressedCollection* collection, int docid);


#endif