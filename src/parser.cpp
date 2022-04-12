#include "parser.h"


ParserT<bool> False = [](std::string_view){
    ParserRet<bool> ret = {};
    return ret;
};

ParserT<bool> True = [](std::string_view s){
    ParserRet<bool> ret;
    ret = std::make_tuple(true, s);
    return ret;
};

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

// Parse any character
ParserT<char> AnyChar = [](std::string_view s) {
    ParserRet<char> ret = {};
    if (s.empty()) return ret;
    ret = std::make_tuple(s[0],s.substr(1, s.size()-1));
    return ret;
};

// Parse any character that is a letter
ParserT<char> Alpha = [](std::string_view s) {
    ParserRet<char> ret = {};
    if (s.length() == 0)
        return ret;
    if (!isalpha(s[0]) || s[0] == ' ')
        return ret;
    char c = s[0];
    ret = std::make_tuple(c,s.substr(1, s.size()-1));
    return ret;
};

ParserT<char> Special = Char(' ') | Char('\n') | Char('\t')
                      | Char('=') | Char('-');

// Parse any string (that isn't a special character)
ParserT<std::string> AnyLit = [](std::string_view s) {
    ParserRet<std::string> ret = {};
    auto r1 = many1(Not(Special) >> AnyChar)(s);
    if (!r1.has_value()) return ret;
    auto[listC, r] = r1.value();
    std::string str(listC.size(), '\0');
    int i = 0;
    for(char c : listC) str[i++] = c;
    ret = std::make_tuple(str,r);
    return ret;
};

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

ParserT<std::string> Take(int len) {
  int i = len;
  auto pred = [i](char) mutable -> bool {i--; return i >= 0;};
  return TakeWhile(pred);
}

ParserT<char> DigitC = Char('0') | Char('1') | Char('2')
                     | Char('3') | Char('4') | Char('5')
                     | Char('6') | Char('7') | Char('8')
                     | Char('9');
auto char2Int = [](char c){
    return c - '0';
};

ParserT<int> Digit = fmap<char,int>(char2Int, DigitC);

auto mkInt = [](Many<int> ints){
    int rec = 0;
    for(int i : ints)
        rec = (10 * rec) + i;
    return rec;
};

ParserT<int> Natural = fmap<Many<int>,int>(mkInt, many1(Digit));

ParserT<int> Integer = Char('-') >> fmap<int,int>([](int i){return -1 * i;}, Natural)
                     | Natural;
