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
- [ ] Give Tokens a filename for error messages
- [ ] Give AST nodes a line number and filename for better traversal stage error messages
- [ ] Create a VSCode syntax highlighting plugin
- [ ] Create an Atom syntax highlighting package

## Bugs
- [ ] Infinite loop when a file requires itself (or there's a loop in requires)

## Developers
Moonshot was developed by [LugoCorp](http://lugocorp.net)
