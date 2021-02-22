# Optionally-typed Lua wrapper language
An optionally-typed object-oriented language that compiles to Lua. This project allows you to use your favorite Lua libraries and frameworks with a better organized and error-safe syntax. This is a compiler, so it has no effect on the performance at runtime.

## Todo
- [x] Obtain a grammar for vanilla Lua code
- [x] Build a tokenizer for Lua source
- [x] Write parser structure to consume tokens and validate syntax
- [x] Test vanilla parser vs official Lua distribution
- [ ] Figure out recursive parsing with operators
- [ ] Write AST traversal to consume tokens and output Lua code
- [ ] Extend the Lua grammar for the new language
- [ ] Modify tokenizer for new keywords
- [ ] Create additional AST nodes as needed for new syntax
- [ ] Finalize the error system for throwing multiple parsing errors at once
- [ ] Consider lazy tokenizing while parsing (fully streamed approach)
- [ ] Unit test generated code
- [ ] Provide options for code output
  - Generate one vs multiple files
  - Remove comments vs leave them
- [ ] Integrate library into command line tool
- [ ] Double check the commenting
- [ ] Add a permissive license
- [ ] Package for deployment
- [ ] Design website
- [ ] Launch

## Notes
- Lua 5.3 [source code](https://www.lua.org/source/5.3/)
  - [parser](https://www.lua.org/source/5.3/lparser.c.html)

Base Lua Grammar
----------

	block: { stat* }

	stat:
            `Do{ stat* }
          | `Set{ {lhs+} {expr+} }                    -- lhs1, lhs2... = e1, e2...
          | `While{ expr block }                      -- while e do b end
          | `Repeat{ block expr }                     -- repeat b until e
          | `If{ (expr block)+ block? }               -- if e1 then b1 [elseif e2 then b2] ... [else bn] end
          | `Fornum{ ident expr expr expr? block }    -- for ident = e, e[, e] do b end
          | `Forin{ {ident+} {expr+} block }          -- for i1, i2... in e1, e2... do b end
          | `Local{ {ident+} {expr+}? }               -- local i1, i2... = e1, e2...
          | `Localrec{ ident expr }                   -- only used for 'local function'
          | `Goto{ <string> }                         -- goto str
          | `Label{ <string> }                        -- ::str::
          | `Return{ <expr*> }                        -- return e1, e2...
          | `Break                                    -- break
          | apply

	expr:
            `Nil
          | `Dots
          | `Boolean{ <boolean> }
          | `Number{ <number> }
          | `String{ <string> }
          | `Function{ { `Id{ <string> }* `Dots? } block }
          | `Table{ ( `Pair{ expr expr } | expr )* }
          | `Op{ opid expr expr? }
          | `Paren{ expr }       -- significant to cut multiple values returns
          | apply
          | lhs

	apply:
             `Call{ expr expr* }
           | `Invoke{ expr `String{ <string> } expr* }

	lhs: `Id{ <string> } | `Index{ expr expr }

	opid:  -- includes additional operators from Lua 5.3 and all relational operators
            'add'  | 'sub' | 'mul'  | 'div'
          | 'idiv' | 'mod' | 'pow'  | 'concat'
          | 'band' | 'bor' | 'bxor' | 'shl' | 'shr'
          | 'eq'   | 'ne'  | 'lt'   | 'gt'  | 'le'   | 'ge'
          | 'and'  | 'or'  | 'unm'  | 'len' | 'bnot' | 'not'

Extended Grammar
----------
    block -> stmt `end
    stmt -> (function | if | set | call | while | repeat | local | goto | label | return | `break | do | fornum)*
    expr -> lhs | `nil | `true | `false | number | string | function | table | operation | `paren expr `paren | call
    while -> `while expr do
    repeat -> `repeat stmt `until expr
    tuple -> expr (`comma expr)+
    number -> `int+ (`dot `int+)?
    set -> lhs `equal expr
    string -> `quote whatever `quote
    lhs -> `name | `name sub+ | `name (`comma `name)+
    sub -> (`square expr `square | `dot `name)
    operation ->
    function -> `function `name (`dot `name)* `paren expr* `paren block
    table -> `curly (`name `equal expr (`comma `name `equal expr)* )? `curly)
    if -> `if expr `then stmt (`elseif expr `then stmt)* (`else stmt)? `end
    call -> lhs `paren expr* `paren
    local -> `local lhs (`equal expr)?
    goto -> `goto `name
    label -> `dbcolon `name `dbcolon
    return -> `return expr?
    do -> `do block
    fornum -> `for `name `equal expr `comma expr (`comma expr)? do
    forin -> `for `name (`comma `name)+ `in expr (`comma expr)+ do
