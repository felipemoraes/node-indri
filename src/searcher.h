#ifndef SEARCHER_H
#define SEARCHER_H

#include <napi.h>

#include <indri/QueryEnvironment.hpp>
#include <indri/Parameters.hpp>
#include <indri/RMExpander.hpp>
#include <indri/SnippetBuilder.hpp>


#include <cmath>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <iostream>

typedef struct  {
    std::unordered_map<std::string,std::string> fields;
    std::string snippet;
    std::string document;
    int docid;
    float score;
} SearchResult;


typedef struct  {
  std::unordered_map<std::string,std::string> includeFields;
  indri::api::QueryEnvironment* environment;
  indri::query::QueryExpander* expander;
  indri::api::Parameters parameters;
  bool includeDocument;
  bool includeDocumentScore;
} SearchParameters;

class Searcher : public Napi::ObjectWrap<Searcher> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Searcher(const Napi::CallbackInfo& info);
 private:
  static Napi::FunctionReference constructor;
  Napi::Value Search(const Napi::CallbackInfo& info);

  SearchParameters parameters_;
  std::string query_; 
  int page_; 
  int results_per_page_;
  std::vector<int> feedback_docs_;
  vector<SearchResult> results_;

};


vector<SearchResult> search(SearchParameters& parameters, std::string query,
    int page, int  results_per_page, std::vector<int>& feedback_docs);

#endif