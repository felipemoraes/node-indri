const indri = require('./build/Release/node-indri.node');

var searcher = new indri.Searcher( 
    {"index": "./etc/poems_index", 
    "rules" : "method:dirichlet,mu:1000",
    "fbTerms": 10,
    "fbMu": 1500, 
    "includeFields": { "title": "headline", "docno": "docno"},
    "includeDocument" : true}
);

console.log('indri', searcher);


module.exports = indri;
