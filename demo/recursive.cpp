#include "../src/parser.h"
#include <iostream>
#include <optional>
#include <tuple>
#include <string>
#include <string_view>

/* This is a seriously ugly demo of recursive parsing rules!
 * If I work out how to get this kind of recursion working without long captures
 * you will be the first to know!
*/

int main() {
    ParserT<int> Expr;
    ParserT<int> Term;
    ParserT<int> Factor;

    // ss = skip space
    ParserT<Many<char>> ss = many(Char(' '));

    auto Mult = [&Factor,&Term,&ss](std::string_view s){
        return ((Factor << ss << Char('*')) & (ss >> Term))(s);};

    auto Plus = [&Term,&Expr,&ss](std::string_view s){
        return ((Term << ss << Char('+')) & (ss >> Expr))(s);};

    auto mult = [](std::tuple<int,int> t) {
      return std::get<0>(t) * std::get<1>(t);
    };

    auto plus = [](std::tuple<int,int> t) {
      return std::get<0>(t) + std::get<1>(t);
    };

  Factor = [&Expr,&ss](std::string_view s){
      return ( Char('(') >> ss >> Expr << ss << Char(')')
             | Integer
             )(s);
  };
  
  Term = [&Factor, &mult, Mult](std::string_view s) {
      return ( fmap<std::tuple<int,int>,int>(mult, Mult)
             | Factor
             )(s);
  };

  Expr = [&plus,&Plus,&Term](std::string_view s) {
      return (fmap<std::tuple<int,int>,int>(plus, Plus)
       | Term
       )(s);
  };

  std::cout << "This is a simple repl with the follwong gramar:" << std::endl;
  std::cout << "\texpr ::= term + expr | term" << std::endl;
  std::cout << "\tterm ::= factor * term | factor" << std::endl;
  std::cout << "\tfactor ::= (expr) | 0 | 1 | ..." << std::endl << std::endl;

  std::cout << "Enter and expression and it shall be evaluated" << std::endl;

  for (std::string line; std::getline(std::cin, line);) {
    std::string_view sv {line};
    auto x = Run(Expr(sv));
    if (!x.has_value()) std::cout << "Some Error" << std::endl;
    else std::cout << "--> " << x.value() << std::endl;
  }
  return 0;
}
