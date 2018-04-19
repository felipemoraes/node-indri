
node-indri is native Node.js module that integrates Node.js and [Indri](https://www.lemurproject.org/indri.php) search engine 

# Setup
These instructions are for Ubuntu Linux, but the steps can be adapted for all major platforms with a GCC compiler.

- Install [NodeJS](https://nodejs.org/en/) (at least version 8.0)
```
sudo apt install npm

// Check if node is installed
which node
```

- Install [Cmake](https://cmake.org/) and [Cmake-js](https://www.npmjs.com/package/cmake-js)
```
sudo apt install cmake
sudo npm install cmake-js -g
```

- Install [Indri](https://www.lemurproject.org/indri.php):
```
wget <link-5.11.tar.gz>
tar xzvf indri-5.11.tar.gz
cd indri-5.11
./configure CXX="g++ -D_GNU_SOURCE=1 -D_GLIBCXX_USE_CXX11_ABI=0"
make
sudo make install
```

- Install node-indri

```
npm install --unsafe-perm
```

- To test if everything works:
```
npm test 
```

# Examples


```
const indri =  require('../build/Release/node-indri');
```

## Searcher

```
var searcher = new indri.Searcher(
    {
    "index": "index_path", 
    "rules" : "rules", // method:dirichlet,mu:1000
    "fbTerms": 10,
    "fbMu": 1500, 
    "includeDocument" : true,
    "includeFields": [{ 
        "nameInIndex1": "nameInResponse1", 
        "nameInIndex2": "nameInResponse2"
    }
    ]}
);

searcher.search(query, page, results_per_page, feedback_docs, callback);
```

## Reader 

```

var reader = new indri.Reader("index": "index_path");

reader.getDocument(docid, callback);

```

# Implementation details 

node-indri was implemented using Native Abstractions for Node.js (NAN) and C++11. It supports a simple async searching function with relevance feedback.






