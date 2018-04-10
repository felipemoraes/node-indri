const indri = require('../build/Release/node-indri');


var searcher = new indri.Searcher( 
    {"index": "./etc/poems_index", 
    "rules" : "method:dirichlet,mu:1000",
    "fbTerms": 10,
    "fbMu": 1500, 
    "includeFields": { "title": "headline", "docno": "docno"},
    "includeDocument" : true,
    "resultsPerPage": 10}
);

var results = searcher.search("school", 1, [2], function(error, results) {
    if (error) {
      console.log("error");
    }
    console.log(results);
  }
);