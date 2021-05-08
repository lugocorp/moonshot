

string greeting="Greetings!"

interface Test where

end
class Object implements Test where
  (int,int) greet(int a)
    print(a)
    return a,a
  end
end

typedef Thing Object
Thing obj=Object()

repeat
  a,b=obj.greet(-3+(5*2))
until true

local tab={msg="hello"}
do
  if true then
    tab["msg"]="goodbye"
  end
end

for a=1,3 do
  print(a)
end
for k,v in ipairs(tab) do
  print(k)
end

while false do
  break
end

a=true
::hello::
print("Got to hello")
if a then
  a=false
  goto hello
end
print("Goodbye lol")
