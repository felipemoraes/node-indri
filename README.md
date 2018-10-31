
node-indri is native Node.js module that integrates Node.js and [Indri](https://www.lemurproject.org/indri.php) search engine 

# Setup
These instructions are for Ubuntu Linux, but the steps can be adapted for all major platforms with a GCC compiler.

- Install [NodeJS](https://nodejs.org/en/) (at least version 8.0)
```
sudo apt install npm

// Check if node is installed
which node
```

- Install [Cmake](https://cmake.org/), [zlib](https://zlib.net/), and [Cmake-js](https://www.npmjs.com/package/cmake-js)
```
sudo apt install cmake zlib1g-dev
sudo npm install cmake-js -g
```

- Install [Indri](https://www.lemurproject.org/indri.php):
```
wget https://sourceforge.net/projects/lemur/files/lemur/indri-5.11/indri-5.11.tar.gz
tar xzvf indri-5.11.tar.gz
cd indri-5.11
./configure CXX="g++ -D_GNU_SOURCE=1 -D_GLIBCXX_USE_CXX11_ABI=0"
make
sudo make install
cd ..
```

- Install node-indri

```
git clone https://github.com/felipemoraes/node-indri.git
cd node-indri
npm install --unsafe-perm
```

- To test if everything works:
```
npm test 
```

# Examples
Require the native module that was built during the setup. You can find it at the path below in the node-indri folder, usually you want to copy the module into the project that you are using it in and adjust the path accordingly.

```
const indri =  require('./build/Release/node-indri');
```

## Searcher
Use the Searcher to execute a search with optional relevance feedback. Pass an empty list as `feedback_docs` to use no relevance feedback.

```
const searcher = new indri.Searcher(
    {
    "index": "index_path", 
    "rules" : "rules", // method:dirichlet,mu:1000
    "fbTerms": 10,
    "fbMu": 1500, 
    "includeDocument" : true,
    "includeDocumentScore" : true, 
    "includeFields": [{ 
        "nameInIndex1": "nameInResponse1", 
        "nameInIndex2": "nameInResponse2"
    }
    ]}
);

searcher.search(query, page, results_per_page, feedback_docs, callback);
```

## Scorer
Use the scorer to retrieve score for a list of documents `scoreDocuments` or the top K retrieved documents and their scores with  `retrieveTopKScores`.

```
const scorer = new indri.Scorer(
    {
    "index": "index_path", 
    "rules" : "rules", // method:dirichlet,mu:1000
    "fbTerms": 10,
    "fbMu": 1500}
);

scorer.scoreDocuments(query, page, docs, callbac_k);
scorer.retrieveTopKScores(query, page, number_results, callback);

```

## Reader 
Use the Reader to retrieve an individual document by id.

```
const reader = new indri.Reader("index": "index_path");

reader.getDocument(docid, callback);
```

# Benchmark

Benchmark files used to compare `node-indri` and `pyndri` can be folder inside folder `benchmark`.

# Implementation details 

node-indri was implemented using Native Abstractions for Node.js (NAN) and C++11. It supports a simple async searching function with relevance feedback.






