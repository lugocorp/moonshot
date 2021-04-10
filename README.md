<img src="moonshot.svg" height="300px"/>

# Moonshot
Moonshot is an optionally-typed object-oriented extension language for Lua. This project allows you to use your favorite Lua libraries and frameworks with a more organized and error-safe code base. Moonshot is lightweight, and doesn't weigh down on your runtime. It gives you full interop with vanilla Lua as well as a fully developed optional typing system.

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
- [x] In the validation step, process type definitions first in a stmt, then check promises and then process all other nodes
- [x] Update the types equivalency graph as scoped types come and go
- [ ] Give AST nodes a line number for better traversal stage error messages
- [ ] Create a VSCode syntax highlighting plugin
- [ ] Change file extension for source files

## Developers
Moonshot was developed by [LugoCorp](http://lugocorp.net)
