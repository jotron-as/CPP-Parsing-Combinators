#ifndef PARSER_CPP
#define PARSER_CPP

#include <functional>
#include <tuple>
#include <list>
#include <string>
#include <optional>

#define UNUSED(x) (void)(x)

// What a parser returns
template<typename T>
using ParserRet = std::optional<std::tuple<T, std::string>>;

// What a parser is
template<typename T>
using ParserT = std::function<ParserRet<T>(std::string)>;

// Get value from parser. If it failed, we provide a default value
template<typename T>
T Get(ParserRet<T> result, T def) {
    if (!result.has_value())
        return def;
    auto[res, rest] = result.value();
    return res;
}

template<typename T>
std::optional<T> Run(ParserRet<T> result) {
    if (!result.has_value())
        return {};
    auto[res, rest] = result.value();
    return res;
}

// A << B means run parser A, run parser B, forget the result of B
template<typename A, typename B>
ParserT<A> operator<< (const ParserT<A>& l, const ParserT<B>& r) {
    ParserT<A> func = [l,r](std::string s){
        ParserRet<A> ret;
        ParserRet<A> temp = l(s);
        if (!temp.has_value())
            return ret;
        auto[p,rest] = temp.value();
        ParserRet<B> temp1 = r(rest);
        if (!temp1.has_value())
            return ret;
        auto[p1,rest1] = temp1.value(); UNUSED(p1);
        ret = std::make_tuple(p,rest1);
        return ret;
    };
    return func;
}

// A >> B means run parser A, forget result, run parser B
template<typename A, typename B>
ParserT<B> operator>> (const ParserT<A>& l, const ParserT<B>& r) {
    ParserT<B> func = [l,r](std::string s){
        ParserRet<B> ret;
        ParserRet<A> temp = l(s);
        if (!temp.has_value())
            return ret;
        auto[p,rest] = temp.value(); UNUSED(p);
        return r(rest);
    };
    return func;
}

// A | B means run parser A. If it fails, run parser B
template<typename T>
ParserT<T> operator| (const ParserT<T>& l, const ParserT<T>& r)
{
    return [l, r](std::string s) {
        ParserRet<T> ret;
        ret = l(s);
        if (ret.has_value()) return ret;
        return r(s);
    };
}

// Cartesian Product
template<typename A, typename B>
ParserT<std::tuple<A,B>> operator& (const ParserT<A>& l, const ParserT<B>& r)
{
    return [l, r](std::string s) {
        ParserRet<std::tuple<A,B>> ret = {};
        auto ret1 = l(s);
        if (!ret1.has_value())
            return ret;
        auto[res1,str1] = ret1.value();
        auto ret2 = r(str1);
        if (!ret2.has_value())
             return ret;
        auto[res2,str2] = ret2.value();
        ret = std::make_tuple(std::make_tuple(res1, res2), str2);
        return ret;
    };
}

// fmap(f,p) creates a parse from p, but run function f on result
template<typename A, typename B>
ParserT<B> fmap(std::function<B(A)> f, const ParserT<A>& r) {
    return [f,r](std::string s){
        ParserRet<B> ret = {};
        auto t = r(s);
        if (t.has_value()) {
            auto[a,b] = t.value();
            ret = std::make_tuple(f(a), b);
        }
        return ret;
    };
}

template<typename T>
using Many = std::list<T>;

// Parse as much as we can with parser p
template<typename T>
ParserT<Many<T>> many(const ParserT<T> & p) {
    return [p](std::string s) {
        ParserRet<Many<T>> ret = {};
        Many<T> parsed;
        while(true) {
            auto innerRet = p(s);
            if (!innerRet.has_value()) {
                break;
            }
            auto[p,r] = innerRet.value();
            parsed.push_back(p);
            s = r;
        }
        ret = std::make_tuple(parsed, s);
        return ret;
    };
}

// same as many but fails instead of empty list
template<typename T>
ParserT<Many<T>> many1(const ParserT<T> & p) {
    return [p](std::string s) {
        ParserRet<Many<T>> ret = {};
        auto t = many(p)(s);
        if (!t.has_value()) return ret;
        auto[r1,r2] = t.value(); UNUSED(r2);
        if (r1.empty()) return ret;
        return t;
    };
}

// Add a tag to parser result
template<typename A, typename T>
ParserT<std::tuple<A,T>> Tag(const ParserT<A>& p, T tag) {
    return fmap<A, std::tuple<A,T>>([tag](A x){return std::make_tuple(x,tag);}, p);
}

// Replace Parser result with something
template<typename A, typename T>
ParserT<T> Replace(const ParserT<A>& p, T tag) {
    return fmap<A, T>([tag](A){return tag;}, p);
}

// Fail
ParserT<bool> False = [](std::string){
    ParserRet<bool> ret = {};
    return ret;
};

// True
ParserT<bool> True = [](std::string s){
    ParserRet<bool> ret;
    ret = std::make_tuple(true, s);
    return ret;
};

template<typename T>
ParserT<bool> Not(const ParserT<T>& p) {
    return [p](std::string s) {
        return (p(s).has_value() ? False(s) : True(s));
    };
}

template<typename T>
ParserT<T> Const(T t){
    return [t](std::string s) {
    ParserRet<T> ret;
    ret = std::make_tuple(t,s);
    return ret;
};
}

// Parse the specific character c
ParserT<char> Char(char c) {
    return [c](std::string s) {
        ParserRet<char> ret = {};
        if (s.length() == 0)
            return ret;
        if (s[0] == c)
            ret = std::make_tuple(c,s.substr(1, s.size()-1));
        return ret;
    };
}

// Parse the specific string lit
ParserT<std::string> Lit(std::string lit) {
    if (lit.empty())
        return Replace(True,lit);
    auto p = Char(lit[0]);
    for(int i = 1; static_cast<uint>(i) < lit.length(); i++)
        p = p >> Char(lit[i]);
    return Replace(p,lit);
}

// Parse any character
ParserT<char> AnyChar = [](std::string s) {
    ParserRet<char> ret = {};
    if (s.empty()) return ret;
    ret = std::make_tuple(s[0],s.substr(1, s.size()-1));
    return ret;
};

// Parse any character that is a letter
ParserT<char> Alpha = [](std::string s) {
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
ParserT<std::string> AnyLit = [](std::string s) {
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


#endif