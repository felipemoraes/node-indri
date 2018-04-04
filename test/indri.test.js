'use strict';

var expect = require('chai').expect;

const indri = require('../build/Release/node-indri');

describe('Searcher', function() {

    it('should create Searcher object ', function() {
        var searcher = new indri.Searcher( 
            {"index": "/var/node-indri/etc/poems_index", 
            "rule" : "method:dirichlet,mu:1000",
            "fbTerms": 10,
            "fbMu": 1500, "nameField": "title",
            "resultsPerPage": 1}
        );
        expect(searcher).to.not.be.null;
    });

    it('should not create Searcher object ', function() {
        expect(indri.Searcher).to.throw(Error);
    });

});


