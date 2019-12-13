#include "reader.h"


Napi::FunctionReference Reader::constructor;

Napi::Object Reader::Init(Napi::Env env, Napi::Object exports) {
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Reader", {
        InstanceMethod("getDocument", &Reader::GetDocument)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("Reader", func);
    return exports;
}

Reader::Reader(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Reader>(info)  {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();
    if (length != 1) {
        Napi::TypeError::New(env, "Reader::New - expected 1 argument").ThrowAsJavaScriptException();
    }

    if(!info[0].IsString()){
        Napi::TypeError::New(env, "Reader::New expected arg 1: string").ThrowAsJavaScriptException();
    }

    std::string index = info[0].As<Napi::String>().Utf8Value();
    
    indri::collection::Repository* r = new indri::collection::Repository;
    
    try {
      r->openRead(index);
    } catch (const lemur::api::Exception& e) {
        Napi::TypeError::New(env, "Reader::New cannot open index").ThrowAsJavaScriptException();
    }
    

    indri::collection::CompressedCollection* collection = r->collection();
    this->collection_ = collection;
}

class ReaderWorker: public Napi::AsyncWorker {
  public:
    ReaderWorker(indri::collection::CompressedCollection* collection, int docid, Napi::Function& callback)
      : Napi::AsyncWorker(callback), collection_(collection), docid_(docid) {
    }

  private:
    void Execute() {
      this->document_ = get_document(collection_, docid_);
    }

    void OnOK() {
        const Napi::Env env = Env();
        const Napi::HandleScope scope(env);
        Napi::Object js_result = Napi::Object::New(env);
        ToJS(env, js_result);
        Callback().Call({env.Undefined(), js_result});
    }

    void ToJS(const Napi::Env &env, Napi::Object &js_result){
        js_result.Set("document", document_.text);
        js_result.Set("docid", document_.docid);

        Napi::Object fields = Napi::Object::New(env);
        for (auto fieldEntry : document_.fields){
            fields.Set(fieldEntry.first, fieldEntry.second);
        }
        if (document_.fields.size()) {
            js_result.Set("fields", fields);
        }
    }

    indri::collection::CompressedCollection* collection_;
    int docid_;
    Document document_;
};

Napi::Value Reader::GetDocument(const Napi::CallbackInfo& info) {

    if (info.Length() != 2) {
        throw Napi::TypeError::New(info.Env(), "Reader::GetDocument - expected 2 arguments");
    }

    // expect first argument to be string
    if(!info[0].IsNumber()) {
        throw Napi::TypeError::New(info.Env(), "Reader::GetDocument - expected arg 1: number");
    }


    if(!info[1].IsFunction()) {
        throw Napi::TypeError::New(info.Env(), "Reader::GetDocument - expected arg 2: function callback");
    }
    
    double docid = info[0].As<Napi::Number>().DoubleValue();
    
    Napi::Function callback = info[1].As<Napi::Function>();

    ReaderWorker* worker = new ReaderWorker(collection_, docid, callback);
    worker->Queue();
    return info.Env().Undefined();
}

Document get_document(indri::collection::CompressedCollection* collection, int docid){
    Document doc;
    doc.docid = docid;

    try {
        indri::api::ParsedDocument* document = collection->retrieve(docid);
        doc.text = document->text;
        
            
        for( size_t i=0; i<document->metadata.size(); i++ ) {
            if( document->metadata[i].key[0] == '#' )
                continue;
            doc.fields.emplace(document->metadata[i].key, (const char*) document->metadata[i].value);
        }
    } catch (const lemur::api::Exception& e) {

    }

    return doc;
}