#include "../src/parser.h"
#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <string>

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

typedef std::variant<int, std::string, STATE, ECODE> Value;


struct KeyValue {
    KEY key;
    Value value;

};

struct Message {
    struct TagLine tag;
    std::list<KeyValue> header;
    std::string body;
    bool hasBody;
};

ParserT<std::string> EOL = Lit("\r\n");
ParserT<std::string> assign = Lit(": ");

auto mkContentLength = [](int i){
    Value v;
    v = i;
    return (KeyValue){ContentLength, v};
};

auto mkContentType = [](std::string i){
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

auto mkUrl = [](std::string i){
    Value v;
    v = i.data();
    return (KeyValue){URL, v};
};

auto toEnd = takeWhile([](char c){ return c != '\r';});

ParserT<TagLine> parseTag = ( Lit("CONFIGURE") >> Const((TagLine){Configure, 0})
                            | Lit("STATUS")    >> Const((TagLine){Status,0})
                            | Lit("COMMAND")   >> Const((TagLine){Command,0})
                            | Lit("ERROR")     >> Const((TagLine){Error,0})
                            | Lit("REQUEST")   >> Const((TagLine){Request,0})
                            | Lit("SETUP")     >> Const((TagLine){Setup,0})
                            | fmap<int,TagLine>([](int i){return (TagLine){Responce, i};},Natural)
                            ) << Lit(" XMP/1.0") << EOL;

ParserT<STATE> parseState = Lit("Start") >> Const(Start)
                          | Lit("Stop")  >> Const(Stop)
                          ;

ParserT<ECODE> parseECode = Lit("FULL") >> Const(Full);

ParserT<KeyValue> parseKV = ( fmap<int,         KeyValue>(mkContentLength,Lit("Content-Length") >> assign >> Natural)
                            | fmap<std::string, KeyValue>(mkContentType,  Lit("Content-Type")   >> assign >> toEnd)
                            | fmap<STATE,       KeyValue>(mkState      ,  Lit("State")          >> assign >> parseState)
                            | fmap<ECODE,       KeyValue>(mkErrorCode  ,  Lit("Error-Code")     >> assign >> parseECode)
                            | fmap<std::string, KeyValue>(mkUrl,          Lit("Url")            >> assign >> toEnd)
                            ) << EOL;

ParserT<Many<KeyValue>> parseHeader = many(parseKV) << EOL;

template<typename T>
std::optional<T> findKV(std::list<KeyValue> header, KEY key)
{
    for (auto kv : header) {
        if (kv.key == key) return std::get<T>(kv.value);
    }
    return {};
}

ParserT<Message> parseXMP = [](std::string_view s){
    ParserRet<Message> ret;

    auto th = (parseTag & parseHeader)(s);
    if (!th.has_value()) return ret;
    auto [value,rem] = th.value();
    auto[tag,header] = value;
    int cl = -1;
    Message msg {tag, header, "", findKV<int>(header, ContentLength).value_or(0) > 0};
    for (auto kv : header) {
        if (kv.key == ContentLength) {
            cl = std::get<int>(kv.value);
            break;
        }
    }
    std::string newRem;
    if (cl != -1) {
        msg.body = rem.substr(0,cl);
        msg.hasBody = true;
        newRem   = rem.substr(cl, rem.length() - cl);
    } else newRem = rem;
    ret = make_tuple(msg, newRem);
    return ret;
};

int main() {
    const std::string msg =
            "CONFIGURE XMP/1.0\r\n"
            "Content-Type: Text\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Hello World";
    std::string_view sv{msg};
    auto ret = Run(parseXMP(sv));
    if (!ret.has_value()) exit(-1);
    auto ty = findKV<std::string>(ret->header, ContentType);
    auto cl = findKV<int>(ret->header, ContentLength);
    if (ty.has_value())
        std::cout << "Content-Type: " << ty.value() << std::endl;
    if (cl.has_value())
        std::cout << "Content-Length: " << cl.value() << std::endl;
    std::cout << ret->body << std::endl;
    return 0;
}
