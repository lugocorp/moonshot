
typedef Message string

class Messenger where
  var sendMessage(Message m)

  end
end

class Hello where
  Goodbye gb
end

class Goodbye where
  Hello h
end

print("Reference here")
