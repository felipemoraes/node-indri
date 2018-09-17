const indri = require('../build/Release/node-indri');


var searcher = new indri.Searcher( 
    {"index": "./etc/poems_index", 
    "rules" : "method:dirichlet,mu:1000",
    "fbTerms": 10,
    "fbMu": 1500, 
    "includeFields": { "title": "headline", "docno": "docno"},
    "includeDocument" : true}
);

var results = searcher.search("school", 1, 10, [2], function(error, results) {
    if (error) {
      console.log("error");
    }
    console.log(results);
  }
);


var reader = new indri.Reader("./etc/poems_index")

reader.getDocument(1,  function(error, data) {
     if (error) {
       console.log("error");
     }
      console.log(data);
  }
);


var scorer = new indri.Scorer( 
  {"index": "./etc/poems_index", 
  "rules" : "method:dirichlet,mu:1000"}
);

scorer.score("summer school", [], 2, function(error, results) {
    if (error) {
      console.log("error");
    } else {
      console.log(results);
    }
})