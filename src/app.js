const indri = require('../build/Release/node-indri');

var searcher = new indri.Searcher("/var/node-indri/Aquaint-Index/", "method:dirichlet,mu:1000");

console.log(searcher.search("piracy", 10));
