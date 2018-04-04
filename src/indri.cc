#include <indri/QueryEnvironment.hpp>
#include <indri/RMExpander.hpp>
#include <indri/SnippetBuilder.hpp>


#include <cmath>
#include <nan.h>
#include <string>
#include <algorithm>
#include <iostream>

using namespace Nan;

typedef struct  {
    std::string docno;
    std::string name;
    std::string snippet;
} SearchResult;

vector<SearchResult> search(indri::api::Parameters& parameters, std::string query, 
        int page, std::vector<std::string> rel_fb_docs){

    
    indri::api::QueryEnvironment environment;
    indri::api::QueryAnnotation* annotation;
    indri::query::QueryExpander* expander;

    
    std::vector<std::string> rules;
    rules.push_back(parameters.get("rules", ""));
    environment.setScoringRules(rules);
    
    environment.addIndex(parameters.get("index"));

    expander = new indri::query::RMExpander( &environment, parameters);
    std::vector<lemur::api::DOCID_T> docids;

    if (rel_fb_docs.size()>0) {
      docids = environment.documentIDsFromMetadata("docno", rel_fb_docs);
    }

    std::vector<indri::api::ScoredExtentResult> results;
    int resultsPerPage = parameters.get("resultsPerPage", 10);
    int numberResults = page*resultsPerPage;
    
    if (docids.size() != 0) {
        std::vector<indri::api::ScoredExtentResult> fbDocs;
        for (int i = 0; i < docids.size(); i++) {
          indri::api::ScoredExtentResult r(0.0, docids[i]);
          fbDocs.push_back(r);
        }

        std::string expandedQuery = expander->expand( query, fbDocs);
        annotation = environment.runAnnotatedQuery( expandedQuery, numberResults);
        results  = annotation->getResults();
    } else {
      annotation = environment.runAnnotatedQuery( query, numberResults);
      results  = annotation->getResults();
    }

    //v8::Local<v8::Array> jsArr1 = Nan::New<v8::Array>(5);  


    std::vector<indri::api::ParsedDocument*> documents;
    std::vector<std::string> docnos;
    std::vector<std::string> names;

    documents = environment.documents(results);
    docnos = environment.documentMetadata( results, "docno" );
    names = environment.documentMetadata(results, parameters.get("nameField", ""));

    vector<SearchResult> searchResults;

    for(int i = (page-1)*resultsPerPage; i < documents.size(); i++) {
        
        indri::api::SnippetBuilder builder(true);
        
        std::string snippet = builder.build( results[i].document, documents[i], annotation );

        snippet.erase(std::remove(snippet.begin(), snippet.end(), '\n'), snippet.end());

        SearchResult result;
        result.name = names[i];
        result.docno = docnos[i];
        result.snippet = snippet;

        searchResults.push_back(result);
       
    }

    return searchResults;
}


static Persistent<v8::FunctionTemplate> constructor;


class SearchAsyncWorker : public Nan::AsyncWorker {
private:

  indri::api::Parameters parameters_;
  std::string query_; 
  int page_; 
  std::vector<std::string> rel_fb_docs_;
  vector<SearchResult> results_;

public:
  SearchAsyncWorker(indri::api::Parameters& parameters, std::string query, 
        int page, std::vector<std::string> &rel_fb_docs, Nan::Callback *callback)
    : Nan::AsyncWorker(callback) {

    this->parameters_ = parameters;
    this->query_ = query;
    this->page_ = page;
    this->rel_fb_docs_ = rel_fb_docs;
  }

  void Execute() {
    this->results_ = search(this->parameters_, this->query_, this->page_, this->rel_fb_docs_);
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;


    v8::Local<v8::Array> resultArray = Nan::New<v8::Array>();

    for(int i = 0; i < this->results_.size(); ++i) {
        SearchResult result = this->results_.at(i);
        v8::Local<v8::Object> jsonObject = Nan::New<v8::Object>();

        v8::Local<v8::String> docnoKey = Nan::New("url").ToLocalChecked();
        v8::Local<v8::String> nameKey = Nan::New("name").ToLocalChecked();
        v8::Local<v8::String> snippetKey = Nan::New("snippet").ToLocalChecked();

        v8::Local<v8::Value> docnoValue = Nan::New(result.docno).ToLocalChecked();
        v8::Local<v8::Value> nameValue = Nan::New(result.name).ToLocalChecked();
        v8::Local<v8::Value> snippetValue = Nan::New(result.snippet).ToLocalChecked();

        Nan::Set(jsonObject, docnoKey, docnoValue);
        Nan::Set(jsonObject, nameKey, nameValue);
        Nan::Set(jsonObject, snippetKey, snippetValue);
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

  indri::api::Parameters parameters;

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
    v8::Local<v8::String> nameFieldProp = Nan::New("nameField").ToLocalChecked();
    v8::Local<v8::String> resultsPerPageProp = Nan::New("resultsPerPage").ToLocalChecked();

    std::string index = "";

    std::string rule = "";

    std::string nameField = "title";

    int fbTerms = 100;
    int fbMu = 1500;

    int resultsPerPage = 10;

    indri::api::Parameters parameters;
  
    if (Nan::HasOwnProperty(jsonObj, indexProp).FromJust()) {
      v8::Local<v8::Value> indexValue = Nan::Get(jsonObj, indexProp).ToLocalChecked();
      index = std::string(*Nan::Utf8String(indexValue->ToString()));
      parameters.set("index", index);
    }


    if (Nan::HasOwnProperty(jsonObj, rulesProp).FromJust()) {
      v8::Local<v8::Value> rulesValue = Nan::Get(jsonObj, rulesProp).ToLocalChecked();
      rule = std::string(*Nan::Utf8String(rulesValue->ToString()));
      parameters.set("rules", rule);
    }


    if (Nan::HasOwnProperty(jsonObj, fbTermsProp).FromJust()) {
      v8::Local<v8::Value> fbMuValue = Nan::Get(jsonObj, fbTermsProp).ToLocalChecked();
      fbTerms = fbMuValue->NumberValue();
      parameters.set("fbTerms", fbTerms);
    }

    if (Nan::HasOwnProperty(jsonObj, fbMuProp).FromJust()) {
      v8::Local<v8::Value> fbMuValue = Nan::Get(jsonObj, fbMuProp).ToLocalChecked();
      fbMu = fbMuValue->NumberValue();
      parameters.set("fbMu", fbMu);
    }

    if (Nan::HasOwnProperty(jsonObj, nameFieldProp).FromJust()) {
      v8::Local<v8::Value> nameValue = Nan::Get(jsonObj, nameFieldProp).ToLocalChecked();
      nameField = std::string(*Nan::Utf8String(nameValue->ToString()));
      parameters.set("nameField", nameField);
    }

    if (Nan::HasOwnProperty(jsonObj, resultsPerPageProp).FromJust()) {
      v8::Local<v8::Value> resultsPerPageValue = Nan::Get(jsonObj, resultsPerPageProp).ToLocalChecked();
      resultsPerPage = resultsPerPageValue->NumberValue();
      parameters.set("resultsPerPage", resultsPerPage);
    }
    
    Searcher* obj = new Searcher();

    obj->parameters = parameters;

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

    std::vector<std::string> rel_fb_docs;

    v8::Local<v8::Array> jsArr = v8::Local<v8::Array>::Cast(info[2]);
    for (int i = 0; i < jsArr->Length(); i++) {
        v8::Local<v8::Value> jsElement = jsArr->Get(i);
        v8::String::Utf8Value str(jsElement->ToString());
        std::string doc (*str,str.length());
        rel_fb_docs.push_back(doc);
    } 

    Searcher* obj = ObjectWrap::Unwrap<Searcher>(info.Holder());

    // starting the async worker
    Nan::AsyncQueueWorker(new SearchAsyncWorker(obj->parameters, query, page, rel_fb_docs,
            new Nan::Callback(info[3].As<v8::Function>())
    ));

}
  
void InitIndri(v8::Local<v8::Object> exports) {
  Searcher::Init(exports);
}


NODE_MODULE(indri, InitIndri)