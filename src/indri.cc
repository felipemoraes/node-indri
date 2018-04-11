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
    int page, std::vector<int>& feedback_docs){

    
    indri::api::QueryAnnotation* annotation;

    std::vector<indri::api::ScoredExtentResult> results;
    int resultsPerPage = parameters.results_per_page;
    int numberResults = page*resultsPerPage;
    
    if (feedback_docs.size() != 0) {

        std::vector<indri::api::ScoredExtentResult> score_fb_docs;
        for (int i = 0; i < feedback_docs.size(); i++) {
          indri::api::ScoredExtentResult r(0.0, feedback_docs[i]);
          score_fb_docs.push_back(r);
        }
        std::string expandedQuery = parameters.expander->expand( query, score_fb_docs);
        
        annotation = parameters.environment->runAnnotatedQuery( expandedQuery, numberResults);
        results  = annotation->getResults();
    } else {
      annotation = parameters.environment->runAnnotatedQuery( query, numberResults);
      results  = annotation->getResults();
    }


    std::vector<indri::api::ParsedDocument*> documents;
    std::vector<std::string> docnos;
    std::vector<std::string> titles;

   
    documents = parameters.environment->documents(results);
    

    std::unordered_map<std::string,std::vector<std::string> > fieldsData;

    for (auto fieldMap : parameters.includeFields) {
        auto valueFields = parameters.environment->documentMetadata(results, fieldMap.first);
        fieldsData.emplace(fieldMap.second, valueFields);
    }

    vector<SearchResult> searchResults;

    for(int i = (page-1)*resultsPerPage; i < documents.size(); i++) {
        
        indri::api::SnippetBuilder builder(true);
        
        std::string snippet = builder.build( results[i].document, documents[i], annotation );

        snippet.erase(std::remove(snippet.begin(), snippet.end(), '\n'), snippet.end());

        SearchResult result;
        result.snippet = snippet;
        result.docid = results[i].document;

        if (parameters.includeDocument) {
          result.document =  std::string(documents[i]->text, documents[i]->textLength);
        }


      for (auto fieldMap : fieldsData) {
        auto valueFields = fieldMap.second;
        result.fields.emplace(fieldMap.first, valueFields[i]);
      
      } 


        searchResults.push_back(result);
       
    }
    return searchResults;
}


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

    std::string index = "";

    std::string rule = "";

    std::unordered_map<std::string, std::string> fields;


    int fbTerms = 100;
    int fbMu = 1500;

    int resultsPerPage = 10;

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
      parameters->set("fbTerms", fbTerms);
    }

    if (Nan::HasOwnProperty(jsonObj, fbMuProp).FromJust()) {
      v8::Local<v8::Value> fbMuValue = Nan::Get(jsonObj, fbMuProp).ToLocalChecked();
      fbMu = fbMuValue->NumberValue();
      parameters->set("fbMu", fbMu);
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
    search_parameters.results_per_page = resultsPerPage;

    

    obj->parameters = search_parameters;

    obj->Wrap(info.This());
    
    info.GetReturnValue().Set(info.This());

  }
  
  static NAN_METHOD(Search);

};


NAN_METHOD(Searcher::Search){

    // expect exactly 3 arguments
    if(info.Length() == 3) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected 3 arguments").ToLocalChecked());
    }

    // expect first argument to be string
    if(!info[0]->IsString()) {
      return Nan::ThrowError(Nan::New("expected arg 1: string").ToLocalChecked());
    }


    // expect second argument to be number
    if(!info[1]->IsNumber()) {
      return Nan::ThrowError(Nan::New("expected arg 2: number").ToLocalChecked());
    }

    if (!info[2]->IsArray()) {
      return Nan::ThrowError(Nan::New("expected arg 3: array").ToLocalChecked());
    }

    if(!info[3]->IsFunction()) {
      return Nan::ThrowError(Nan::New("expected arg 4: function callback").ToLocalChecked());
    }


    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string query(*arg1, arg1.length());
    
    double page = info[1]->IsUndefined() ? 10 : info[1]->NumberValue();

    std::vector<int> fb_docs;

    v8::Local<v8::Array> jsArr = v8::Local<v8::Array>::Cast(info[2]);
    for (int i = 0; i < jsArr->Length(); i++) {
        v8::Local<v8::Value> jsElement = jsArr->Get(i);
        fb_docs.push_back(jsElement->NumberValue());
    } 

    Searcher* obj = ObjectWrap::Unwrap<Searcher>(info.Holder());

    Nan::AsyncQueueWorker(new SearchAsyncWorker(obj->parameters, query, page, fb_docs,
            new Nan::Callback(info[3].As<v8::Function>())
    ));

}
  
void InitIndri(v8::Local<v8::Object> exports) {
  Searcher::Init(exports);
}


NODE_MODULE(indri, InitIndri)