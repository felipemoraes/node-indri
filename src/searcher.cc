#include "searcher.h"

Napi::FunctionReference Searcher::constructor;

Napi::Object Searcher::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Searcher", {
        InstanceMethod("search", &Searcher::Search)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("Searcher", func);
    return exports;
}

Searcher::Searcher(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Searcher>(info)  {
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

    std::unordered_map<std::string, std::string> fields;

    bool includeDocument = false;

    bool includeDocumentScore = false;

    int fbTerms = 100;
    int fbMu = 1500;

    int fbDocs = -1;

    indri::api::Parameters* parameters = new indri::api::Parameters();
  
    if (jsonObj.Has("index")) {
        index = jsonObj.Get("index").As<Napi::String>().Utf8Value();
        parameters->set("index", index);
    }

    if (jsonObj.Has("rules")) {
        rule = jsonObj.Get("rules").As<Napi::String>().Utf8Value();
        parameters->set("rules", index);
    }

    if (jsonObj.Has("fbTerms")) {
        fbTerms = jsonObj.Get("fbTerms").As<Napi::Number>().DoubleValue();
    }

    if (jsonObj.Has("fbMu")) {
        fbMu = jsonObj.Get("fbMu").As<Napi::Number>().DoubleValue();
    }

    if (jsonObj.Has("includeFields")) {
        Napi::Object fieldsObj = jsonObj.Get("includeFields").As<Napi::Object>();
        Napi::Array fieldNames = fieldsObj.GetPropertyNames();
        for (int i = 0; i < fieldNames.Length(); i++){
            Napi::Value keyValue = fieldNames[i];
            std::string key = keyValue.ToString().Utf8Value();
            std::string value = fieldsObj.Get(key).As<Napi::String>().Utf8Value();
            fields.emplace(key, value);
        }
    }

    if (jsonObj.Has("includeDocument")) {
        includeDocument = jsonObj.Get("includeDocument").As<Napi::Boolean>().Value();
    }

    if (jsonObj.Has("includeDocumentScore")) {
        includeDocumentScore = jsonObj.Get("includeDocumentScore").As<Napi::Boolean>().Value();
    }

    if (jsonObj.Has("fbDocs")) {
        fbDocs = jsonObj.Get("fbDocs").As<Napi::Number>().DoubleValue();
    }

    parameters->set("fbDocs", fbDocs);
    parameters->set("fbTerms", fbTerms);
    parameters->set("fbMu", fbMu);
        
    std::vector<std::string> rules;

    rules.push_back(parameters->get("rules", ""));

    indri::api::QueryEnvironment* environment = new indri::api::QueryEnvironment();
    indri::query::QueryExpander* expander;

    environment->setScoringRules(rules);
    
    try {
        environment->addIndex(parameters->get("index", ""));
    } catch (const lemur::api::Exception& e) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    }

    try {
      expander = new indri::query::RMExpander(environment, *parameters);
    }  catch (const lemur::api::Exception& e) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    }
    

    SearchParameters search_parameters;
    search_parameters.environment = environment;
    search_parameters.expander = expander;
    search_parameters.includeFields = fields;
    search_parameters.includeDocument = includeDocument;
    search_parameters.includeDocumentScore = includeDocumentScore;

    this->parameters_ = search_parameters;
}

class SearcherWorker: public Napi::AsyncWorker {
  public:
    SearcherWorker(SearchParameters& parameters, std::string query,
    int page, int  results_per_page, std::vector<int>& feedback_docs, Napi::Function& callback)
      : Napi::AsyncWorker(callback), parameters(parameters), query(query), 
      results_per_page(results_per_page), page(page), feedback_docs(feedback_docs)  {
    }

  private:
    void Execute() {
      this->results = search(parameters, query, page, results_per_page, feedback_docs);
    }

    void OnOK() {
        const Napi::Env env = Env();
        const Napi::HandleScope scope(env);
        Napi::Array js_results = Napi::Array::New(env, this->results.size());
        ToJS(env, parameters, js_results);
        Callback().Call({env.Undefined(), js_results});
    }

    void ToJS(const Napi::Env &env, SearchParameters& parameters, Napi::Array &js_results){
        
        for (int i=0; i<results.size(); i++) {
            Napi::Object obj = Napi::Object::New(env);
            obj.Set("snippet", results[i].snippet);
            obj.Set("docid", results[i].docid);

            if (parameters.includeDocument) {
                obj.Set("document", results[i].document);
            }
            if (parameters.includeDocument) {
                obj.Set("score", results[i].score);
            }

            Napi::Object fields = Napi::Object::New(env);
            for (auto fieldEntry : results[i].fields){
                fields.Set(fieldEntry.first, fieldEntry.second);
            }
            if (results[i].fields.size()) {
                obj.Set("fields", fields);
            }
            js_results[i] = obj;
        }
    }

    SearchParameters parameters;

    vector<SearchResult> results;
    std::string query;
    int page;
    int  results_per_page;
    std::vector<int>& feedback_docs;
};




Napi::Value Searcher::Search(const Napi::CallbackInfo& info) {

    if (info.Length() == 3) {
        throw Napi::TypeError::New(info.Env(), "Searcher::Search - expected 3 arguments");
    }

    // expect first argument to be string
    if(!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "Searcher::Search - expected arg 1: string");
    }


    // expect second argument to be number
    if(!info[1].IsNumber()) {
        throw Napi::TypeError::New(info.Env(), "Searcher::Search - expected arg 2: number");
    }

    if (!info[2].IsNumber()) {
        throw Napi::TypeError::New(info.Env(), "Searcher::Search - expected arg 3: number");
    }

    if (!info[3].IsArray()) {
        throw Napi::TypeError::New(info.Env(), "Searcher::Search - expected arg 4: array");
    }

    if(!info[4].IsFunction()) {
        throw Napi::TypeError::New(info.Env(), "Searcher::Search - expected arg 5: function callback");
    }
    
    std::string query = info[0].As<Napi::String>().Utf8Value();
    
    double page = info[1].As<Napi::Number>().DoubleValue();
    double results_per_page = info[2].As<Napi::Number>().DoubleValue();
    std::vector<int> fb_docs;

    Napi::Array jsArr = info[3].As<Napi::Array>();

    for (int i = 0; i < jsArr.Length(); i++) {
        Napi::Value v = jsArr[i];
        int docid = (int) v.As<Napi::Number>();
        fb_docs.push_back(docid);
    } 

    Napi::Function callback = info[4].As<Napi::Function>();

    SearcherWorker* worker = new SearcherWorker(parameters_, query, page, results_per_page, fb_docs, callback);
    worker->Queue();
    return info.Env().Undefined();
}

vector<SearchResult> search(SearchParameters& parameters, std::string query,
    int page, int  results_per_page, std::vector<int>& feedback_docs){

    indri::api::QueryAnnotation* annotation;
    std::vector<indri::api::ScoredExtentResult> results;
    int numberResults = page*results_per_page;

    vector<SearchResult> searchResults;

    try {
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
    } catch (const lemur::api::Exception& e) {
        return searchResults;
    }


    std::vector<indri::api::ParsedDocument*> documents;
    std::vector<std::string> docnos;
    std::vector<std::string> titles;
   
    documents = parameters.environment->documents(results);
    

    std::unordered_map<std::string,std::vector<std::string> > fieldsData;

    for (auto fieldMap : parameters.includeFields) {
      try {
        auto valueFields = parameters.environment->documentMetadata(results, fieldMap.first);
      
        fieldsData.emplace(fieldMap.second, valueFields);
      } catch (const lemur::api::Exception& e) {
      }
    }

    for(int i = (page-1)*results_per_page; i < documents.size(); i++) {
    	
        indri::api::SnippetBuilder builder(true);
        SearchResult result;
        result.docid = results[i].document;
	    result.score = results[i].score;
        try {

          std::string snippet = builder.build( results[i].document, documents[i], annotation );
          snippet.erase(std::remove(snippet.begin(), snippet.end(), '\n'), snippet.end());
          result.snippet = snippet;
          
          if (parameters.includeDocument) {
            result.document =  std::string(documents[i]->text, documents[i]->textLength);
          }
        } catch (const lemur::api::Exception& e) {

        }

        for (auto fieldMap : fieldsData) {
            auto valueFields = fieldMap.second;
            result.fields.emplace(fieldMap.first, valueFields[i]);
        } 
        searchResults.push_back(result);
       
    }
    return searchResults;
}