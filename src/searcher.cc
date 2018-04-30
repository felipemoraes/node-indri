#include "searcher.h"


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

    if (!info[2]->IsNumber()) {
      return Nan::ThrowError(Nan::New("expected arg 3: number").ToLocalChecked());
    }

    if (!info[3]->IsArray()) {
      return Nan::ThrowError(Nan::New("expected arg 4: array").ToLocalChecked());
    }


    if(!info[4]->IsFunction()) {
      return Nan::ThrowError(Nan::New("expected arg 5: function callback").ToLocalChecked());
    }


    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string query(*arg1, arg1.length());
    
    double page = info[1]->IsUndefined() ? 1 : info[1]->NumberValue();

    double results_per_page = info[2]->IsUndefined() ? 10 : info[2]->NumberValue();

    std::vector<int> fb_docs;

    v8::Local<v8::Array> jsArr = v8::Local<v8::Array>::Cast(info[3]);
    for (int i = 0; i < jsArr->Length(); i++) {
        v8::Local<v8::Value> jsElement = jsArr->Get(i);
        fb_docs.push_back(jsElement->NumberValue());
    } 

    Searcher* obj = ObjectWrap::Unwrap<Searcher>(info.Holder());

    Nan::AsyncQueueWorker(new SearchAsyncWorker(obj->parameters, query, page, results_per_page, fb_docs,
            new Nan::Callback(info[4].As<v8::Function>())
    ));

}