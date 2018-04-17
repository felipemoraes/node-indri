#ifndef READER_H
#define READER_H

#include "indri/Repository.hpp"
#include "indri/CompressedCollection.hpp"


#include <cmath>
#include <nan.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <iostream>

using namespace Nan;



typedef struct  {
    std::unordered_map<std::string,std::string> fields;
    std::string text;
    int docid;
} Document;


Document get_document(indri::collection::CompressedCollection* collection, int docid);


static Persistent<v8::FunctionTemplate> reader_constructor;

class DocAsyncWorker : public Nan::AsyncWorker {
private:
  indri::collection::CompressedCollection* collection_;
  int docid_;
  Document document_;

public:
  DocAsyncWorker(indri::collection::CompressedCollection* collection, int docid, Nan::Callback *callback)
    : Nan::AsyncWorker(callback) {

    this->collection_ = collection;
    this->docid_ = docid;
  }

  void Execute() {
    this->document_ = get_document(this->collection_, this->docid_);
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;


    v8::Local<v8::String> docidKey = Nan::New("docid").ToLocalChecked();
    v8::Local<v8::String> fieldsKey = Nan::New("fields").ToLocalChecked();
    v8::Local<v8::String> documentKey = Nan::New("document").ToLocalChecked();

    v8::Local<v8::Object> jsonObject = Nan::New<v8::Object>();
    v8::Local<v8::Object> fieldsJsonObject = Nan::New<v8::Object>();
    
    for(auto fieldMap : this->document_.fields) {
        v8::Local<v8::String> key = Nan::New(fieldMap.first).ToLocalChecked();
        v8::Local<v8::String> value = Nan::New(fieldMap.second).ToLocalChecked();
        Nan::Set(fieldsJsonObject, key, value);
    }

    v8::Local<v8::Value> docidValue = Nan::New(this->document_.docid);

    v8::Local<v8::String> documentValue = Nan::New(this->document_.text).ToLocalChecked();

    Nan::Set(jsonObject, docidKey, docidValue);

    Nan::Set(jsonObject, documentKey, documentValue);

    Nan::Set(jsonObject, fieldsKey, fieldsJsonObject);
    

    v8::Local<v8::Value> argv[] = {
      Nan::Null(), // no error occured
      jsonObject
    };
    callback->Call(2, argv);
  }

  void HandleErrorCallback() {
    Nan::HandleScope scope;
    v8::Local<v8::Value> argv[] = {
      Nan::New(this->ErrorMessage()).ToLocalChecked(), // return error message
      Nan::Null()
    };
    callback->Call(2, argv);
  }
};


class Reader : public Nan::ObjectWrap{

 public:
  static NAN_MODULE_INIT(Init) {
      v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Reader::New);
      reader_constructor.Reset(tpl);
      tpl->SetClassName(Nan::New<v8::String>("Reader").ToLocalChecked());

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      SetPrototypeMethod(tpl, "getDocument", Reader::GetDocument);

      Set(target, Nan::New<v8::String>("Reader").ToLocalChecked(), tpl->GetFunction());
  }

  indri::collection::CompressedCollection* collection_;

  explicit Reader() {
  }
  
  ~Reader() {}

  static NAN_METHOD(New) {

    // throw an error if constructor is called without new arguments
    if(!info.IsConstructCall()) {
      return Nan::ThrowError(Nan::New("Reader::New - called without arguments").ToLocalChecked());
    }

    if(info.Length() != 1) {
      return Nan::ThrowError(Nan::New("Reader::New - expected 1 argument").ToLocalChecked());
    }


    // expect first argument to be string
    if(!info[0]->IsString()) {
      return Nan::ThrowError(Nan::New("Reader::New expected arg 1: string").ToLocalChecked());
    }


    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string index(*arg1, arg1.length());
    

    Reader* obj = new Reader();
    
    
    indri::collection::Repository* r = new indri::collection::Repository;

    r->openRead(index);

    indri::collection::CompressedCollection* collection = r->collection();
    obj->collection_ = collection;

    obj->Wrap(info.This());
    
    info.GetReturnValue().Set(info.This());

  }
  static NAN_METHOD(GetDocument);
};



#endif