#ifndef SCORER_H
#define SCORER_H

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
    int docid;
    float score;
} ScoredSearchResult;


typedef struct  {
  indri::api::QueryEnvironment* environment;
  indri::api::Parameters parameters;
} ScorerParameters;

class Scorer : public Napi::ObjectWrap<Scorer> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Scorer(const Napi::CallbackInfo& info);
 private:
  static Napi::FunctionReference constructor;
  Napi::Value ScoreDocuments(const Napi::CallbackInfo& info);
  Napi::Value RetrieveTopKScores(const Napi::CallbackInfo& info);

  ScorerParameters parameters_;
  std::string query_; 
  int page_; 
  int results_per_page_;
  std::vector<int> feedback_docs_;
  vector<ScoredSearchResult> results_;

};

vector<ScoredSearchResult> scoreDocuments(ScorerParameters& parameters, std::string query,
    std::vector<int>& docs);

vector<ScoredSearchResult> retrieveTopKScores(ScorerParameters& parameters, std::string query, 
int number_results);

#endif