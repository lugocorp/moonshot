<img src="moonshot.svg" height="300px"/>

# Moonshot
Moonshot is an optionally-typed object-oriented extension language for Lua. This project allows you to use your favorite Lua libraries and frameworks with a more organized and error-safe code base. Moonshot is lightweight, the command line tool compiles to under 20KB. It gives you full interop with vanilla Lua as well as a fully developed optional typing system.

## Todo
- [x] Obtain a grammar for vanilla Lua code
- [x] Build a tokenizer for Lua source
- [x] Write parser structure to consume tokens and validate syntax
- [x] Test vanilla parser vs official Lua distribution
- [x] Write traversal algorithm to validate AST and output Lua code
- [x] Extend the Lua grammar for the new language
- [x] Add new language features to tokenizer, parser and traverser
- [x] Implement a type checker
- [x] Integrate library into command line tool
- [x] Ensure that strings work in all scenarios
- [x] Double check memory deallocation
- [x] Buff up class/interface relationships
- [x] Fix tuple function return statements getting confused between lhs's and real tuples
- [x] Implement constructors and ancestor functions in the output
- [ ] Comment the code
- [ ] Analyze parser and traversal patterns, compress and finalize deallocation
- [ ] Refactor validation check systems to be more well organized
- [ ] Finish plugging up memory leaks

## Post-beta release updates
- [ ] Give AST nodes a line number for better traversal stage error messages
- [ ] Add a permissive license and package for deployment
- [ ] Constructor chaining
