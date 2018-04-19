'use strict';

var expect = require('chai').expect;

const indri = require('../build/Release/node-indri');

describe('Searcher', function() {

    it('should create Searcher object ', function() {
        var searcher = new indri.Searcher( 
            {"index": "./etc/poems_index", 
            "rules" : "method:dirichlet,mu:1000",
            "fbTerms": 10,
            "fbMu": 1500, 
            "includeFields": { "title": "headline"},
            "includeDocument" : true}
        );
        expect(searcher).to.not.be.null;
    });

    it('should not create Searcher object ', function() {
        expect(indri.Searcher).to.throw(Error);
    });

});




describe('Reader', function() {

    it('should create Reader object ', function() {
        var reader = new indri.Reader("./etc/poems_index");
        expect(reader).to.not.be.null;
    });

    it('should not create Reader object ', function() {
        expect(indri.Reader).to.throw(Error);
    });

});

