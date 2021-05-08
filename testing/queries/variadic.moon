
(string,string) hello(string a,...)
  string b=trust arg[1]
  if b==nil then
    b="b"
  end
  return a.." x "..b,a
end

hello("1","yo","extra","ignore me lol")
hello("hi")

function test()

end

test()

function goodbye(...)

end

goodbye(1,2,3)
goodbye()
