<img src="moonshot.svg" height="300px"/>

# Moonshot
Moonshot is an optionally-typed object-oriented extension language for Lua. This project allows you to use your favorite Lua libraries and frameworks with a more organized and error-safe code base. Moonshot is lightweight, the library compiles to just under 110KB. It gives you full interop with vanilla Lua as well as a fully developed optional typing system.

## Getting Started
To build the project, simply run:
```
make moonshot
```

and then to install it globally, run:
```
make install
```

## Todo
- [ ] Make sure each nested stmt node has a corresponding scope
- [ ] Add types to scopes, use that for type lookup instead of searching the equivalence graph
- [ ] In the validation step, process type definitions first in a stmt and then process everything else
- [ ] Give AST nodes a line number for better traversal stage error messages
- [ ] Create a VSCode syntax highlighting plugin
- [ ] Change file extension for source files

## Developers
Moonshot was developed by [LugoCorp](http://lugocorp.net)
