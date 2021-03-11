
interface Methods where
  string hello()
  var whatsup()
  (int,int) coords(int a)
end

class Math implements Methods where
  string hello()
    return "Hi there"
  end
  var whatsup() end
  (int,int) coords(int a)
    return a,a+1
  end
end
