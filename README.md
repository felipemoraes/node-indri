
node-indri is native Node.js module that integrates Node.js and [Indri](https://www.lemurproject.org/indri.php) search engine 

Please see releases for older versions.

# Setup
These instructions are for Ubuntu and MacOS, but the steps can be adapted for all major platforms with a GCC compiler.

- Install [NodeJS](https://nodejs.org/en/) (at least version 12.0)

- Install [Cmake](https://cmake.org/), [zlib](https://zlib.net/), and [Cmake-js](https://www.npmjs.com/package/cmake-js)
```
sudo apt install cmake zlib1g-dev
sudo npm install cmake-js -g
```

- Clone node-indri
```
git clone https://github.com/felipemoraes/node-indri.git
cd node-indri
```

- Install customized [Indri](https://www.lemurproject.org/indri.php) by Fernando Diaz:
```
git clone https://github.com/diazf/indri.git
cd indri
./configure CXX="g++ -D_GNU_SOURCE=1 -D_GLIBCXX_USE_CXX11_ABI=0" --prefix={NODE_INDRI_PATH}/lib/
make
make install
cd ..
```

- Install node-indri
```
npm install
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



# Citation
If you use node-indri to produce results for your scientific publication, please refer to our [ECIR 2019](https://chauff.github.io/documents/publications/ECIR2019-moraes.pdf) paper.
```
@inproceedings{moraes2019nodeindri,
  title={node-indri: moving the Indri toolkit to the modern Web stack},
  author={Moraes, Felipe and Hauff, Claudia},
  booktitle={ECIR},
  year={2019}
}
```
# License
[MIT](https://opensource.org/licenses/MIT) License


