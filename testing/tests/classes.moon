
interface Methods where
  string hello()
  var whatsup()
  var coords(int a)
end

class Math implements Methods where
  string hello()
    return "Hi there"
  end
  var whatsup() end
  var coords(int a)
    return a
  end
end
