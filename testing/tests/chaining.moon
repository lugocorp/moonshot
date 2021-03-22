
class OlderTest where
  constructor()
    print("Old days")
  end

  int number()
    return 0
  end
end

class OldTest extends OlderTest where
  constructor(int x)
    super()
    print("Happy days")
  end

  var hello()
    a=1
  end
end

class Test extends OldTest where
  constructor()
    super(1)
  end

  string hello()
    super()
    return "hello"
  end

  int number()
    super()
    return 0
  end
end

class YoOld where
  int a

  constructor(int a)
    this.a=a
  end
end

class YoChild extends YoOld where
  constructor()
    super(123)
  end
end
