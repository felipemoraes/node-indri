#include <indri/CompressedCollection.hpp>
#include <indri/DiskIndex.hpp>
#include <indri/KrovetzStemmer.hpp>
#include <indri/QueryEnvironment.hpp>
#include <indri/QueryParserFactory.hpp>
#include <indri/QuerySpec.hpp>
#include <indri/Path.hpp>
#include <indri/Porter_Stemmer.hpp>
#include <indri/RMExpander.hpp>
#include <indri/SnippetBuilder.hpp>

#include <cmath>
#include <nan.h>
#include <string>
#include <algorithm>
#include <iostream>

using namespace Nan;


static Persistent<v8::FunctionTemplate> constructor;

class Searcher : public Nan::ObjectWrap {

 public:
  static NAN_MODULE_INIT(Init) {
      v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Searcher::New);
      constructor.Reset(tpl);
      tpl->SetClassName(Nan::New<v8::String>("Searcher").ToLocalChecked());

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      SetPrototypeMethod(tpl, "search", Searcher::Search);

      Set(target, Nan::New<v8::String>("Searcher").ToLocalChecked(), tpl->GetFunction());
  }

  std::string index_path_, background_model_;

 private:

  explicit Searcher(std::string index_path, std::string background_model)
    : index_path_(index_path), background_model_(background_model) {

    }
  
  ~Searcher() {}

  indri::api::QueryEnvironment _environment;
  indri::api::QueryAnnotation* _annotation;

  static NAN_METHOD(New) {

    // throw an error if constructor is called without new arguments
    if(!info.IsConstructCall()) {
      return Nan::ThrowError(Nan::New("Searcher::New - called without arguments").ToLocalChecked());
    }

    // expect exactly 2 arguments
    if(info.Length() != 2) {
      return Nan::ThrowError(Nan::New("Searcher::New - expected arguments index_path and method").ToLocalChecked());
    }

    // expect arguments to be strings
    if(!info[0]->IsString() || !info[1]->IsString()) {
      return Nan::ThrowError(Nan::New("Searcher::New - expected arguments to be strings").ToLocalChecked());
    }

    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string index_path(*arg1, arg1.length());

    v8::String::Utf8Value arg2(info[1]->ToString());
    
    std::string method(*arg2, arg2.length());
    
    Searcher* obj = new Searcher(index_path, method);
    std::vector<std::string> rules;
    rules.push_back(method);

    obj->_environment.setScoringRules(rules);
    obj->_environment.addIndex(index_path);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());

  }
  
  static NAN_METHOD(Search) ;
  
};

NAN_METHOD(Searcher::Search){

    // expect exactly 2 arguments
    if(info.Length() < 2) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected arguments query, n_request").ToLocalChecked());
    }

    // expect first argument to be string
    if(!info[0]->IsString()) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected first argument to be a string").ToLocalChecked());
    }


    // expect second argument to be number
    if(!info[1]->IsNumber()) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected second argument to be a number").ToLocalChecked());
    }


    std::vector<lemur::api::DOCID_T> docids;

    Searcher* obj = ObjectWrap::Unwrap<Searcher>(info.Holder());

    if (!info[2]->IsUndefined()){

      if (!info[2]->IsArray()) {
        return Nan::ThrowError(Nan::New("Searcher::Search - expected third argument to be an array").ToLocalChecked());
      }

      std::vector<std::string> rel_fb_docs;
      v8::Local<v8::Array> jsArr = v8::Local<v8::Array>::Cast(info[2]);

      for (int i = 0; i < jsArr->Length(); i++) {
        v8::Local<v8::Value> jsElement = jsArr->Get(i);
        v8::String::Utf8Value str(jsElement->ToString());
        std::string doc (*str,str.length());
        rel_fb_docs.push_back(doc);
      } 
      if (rel_fb_docs.size()>0) {
        docids = obj->_environment.documentIDsFromMetadata("docno", rel_fb_docs);
      }
    }


    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string query(*arg1, arg1.length());
    
    double n_request = info[1]->IsUndefined() ? 10 : info[1]->NumberValue();

    
    obj->_annotation = obj->_environment.runAnnotatedQuery( query, n_request);
    std::vector<indri::api::ScoredExtentResult> results  = obj->_annotation->getResults();
    std::vector<indri::api::ParsedDocument*> documents;
    std::vector<std::string> documentNames;
    std::vector<std::string> headlines;

    documents = obj->_environment.documents(results);
    documentNames = obj->_environment.documentMetadata( results, "docno" );
    headlines = obj->_environment.documentMetadata( results, "headline" );

    v8::Local<v8::Array> jsArr = Nan::New<v8::Array>(documents.size());
    
    for(int i = 0; i < documents.size(); i++) {
        indri::api::SnippetBuilder builder(true);
        
        std::string snippet = builder.build( results[i].document, documents[i], obj->_annotation );

        snippet.erase(std::remove(snippet.begin(), snippet.end(), '\n'), snippet.end());
        v8::Local<v8::Object> jsonObject = Nan::New<v8::Object>();

        v8::Local<v8::String> docnoKey = Nan::New("docno").ToLocalChecked();
        v8::Local<v8::String> titleKey = Nan::New("title").ToLocalChecked();
        v8::Local<v8::String> snippetKey = Nan::New("snippet").ToLocalChecked();

        v8::Local<v8::Value> docnoValue = Nan::New(documentNames[i]).ToLocalChecked();
        v8::Local<v8::Value> tittleValue = Nan::New(headlines[i]).ToLocalChecked();
        v8::Local<v8::Value> snippetValue = Nan::New(snippet).ToLocalChecked();

        Nan::Set(jsonObject, docnoKey, docnoValue);
        Nan::Set(jsonObject, titleKey, tittleValue);
        Nan::Set(jsonObject, snippetKey, snippetValue);

        jsArr->Set(i, jsonObject);
       
    }

    info.GetReturnValue().Set(jsArr);

}

  
void InitIndri(v8::Local<v8::Object> exports) {
  Searcher::Init(exports);
}


NODE_MODULE(indri, InitIndri)