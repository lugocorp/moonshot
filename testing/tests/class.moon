
class Testing where
  int a=0

  constructor(int a)
    this.a=a
    a=a
  end

  int get_a2()
    return this.a+a
  end

  var other()
    a=1
    get_a2()
  end
end
