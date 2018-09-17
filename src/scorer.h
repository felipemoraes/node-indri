
#ifndef SCORER_H
#define SCORER_H

#include <indri/QueryEnvironment.hpp>
#include <indri/Parameters.hpp>
#include <indri/RMExpander.hpp>
#include <indri/SnippetBuilder.hpp>


#include <cmath>
#include <nan.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <iostream>

using namespace Nan;


typedef struct  {
    int docid;
    float score;
} ScoredSearchResult;


typedef struct  {
  indri::api::QueryEnvironment* environment;
  indri::api::Parameters parameters;
} ScorerParameters;


vector<ScoredSearchResult> score(ScorerParameters& parameters, std::string query,
    std::vector<int>& docs, int number_results);

static Persistent<v8::FunctionTemplate> scorer_constructor;

class ScoreAsyncWorker : public Nan::AsyncWorker {
private:

  ScorerParameters parameters_;
  std::string query_; 
  int number_results_;
  std::vector<int> docs_;
  vector<ScoredSearchResult> results_;

public:
  ScoreAsyncWorker(ScorerParameters& parameters, std::string query, std::vector<int> &docs, 
  int number_results, Nan::Callback *callback): Nan::AsyncWorker(callback) {

    this->parameters_ = parameters;
    this->query_ = query;
    this->number_results_ = number_results;
    this->docs_ = docs;
  }

  void Execute() {
    this->results_ = score(this->parameters_, this->query_, this->docs_, this->number_results_);
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;


    v8::Local<v8::Array> resultArray = Nan::New<v8::Array>();


    v8::Local<v8::String> docidKey = Nan::New("docid").ToLocalChecked();
    v8::Local<v8::String> documentScoreKey = Nan::New("score").ToLocalChecked();

    for(int i = 0; i < this->results_.size(); ++i) {

        ScoredSearchResult result = this->results_.at(i);

        v8::Local<v8::Object> jsonObject = Nan::New<v8::Object>();
        v8::Local<v8::Object> fieldsJsonObject = Nan::New<v8::Object>();
    

        v8::Local<v8::Value> docidValue = Nan::New(result.docid);
        v8::Local<v8::Value> documentScoreValue = Nan::New(result.score);
        
        Nan::Set(jsonObject, docidKey, docidValue);
	    Nan::Set(jsonObject, documentScoreKey, documentScoreValue);
	    
        resultArray->Set(i, jsonObject);
    }

    v8::Local<v8::Value> argv[] = {
      Nan::Null(), // no error occured
      resultArray
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


class Scorer : public Nan::ObjectWrap{

 public:
  static NAN_MODULE_INIT(Init) {
      v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Scorer::New);
      scorer_constructor.Reset(tpl);
      tpl->SetClassName(Nan::New<v8::String>("Scorer").ToLocalChecked());

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      SetPrototypeMethod(tpl, "score", Scorer::Score);

      Set(target, Nan::New<v8::String>("Scorer").ToLocalChecked(), tpl->GetFunction());
  }

  ScorerParameters parameters;

  explicit Scorer() {
  }
  
  ~Scorer() {}

  static NAN_METHOD(New) {

    // throw an error if constructor is called without new arguments
    if(!info.IsConstructCall()) {
      return Nan::ThrowError(Nan::New("Scorer::New - called without arguments").ToLocalChecked());
    }

    // expect exactly one argument
    if(info.Length() != 1) {
      return Nan::ThrowError(Nan::New("Scorer::New - expected an object with parameters").ToLocalChecked());
    }

    // expect arguments to be strings
    if(!info[0]->IsObject()) {
      return Nan::ThrowError(Nan::New("Scorer::New - expected arguments to be object").ToLocalChecked());
    }


    v8::Local<v8::Object> jsonObj = info[0]->ToObject();


    v8::Local<v8::String> indexProp = Nan::New("index").ToLocalChecked();
    v8::Local<v8::String> rulesProp = Nan::New("rules").ToLocalChecked();

    std::string index = "";

    std::string rule = "";

   

    indri::api::Parameters* parameters = new indri::api::Parameters();
  
    if (Nan::HasOwnProperty(jsonObj, indexProp).FromJust()) {
      v8::Local<v8::Value> indexValue = Nan::Get(jsonObj, indexProp).ToLocalChecked();
      index = std::string(*Nan::Utf8String(indexValue->ToString()));
      parameters->set("index", index);
    }

    if (Nan::HasOwnProperty(jsonObj, rulesProp).FromJust()) {
      v8::Local<v8::Value> rulesValue = Nan::Get(jsonObj, rulesProp).ToLocalChecked();
      rule = std::string(*Nan::Utf8String(rulesValue->ToString()));
      parameters->set("rules", rule);
    }
    

   

    Scorer* obj = new Scorer();
        
    std::vector<std::string> rules;

    rules.push_back(parameters->get("rules", ""));

    indri::api::QueryEnvironment* environment = new indri::api::QueryEnvironment();
    indri::query::QueryExpander* expander;

    environment->setScoringRules(rules);
    
    try {
      environment->addIndex(parameters->get("index", ""));
    } catch (const lemur::api::Exception& e) {
      return Nan::ThrowError(Nan::New("Scorer::New could not open index").ToLocalChecked());
    }


    

    ScorerParameters scorer_parameters;
    scorer_parameters.environment = environment;


    obj->parameters = scorer_parameters;

    obj->Wrap(info.This());
    
    info.GetReturnValue().Set(info.This());

  }
  
  static NAN_METHOD(Score);

};

#endif
