<img src="moonshot.svg" height="300px"/>

# Moonshot
An optionally-typed object-oriented language that extends Lua. This project allows you to use your favorite Lua libraries and frameworks with a better organized and error-safe syntax. This is a compiler, so it has no effect on the performance at runtime.

## Todo
- [x] Obtain a grammar for vanilla Lua code
- [x] Build a tokenizer for Lua source
- [x] Write parser structure to consume tokens and validate syntax
- [x] Test vanilla parser vs official Lua distribution
- [x] Write traversal algorithm to validate AST and output Lua code
- [x] Extend the Lua grammar for the new language
- [x] Add new language features to tokenizer, parser and traverser
- [x] Implement a type checker
- [ ] Implement streamed tokenization, combine with parsing step
- [ ] Ensure that strings work in all scenarios, double check memory deallocation
- [ ] Integrate library into command line tool
- [ ] Double check the commenting
- [ ] Add a permissive license
- [ ] Package for deployment
- [ ] Design website
- [ ] Launch

## CLI options
- Organize output files after the input files or compress them into a single file
- Just compile into Lua or also run Lua after compilation
- Input from stdin or a file/directory
- Output code or just validate syntax

## Notes
- [Parts of a compiler](https://cs.lmu.edu/~ray/notes/compilerarchitecture/)
- Lua 5.3 [source code](https://www.lua.org/source/5.3/)
  - [parser](https://www.lua.org/source/5.3/lparser.c.html)
