# CPP Parser Combinators

This is an experimental C++ library for rapidly building parsers. It is inspired by parser combinators in haskell
such as [attoparsec](https://hackage.haskell.org/package/attoparsec) and, like those libraries, allows
for the construction of fully fledged parsers in  a few lines of code. There are however the following problems:
- It gets a little ugly when you want recursive grammars (but no more ugly than c++ normally is)
- Performance hasn't been tested yet. Not much effort has been put into performance yet
- It requires C++ 17

## Installing
There are no dependencies for building as it is a single header. Simply run `make install`

## Combinators and types
For more info, consult the source. It's only one file and it isn't long.

### Types and results

#### ParserRet
This is the return type for a parser unit.
```cpp
parserRet<T> = std::optional<std::tuple<T, std::string>>
```
Optional means that a parser can fail. The first element of the tuple is what was parsed.
The second tuple is the part of the string that hasn't been consumed.

#### ParserT
The parser type:
```cpp
ParserT<T> = std::function<ParserRet<T>(std::string)>;
```
A parser is a function from a string to a `ParserRet`

#### Many
Just an alias for a list
```cpp
Many<T> = std::list<T>;
```

#### Get
Returns the result of a parse or a default value:
```cpp
T Get(ParserRet<T> result, T def)
```

#### Run
Like `Get` but returns optional values instead of a default value when handeling failue:
```cpp
std::optional<T> Run(ParserRet<T> result)
```

### Sequence

#### A \<\< B
`A << B` created a parser out of the parsers `A` and `B`. The behaviour is the same as
running parser `A` followed by `B`. If either fail, the new parsers failes. Else,
return the result of parser A.
```cpp
ParserT<A> operator<< (const ParserT<A>& l, const ParserT<B>& r)
```
Consider the example that parsers an integer forllowed by a space:
```cpp
Parser<Int> intSpace = Integer << Char(' ');
```

#### A >> B
`A >> B` creates a parser out of the parsers `A` and `B`. The behaviour of the new parser is equivilent
to running `A`. If it fails, we fail. Else we discard the result and run parser B.
```cpp
ParserT<B> operator>> (const ParserT<A>& l, const ParserT<B>& r)
```

#### A | B
This is the alternative combinator. `A | B` creates a parser that out of the parsers
`A` and `B`. The behavior is the same as running `A`. If it succeeds, return the output,
else we return `B`.
```cpp
ParserT<T> operator| (const ParserT<T>& l, const ParserT<T>& r)
```

#### A & B
`A & B` creates a parser out of the parsers `A` and `B`. It means run `A` then `B` storing the output of
both in a tuple.
```cpp
ParserT<std::tuple<A,B>> operator& (const ParserT<A>& l, const ParserT<B>& r)
```

### Modifying Parsers
#### fmap
`fmap(f, A)` means create a new parser from function `f` and parser `A` that is equivilent to applying
the function `f` on the result of running `A`.
```cpp
ParserT<B> fmap(std::function<B(A)> f, const ParserT<A>& r) {
```

#### Tag
`Tag(P, t)` creates a parser out of parser `P` and value `t` that when run,
returns the result of `P` ina tuple with value `t`
```cpp
ParserT<std::tuple<A,T>> Tag(const ParserT<A>& p, T tag)
```

#### Replace
`Replace(P,t)` creates a parser put of parser `P` and value `t` that whe n run,
will replace the output of parser `P` with `t` if successful.

### Lists
#### many
Runs a parser over and over again until it fails.
```cpp
ParserT<Many<T>> many(const ParserT<T> & p)
```
Example: We want to parse a comma seporated list.
```cpp
auto SpaceChars = Char(' ') | Char('\t') | Char('\n') | Char('\r');

auto WhiteSpace = many(SpaceChars);

auto ListItem = // Your parser for a list item

auto InnerList = Char(',') >> WhiteSpace >> ListItem;

auto cons = [](std::tuple<Item,Many<Item>> i) {
  auto [head,tail] = i;
  tail.push_front(head);
  return tail;
};

auto List = fmap<std::tuple<Item,Many<Item>>>(cons, ListItem & many(InnerList));
```

#### many1
same as many, but fails instead of returning the empty list.

### Parser Units
#### False
Parser that always fails
```cpp
ParserT<bool> False
```

#### True
Parser that always succeeds
```cpp
ParserT<bool> True = [](std::string s)
```

#### Not
`Not(P)` creates a parser that fails if `P` succeeds and succeeds if `P` fails.
```cpp
ParserT<bool> Not(const ParserT<T>& p)
```

#### Const
`Const(t)` creates a parser that consumes no input, always succeeds and retuns `t`.
```cpp
ParserT<T> Const(T t)
```

#### Char
Parses the provided character
```cpp
ParserT<char> Char(char c)
```

#### Lit
Parses the provided string literal
```cpp
ParserT<std::string> Lit(std::string lit)
```

#### AnyChar
Parses any character
```cpp
ParserT<char> AnyChar
```

#### Alpha
Parses any Character that is a letter
```cpp
ParserT<char> Alpha
```

#### Special
A set of characters that aren't parses by AnyLit
```cpp
ParserT<char> Special
```

#### AnyLit
Any String that doesn't contain special
```cpp
ParserT<std::string> AnyLit
```

#### DigitC
Parses a single digit into a character
```
ParserT<char> DigitC
```

#### Digit
Parses a single digit to an `int`.
```cpp
ParserT<int> Digit;
```

#### Natural
Parses a natural number
```cpp
ParserT<int> Natural
```

#### Integer
parses an integer
```cpp
ParserT<int> Integer
```
