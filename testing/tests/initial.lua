function sup()
  return red==0
end
red=0

function hello()
  msg="Hello there"
  while sup() do
    msg="Ello gov'nor"
  end
  if not red then
    red=1
  end
  print("msg")
end

do
  print("Goodbye")
  print("Ad".."i".."os")
end
