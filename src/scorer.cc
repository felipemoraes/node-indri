#include "scorer.h"

vector<ScoredSearchResult> score(ScorerParameters& parameters, std::string query, std::vector<int>& docs, int number_results){

    std::vector<indri::api::ScoredExtentResult> results;


    vector<ScoredSearchResult> searchResults;

    try {
        
      if (docs.size() != 0) {
          std::vector<lemur::api::DOCID_T> docids;
          for (int i = 0; i < docs.size(); i++) {
            docids.push_back(docs[i]);
          }
          
          results = parameters.environment->runQuery(query, docids, docs.size());
      }  else {
          results = parameters.environment->runQuery(query, number_results);
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



NAN_METHOD(Scorer::Score){

    // expect exactly 3 arguments
    if(info.Length() == 3) {
      return Nan::ThrowError(Nan::New("Searcher::Search - expected 3 arguments").ToLocalChecked());
    }

    // expect first argument to be string
    if(!info[0]->IsString()) {
      return Nan::ThrowError(Nan::New("expected arg 1: string").ToLocalChecked());
    }

    if (!info[1]->IsArray()) {
      return Nan::ThrowError(Nan::New("expected arg 2: array").ToLocalChecked());
    }

    if (!info[2]->IsNumber()) {
      return Nan::ThrowError(Nan::New("expected arg 3: number").ToLocalChecked());
    }


    if(!info[3]->IsFunction()) {
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

    int number_results = info[2]->IsUndefined() ? 10 : info[2]->NumberValue();

    Scorer* obj = ObjectWrap::Unwrap<Scorer>(info.Holder());

    Nan::AsyncQueueWorker(new ScoreAsyncWorker(obj->parameters, query, docs, number_results,
            new Nan::Callback(info[3].As<v8::Function>())
    ));

}
