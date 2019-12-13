#include "scorer.h"

Napi::FunctionReference Scorer::constructor;

Napi::Object Scorer::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Scorer", {
        InstanceMethod("scoreDocuments", &Scorer::ScoreDocuments),
        InstanceMethod("retrieveTopKScores", &Scorer::RetrieveTopKScores)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("Scorer", func);
    return exports;
}

Scorer::Scorer(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Scorer>(info)  {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();
    if (length != 1) {
        Napi::TypeError::New(env, "Only one argument expected").ThrowAsJavaScriptException();
    }

    if(!info[0].IsObject()){
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    }

    Napi::Object jsonObj = info[0].As<Napi::Object>();
  

    std::string index = "";

    std::string rule = "";

    indri::api::Parameters* parameters = new indri::api::Parameters();
  
    if (jsonObj.Has("index")) {
        index = jsonObj.Get("index").As<Napi::String>().Utf8Value();
        parameters->set("index", index);
    }

    if (jsonObj.Has("rules")) {
        rule = jsonObj.Get("rules").As<Napi::String>().Utf8Value();
        parameters->set("rules", index);
    }

        
    std::vector<std::string> rules;

    rules.push_back(parameters->get("rules", ""));

    indri::api::QueryEnvironment* environment = new indri::api::QueryEnvironment();

    environment->setScoringRules(rules);
    
    try {
        environment->addIndex(parameters->get("index", ""));
    } catch (const lemur::api::Exception& e) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    }


    ScorerParameters scorer_parameters;
    scorer_parameters.environment = environment;

    this->parameters_ = scorer_parameters;

}

class ScorerWorker: public Napi::AsyncWorker {
  public:
    ScorerWorker(ScorerParameters& parameters, std::string query, std::vector<int> &docs, int number_results, 
    Napi::Function& callback)
      : Napi::AsyncWorker(callback), parameters_(parameters), query_(query), docs_(docs) ,
      number_results_(number_results) {
    }

  private:
    void Execute() {
        if (this->docs_.size() == 0) {
            this->results_ = retrieveTopKScores(this->parameters_, this->query_, this->number_results_);
        } else {
            this->results_ = scoreDocuments(this->parameters_, this->query_, this->docs_);
        }
    }

    void OnOK() {
        const Napi::Env env = Env();
        const Napi::HandleScope scope(env);
        Napi::Array js_results = Napi::Array::New(env, this->results_.size());
        ToJS(env, js_results);
        Callback().Call({env.Undefined(), js_results});
    }

    void ToJS(const Napi::Env &env, Napi::Array &js_results){
        for (int i=0; i<results_.size(); i++) {
            Napi::Object obj = Napi::Object::New(env);
            obj.Set("docid", results_[i].docid);
            obj.Set("score", results_[i].score);
            js_results[i] = obj;
        }
    }

    ScorerParameters parameters_;
    std::string query_; 
    std::vector<int> docs_;
    vector<ScoredSearchResult> results_;
    int number_results_;
};




Napi::Value Scorer::ScoreDocuments(const Napi::CallbackInfo& info) {

    if (info.Length() != 3) {
        throw Napi::TypeError::New(info.Env(), "Scorer::ScoreDocuments - expected 3 arguments");
    }

    // expect first argument to be string
    if(!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "Scorer::ScoreDocuments - expected arg 1: string");
    }

    // expect second argument to be number
    if(!info[1].IsArray()) {
        throw Napi::TypeError::New(info.Env(), "Scorer::ScoreDocuments - expected arg 2: array");
    }

    if(!info[2].IsFunction()) {
        throw Napi::TypeError::New(info.Env(), "Scorer::ScoreDocuments - expected arg 3: function callback");
    }
    
    std::string query = info[0].As<Napi::String>().Utf8Value();

    std::vector<int> docs;

    Napi::Array jsArr = info[1].As<Napi::Array>();

    for (int i = 0; i < jsArr.Length(); i++) {
        Napi::Value v = jsArr[i];
        int docid = (int) v.As<Napi::Number>();
        docs.push_back(docid);
    } 

    Napi::Function callback = info[2].As<Napi::Function>();

    ScorerWorker* worker = new ScorerWorker(parameters_, query, docs, docs.size(), callback);
    worker->Queue();
    return info.Env().Undefined();
}

Napi::Value Scorer::RetrieveTopKScores(const Napi::CallbackInfo& info) {

    if (info.Length() != 3) {
        throw Napi::TypeError::New(info.Env(), "Scorer::RetrieveTopKScores - expected 3 arguments");
    }

  // expect first argument to be string
    if(!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "Scorer::RetrieveTopKScores - expected arg 1: string");
    }

    // expect second argument to be number
    if(!info[1].IsNumber()) {
        throw Napi::TypeError::New(info.Env(), "Scorer::RetrieveTopKScores - expected arg 2: number");
    }

    if(!info[2].IsFunction()) {
        throw Napi::TypeError::New(info.Env(), "Scorer::RetrieveTopKScores - expected arg 3: function callback");
    }
    
    std::string query = info[0].As<Napi::String>().Utf8Value();
    
    double number_results = info[1].As<Napi::Number>().DoubleValue();
    std::vector<int> docs;

    Napi::Function callback = info[2].As<Napi::Function>();

    ScorerWorker* worker = new ScorerWorker(parameters_, query, docs, number_results, callback);
    worker->Queue();
    return info.Env().Undefined();
}


vector<ScoredSearchResult> scoreDocuments(ScorerParameters& parameters, std::string query, std::vector<int>& docs){

    std::vector<indri::api::ScoredExtentResult> results;


    vector<ScoredSearchResult> searchResults;

    try {
        
      if (docs.size() != 0) {
          std::vector<lemur::api::DOCID_T> docids;
          for (int i = 0; i < docs.size(); i++) {
            docids.push_back(docs[i]);
          }
          results = parameters.environment->runQuery(query, docids, docs.size());
      }  
    } catch (const lemur::api::Exception& e) {
        return searchResults;
    }

    for(int i = 0; i < results.size(); i++) {
        ScoredSearchResult r;
        r.score = results[i].score;
        r.docid = results[i].document;
	    searchResults.push_back(r);
    }

    return searchResults;
}


vector<ScoredSearchResult> retrieveTopKScores(ScorerParameters& parameters, std::string query, int number_results){

    std::vector<indri::api::ScoredExtentResult> results;


    vector<ScoredSearchResult> searchResults;

    try {
      results = parameters.environment->runQuery(query, number_results);
    } catch (const lemur::api::Exception& e) {
        return searchResults;
    }

    for(int i = 0; i < results.size(); i++) {
        ScoredSearchResult r;
        r.score = results[i].score;
        r.docid = results[i].document;
	    searchResults.push_back(r);
    }

    return searchResults;
}