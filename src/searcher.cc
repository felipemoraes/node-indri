#include "searcher.h"


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