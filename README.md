# node-indri
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

    wget <url-to-indri-5.11.tar.gz>
    tar xzvf indri-5.11.tar.gz
    cd indri-5.11
    ./configure
    make
    sudo make install

- Finally install node-indri

```
npm install 
```

- To test if everything works:
```
npm test 
```

# Examples

```
var indri =  require('node-indri');

var searcher = new indri.Searcher(indexPath, method);

var results = searcher.search(query, nDocs, relFbDocs);

```


# Implementation details 

node-indri was implemented using Native Abstractions for Node.js (NAN) and C++11. It supports a simple searching function with relevance feedback.

 




