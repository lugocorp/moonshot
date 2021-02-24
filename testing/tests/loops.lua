function yellow() print("Yellow") end
function hello() print("Hello") end

repeat
  hello()
  red=127.11
until 1==1

while 1 do
  red=false
  yo=yellow()
  break
end

for a=0,1 do
  print("Yo whaddup")
end

t={a=1,b=2,c=3}
for k,_ in ipairs(t) do
  print(k)
end

do
  print("tee hee tummy tums".."1234")
end
