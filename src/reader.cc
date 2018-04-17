#include "reader.h"

Document get_document(indri::collection::CompressedCollection* collection, int docid){
    Document doc;
    doc.docid = docid;


    indri::api::ParsedDocument* document = collection->retrieve(docid);
    doc.text = document->text;
       
        
    for( size_t i=0; i<document->metadata.size(); i++ ) {
        if( document->metadata[i].key[0] == '#' )
            continue;
        doc.fields.emplace(document->metadata[i].key, (const char*) document->metadata[i].value);
    }

    return doc;
}


NAN_METHOD(Reader::GetDocument){

       // expect exactly 2 arguments
    if(info.Length() != 2) {
      return Nan::ThrowError(Nan::New("Reader::getDocument - expected 2 arguments").ToLocalChecked());
    }

    // expect first argument to be number
    if(!info[0]->IsNumber()) {
      return Nan::ThrowError(Nan::New("expected arg 1: number").ToLocalChecked());
    }


    if(!info[1]->IsFunction()) {
      return Nan::ThrowError(Nan::New("expected arg 2: function callback").ToLocalChecked());
    }
    
    int docid = info[0]->IsUndefined() ? -1 : info[0]->NumberValue();

   
    Reader* obj = ObjectWrap::Unwrap<Reader>(info.Holder());

    Nan::AsyncQueueWorker(new DocAsyncWorker(obj->collection_, docid,
            new Nan::Callback(info[1].As<v8::Function>())
    ));

}