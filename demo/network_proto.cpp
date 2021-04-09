#include "../src/parser.h"
#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <string>

using namespace std;

/* In this example we build a parser for a simple network protcol a bit like http
 * It is probably easier to read this from the main function and track it back
*/

enum TAG {Configure, Status, Command, Error, Request, Responce, Setup};

struct TagLine {
    TAG tag;
    int statusCode;
};

enum KEY { ContentLength, ContentType, State, ErrorCode, URL};

enum STATE {Start, Stop};

enum ECODE {Full};

typedef variant<int, string, STATE, ECODE> Value;


struct KeyValue {
    KEY key;
    Value value;

};

struct Message {
    struct TagLine tag;
    list<KeyValue> header;
    string body;
    bool hasBody;
};

ParserT<string> EOL = Lit("\r\n");
ParserT<string> assign = Lit(": ");

auto mkContentLength = [](int i){
    Value v;
    v = i;
    return (KeyValue){ContentLength, v};
};

auto mkContentType = [](string i){
    Value v;
    v = i.data();
    return (KeyValue){ContentType, v};
};

auto mkState = [](STATE i){
    Value v;
    v = i;
    return (KeyValue){State, v};
};

auto mkErrorCode = [](ECODE i){
    Value v;
    v = i;
    return (KeyValue){ErrorCode, v};
};

auto mkUrl = [](string i){
    Value v;
    v = i.data();
    return (KeyValue){URL, v};
};

ParserT<string> toEnd = [](string s){
    ParserRet<string> ret;
    int splitPoint = 0;
    for (char c : s) {
        if (c == '\r') break;
        splitPoint++;
    }
    ret = make_tuple(s.substr(0,splitPoint),s.substr(splitPoint,s.length() - splitPoint));
    return ret;
};

ParserT<TagLine> parseTag = ( Lit("CONFIGURE") >> Const((TagLine){Configure, 0})
                            | Lit("STATUS")    >> Const((TagLine){Status,0})
                            | Lit("COMMAND")   >> Const((TagLine){Command,0})
                            | Lit("ERROR")     >> Const((TagLine){Error,0})
                            | Lit("REQUEST")   >> Const((TagLine){Request,0})
                            | Lit("SETUP")     >> Const((TagLine){Setup,0})
                            | fmap<int,TagLine>([](int i){return (TagLine){Responce, i};},Integer)
                            ) << Lit(" XMP/1.0") << EOL;

ParserT<STATE> parseState = Lit("Start") >> Const(Start)
                          | Lit("Stop")  >> Const(Stop)
                          ;

ParserT<ECODE> parseECode = Lit("FULL") >> Const(Full);

ParserT<KeyValue> parseKV = ( fmap<int,      KeyValue>(mkContentLength,Lit("Content-Length") >> assign >> Integer )
                            | fmap<string,   KeyValue>(mkContentType,  Lit("Content-Type")   >> assign >> toEnd)
                            | fmap<STATE,    KeyValue>(mkState      ,  Lit("State")          >> assign >> parseState)
                            | fmap<ECODE,    KeyValue>(mkErrorCode  ,  Lit("Error-Code")     >> assign >> parseECode)
                            | fmap<string,   KeyValue>(mkUrl,          Lit("Url")            >> assign >> toEnd)
                            ) << EOL;

ParserT<Many<KeyValue>> parseHeader = many(parseKV) << EOL;

template<typename T>
std::optional<T> findKV(list<KeyValue> header, KEY key)
{
    for (auto kv : header) {
        if (kv.key == key) return get<T>(kv.value);
    }
    return {};
}

ParserT<Message> parseVMP = [](string s){
    ParserRet<Message> ret;

    auto th = (parseTag & parseHeader)(s);
    if (!th.has_value()) return ret;
    auto [value,rem] = th.value();
    auto[tag,header] = value;
    int cl = -1;
    struct Message msg;
    msg.tag     = tag;
    msg.header  = header;
    msg.body    = "";
    msg.hasBody = false;
    for (auto kv : header) {
        if (kv.key == ContentLength) {
            cl = std::get<int>(kv.value);
            break;
        }
    }
    string newRem;
    if (cl != -1) {
        msg.body = rem.substr(0,cl);
        msg.hasBody = true;
        newRem   = rem.substr(cl, rem.length() - cl);
    } else newRem = rem;
    ret = make_tuple(msg, newRem);
    return ret;
};

int main() {
    const char* msg =
            "CONFIGURE XMP/1.0\r\n"
            "Content-Type: Text\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Hello World";
    auto ret = Run(parseVMP(msg));
    if (!ret.has_value()) exit(-1);
    auto ty = findKV<string>(ret->header, ContentType);
    if (ty.has_value())
        cout << "Content-Type: " << ty.value() << endl;
    cout << ret->body << endl;
    return 0;
}
