const indri = require('./node-indri/build/Release/node-indri');

process.env.UV_THREADPOOL_SIZE = 20;

let args = process.argv.slice(2);

let scorer = new indri.Scorer( 
  {"index": args[0], 
  "rules" : "method:dirichlet,mu:2000"}
);


let queries = require(args[1]);

let promises = [];


for ( const i in queries ) {
	
	promises.push(new Promise(function(resolve,reject) {
		scorer.scoreDocuments(queries[i], 1000, function(error, results) {
    			if (error) {
				reject(error);
    			} else {
      				resolve(results);
    			}
		})
	}));
	if (promises.length === 20) {
	    (async ()  => {
	    	await Promise.all(promises)
			.catch((err) => { console.log(err)});
				
 	    })();
	    promises = [];
	}
}

if (promises.length > 0) {
	(async ()  => {
	 await Promise.all(promises)
		.catch((err) => { console.log(err)});
				
 	})();	
}