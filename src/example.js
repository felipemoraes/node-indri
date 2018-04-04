const indri = require('../build/Release/node-indri');


var searcher = new indri.Searcher( 
    {"index": "/var/node-indri/etc/poems_index", 
    "rules" : "method:dirichlet,mu:1000",
    "fbTerms": 10,
    "fbMu": 1500, 
    "nameField": "title", 
    "resultsPerPage": 1}
);

var results = searcher.search("school", 2, ["doc3"], function(error, results) {
    if (error) {
      console.log("error");
    }
    console.log(results);
  }
);