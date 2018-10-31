#include "scorer.h"

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

NAN_METHOD(Scorer::ScoreDocuments){

    // expect exactly 3 arguments
    if(info.Length() == 3) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected 3 arguments").ToLocalChecked());
    }

    // expect first argument to be string
    if(!info[0]->IsString()) {
      return Nan::ThrowError(Nan::New("expected arg 1: string").ToLocalChecked());
    }

    // expect second argument to be array
    if (!info[1]->IsArray()) {
      return Nan::ThrowError(Nan::New("expected arg 2: array").ToLocalChecked());
    }

    // expect third argument to be function
    if(!info[2]->IsFunction()) {
      return Nan::ThrowError(Nan::New("expected arg 4: function callback").ToLocalChecked());
    }



    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string query(*arg1, arg1.length());


    v8::Local<v8::Array> jsArr = v8::Local<v8::Array>::Cast(info[1]);
    std::vector<int> docs;
    for (int i = 0; i < jsArr->Length(); i++) {
        v8::Local<v8::Value> jsElement = jsArr->Get(i);
        docs.push_back(jsElement->NumberValue());
    } 

    Scorer* obj = ObjectWrap::Unwrap<Scorer>(info.Holder());

    Nan::AsyncQueueWorker(new ScorerAsyncWorker(obj->parameters, query, docs,
            new Nan::Callback(info[2].As<v8::Function>())
    ));

}




NAN_METHOD(Scorer::RetrieveTopKScores){

    // expect exactly 3 arguments
    if(info.Length() == 3) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected 3 arguments").ToLocalChecked());
    }

    // expect first argument to be string
    if(!info[0]->IsString()) {
      return Nan::ThrowError(Nan::New("expected arg 1: string").ToLocalChecked());
    }

    // expect second argument to be number
    if (!info[1]->IsNumber()) {
      return Nan::ThrowError(Nan::New("expected arg 2: number").ToLocalChecked());
    }

    // expect second argument to be function
    if(!info[2]->IsFunction()) {
      return Nan::ThrowError(Nan::New("expected arg 3: function callback").ToLocalChecked());
    }


    v8::String::Utf8Value arg1(info[0]->ToString());
    
    std::string query(*arg1, arg1.length());

    int number_results = info[1]->IsUndefined() ? 0 : info[1]->NumberValue();

    Scorer* obj = ObjectWrap::Unwrap<Scorer>(info.Holder());

    Nan::AsyncQueueWorker(new ScorerAsyncWorker(obj->parameters, query, number_results,
            new Nan::Callback(info[2].As<v8::Function>())
    ));

}
