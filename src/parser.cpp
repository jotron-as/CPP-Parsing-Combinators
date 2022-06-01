#include "parser.h"


ParserT<char> Char(char c) {
    return [c](std::string_view s) {
        ParserRet<char> ret = {};
        if (s.length() == 0)
            return ret;
        if (s[0] == c)
            ret = std::make_tuple(c,s.substr(1, s.size()-1));
        return ret;
    };
}

ParserT<std::string> Lit(std::string_view lit) {
    if (lit.empty())
        return Const(std::string(lit));
    auto p = Char(lit[0]);
    for(int i = 1; static_cast<unsigned int>(i) < lit.length(); i++)
        p = p >> Char(lit[i]);
    return Replace(p,std::string(lit));
}


ParserT<char> Satisfy(std::function<bool(char)> pred) {
  return [pred](std::string_view s) {
    ParserRet<char> ret = {};
    if (s.size() == 0) return ret;
    char c = s[0];
    if (!pred(c)) return ret;
    ret = std::make_tuple(c,s.substr(1, s.size()-1));
    return ret;
  };
}

ParserT<std::string> TakeWhile(std::function<bool(char)> pred) {
  return [pred](std::string_view s) {
    ParserRet<std::string> ret = {};
    auto result = many(Satisfy(pred))(s);
    auto [str, rest] = result.value();
    if (!result.has_value()) return ret;
    int len = str.size();
    ret = std::make_tuple(std::string(s.substr(0,len)), rest);
    return ret;
  };
}


ParserT<std::string> Take1While(std::function<bool(char)> pred) {
  return [pred](std::string_view s) {
    ParserRet<std::string> ret = {};
    auto result = many(Satisfy(pred))(s);
    if (!result.has_value()) return ret;
    auto [str, rest] = result.value();
    int len = str.size();
    if (len == 0) return ret;
    ret = std::make_tuple(std::string(s.substr(0,len)), rest);
    return ret;
  };
}


ParserT<std::string> Take(int len) {
  int i = len;
  auto pred = [i](char) mutable -> bool {i--; return i >= 0;};
  return TakeWhile(pred);
}
