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
    std::unordered_map<std::string,std::string> fields;
    std::string snippet;
    std::string document;
    int docid;
} SearchResult;


typedef struct  {
  std::unordered_map<std::string,std::string> includeFields;
  int results_per_page;
  indri::api::QueryEnvironment* environment;
  indri::query::QueryExpander* expander;
  indri::api::Parameters parameters;
  bool includeDocument;
} SearchParameters;


vector<SearchResult> search(SearchParameters& parameters, std::string query,
    int page, std::vector<int>& feedback_docs);


static Persistent<v8::FunctionTemplate> constructor;


class SearchAsyncWorker : public Nan::AsyncWorker {
private:

  SearchParameters parameters_;
  std::string query_; 
  int page_; 
  std::vector<int> feedback_docs_;
  vector<SearchResult> results_;

public:
  SearchAsyncWorker(SearchParameters& parameters, std::string query, 
        int page, std::vector<int> &feedback_docs, Nan::Callback *callback)
    : Nan::AsyncWorker(callback) {

    this->parameters_ = parameters;
    this->query_ = query;
    this->page_ = page;
    this->feedback_docs_ = feedback_docs;
  }

  void Execute() {
    this->results_ = search(this->parameters_, this->query_, this->page_, this->feedback_docs_);
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;


    v8::Local<v8::Array> resultArray = Nan::New<v8::Array>();


    v8::Local<v8::String> docidKey = Nan::New("docid").ToLocalChecked();
    v8::Local<v8::String> fieldsKey = Nan::New("fields").ToLocalChecked();
    v8::Local<v8::String> snippetKey = Nan::New("snippet").ToLocalChecked();
    v8::Local<v8::String> documentKey = Nan::New("document").ToLocalChecked();


    for(int i = 0; i < this->results_.size(); ++i) {

        SearchResult result = this->results_.at(i);

        v8::Local<v8::Object> jsonObject = Nan::New<v8::Object>();
        v8::Local<v8::Object> fieldsJsonObject = Nan::New<v8::Object>();
        
        for(auto fieldMap : result.fields) {
          v8::Local<v8::String> key = Nan::New(fieldMap.first).ToLocalChecked();
          v8::Local<v8::String> value = Nan::New(fieldMap.second).ToLocalChecked();
          Nan::Set(fieldsJsonObject, key, value);
        }

        v8::Local<v8::Value> docidValue = Nan::New(result.docid);
        v8::Local<v8::Value> snippetValue = Nan::New(result.snippet).ToLocalChecked();

        Nan::Set(jsonObject, docidKey, docidValue);
        Nan::Set(jsonObject, snippetKey, snippetValue);
        Nan::Set(jsonObject, fieldsKey, fieldsJsonObject);

        if (this->parameters_.includeDocument) {
          v8::Local<v8::String> documentValue = Nan::New(result.document).ToLocalChecked();
          Nan::Set(jsonObject, documentKey, documentValue);
        }

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


class Searcher : public Nan::ObjectWrap{

 public:
  static NAN_MODULE_INIT(Init) {
      v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(Searcher::New);
      constructor.Reset(tpl);
      tpl->SetClassName(Nan::New<v8::String>("Searcher").ToLocalChecked());

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      SetPrototypeMethod(tpl, "search", Searcher::Search);

      Set(target, Nan::New<v8::String>("Searcher").ToLocalChecked(), tpl->GetFunction());
  }

  SearchParameters parameters;

  explicit Searcher() {
  }
  
  ~Searcher() {}

  static NAN_METHOD(New) {

    // throw an error if constructor is called without new arguments
    if(!info.IsConstructCall()) {
      return Nan::ThrowError(Nan::New("Searcher::New - called without arguments").ToLocalChecked());
    }

    // expect exactly one argument
    if(info.Length() != 1) {
      return Nan::ThrowError(Nan::New("Searcher::New - expected an object with parameters").ToLocalChecked());
    }

    // expect arguments to be strings
    if(!info[0]->IsObject()) {
      return Nan::ThrowError(Nan::New("Searcher::New - expected arguments to be object").ToLocalChecked());
    }


    v8::Local<v8::Object> jsonObj = info[0]->ToObject();


    v8::Local<v8::String> indexProp = Nan::New("index").ToLocalChecked();
    v8::Local<v8::String> rulesProp = Nan::New("rules").ToLocalChecked();
    v8::Local<v8::String> fbTermsProp = Nan::New("fbTerms").ToLocalChecked();
    v8::Local<v8::String> fbMuProp = Nan::New("fbMu").ToLocalChecked();
    v8::Local<v8::String> fieldsProp = Nan::New("includeFields").ToLocalChecked();
    v8::Local<v8::String> resultsPerPageProp = Nan::New("resultsPerPage").ToLocalChecked();
    v8::Local<v8::String> includeDocumentProp = Nan::New("includeDocument").ToLocalChecked();
    v8::Local<v8::String>  fbDocsProp = Nan::New("fbDocs").ToLocalChecked();

    std::string index = "";

    std::string rule = "";

    std::unordered_map<std::string, std::string> fields;

    bool includeDocument = false;

    int fbTerms = 100;
    int fbMu = 1500;

    int resultsPerPage = 10;

    int fbDocs = -1;

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


    if (Nan::HasOwnProperty(jsonObj, fbTermsProp).FromJust()) {
      v8::Local<v8::Value> fbMuValue = Nan::Get(jsonObj, fbTermsProp).ToLocalChecked();
      fbTerms = fbMuValue->NumberValue();
    }

    if (Nan::HasOwnProperty(jsonObj, fbMuProp).FromJust()) {
      v8::Local<v8::Value> fbMuValue = Nan::Get(jsonObj, fbMuProp).ToLocalChecked();
      fbMu = fbMuValue->NumberValue();
      
    }

    

    if (Nan::HasOwnProperty(jsonObj, fieldsProp).FromJust()) {

      v8::Local<v8::Object> jsonFieldObj = Nan::Get(jsonObj, fieldsProp).ToLocalChecked()->ToObject();

      v8::Local<v8::Array> fieldsProps = Nan::GetPropertyNames(jsonFieldObj).ToLocalChecked();


      for (int i = 0; i < fieldsProps->Length(); i++){
        
        std::string key = std::string(*Nan::Utf8String(fieldsProps->Get(i)->ToString()));
        v8::Local<v8::Value> fieldValue = Nan::Get(jsonFieldObj, fieldsProps->Get(i)).ToLocalChecked();
        std::string value = std::string(*Nan::Utf8String(fieldValue->ToString()));
        fields.emplace(key, value);
      }
    }

    if (Nan::HasOwnProperty(jsonObj, resultsPerPageProp).FromJust()) {
      v8::Local<v8::Value> resultsPerPageValue = Nan::Get(jsonObj, resultsPerPageProp).ToLocalChecked();
      resultsPerPage = resultsPerPageValue->NumberValue();
    }

    if (Nan::HasOwnProperty(jsonObj, includeDocumentProp).FromJust()) {
      v8::Local<v8::Value> includeDocumentValue = Nan::Get(jsonObj, includeDocumentProp).ToLocalChecked();
      includeDocument = includeDocumentValue->BooleanValue();
    }

    if (Nan::HasOwnProperty(jsonObj, fbDocsProp).FromJust()) {
      v8::Local<v8::Value> fbDocsValue = Nan::Get(jsonObj, fbDocsProp).ToLocalChecked();
      fbDocs = fbDocsValue->NumberValue();
      
    }

    parameters->set("fbDocs", fbDocs);
    parameters->set("fbTerms", fbTerms);
    parameters->set("fbMu", fbMu);

    Searcher* obj = new Searcher();

        
    std::vector<std::string> rules;

    rules.push_back(parameters->get("rules", ""));

    indri::api::QueryEnvironment* environment = new indri::api::QueryEnvironment();
    indri::query::QueryExpander* expander;

    environment->setScoringRules(rules);
    
    environment->addIndex(parameters->get("index", ""));

    expander = new indri::query::RMExpander(environment, *parameters);

    

    SearchParameters search_parameters;
    search_parameters.environment = environment;
    search_parameters.expander = expander;
    search_parameters.includeFields = fields;
    search_parameters.includeDocument = includeDocument;
    search_parameters.results_per_page = resultsPerPage;

    

    obj->parameters = search_parameters;

    obj->Wrap(info.This());
    
    info.GetReturnValue().Set(info.This());

  }
  
  static NAN_METHOD(Search);

};