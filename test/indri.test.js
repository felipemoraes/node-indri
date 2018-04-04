'use strict';

var expect = require('chai').expect;

const indri = require('../build/Release/node-indri');

describe('Searcher', function() {

    it('should create Searcher object ', function() {
        var searcher = new indri.Searcher( 
            {"index": "/var/node-indri/etc/poems_index", 
            "rule" : "method:dirichlet,mu:1000",
            "fbTerms": 10,
            "fbMu": 1500, "titleField": "title",
            "resultsPerPage": 1}
        );
        expect(searcher).to.not.be.null;
    });

    it('should not create Searcher object ', function() {
        expect(indri.Searcher).to.throw(Error);
    });

    it('should return one result', function() {

        var searcher = new indri.Searcher( 
            {"index": "/var/node-indri/etc/poems_index", 
            "rule" : "method:dirichlet,mu:1000",
            "fbTerms": 10,
            "fbMu": 1500, "titleField": "title",
            "resultsPerPage": 10
            }
        );
        var results = searcher.search("school", 1, []);

        expect(results.length).to.be.equal(1);
    });

    it('should return five results', function() {

        var searcher = new indri.Searcher( 
            {"index": "/var/node-indri/etc/poems_index", 
            "rules" : "method:dirichlet,mu:1000",
            "fbTerms": 10,
            "fbMu": 1500, "titleField": "title", "resultsPerPage": 10}
        );
        var results = searcher.search("school", 1, ["doc3"]);
    
        expect(results.length).to.be.equal(5);
    });

});


