#n
/^import / {
  =;
  s/^import[ \t]*/{r obj\//;
  s/[ \t]*$/.jest.evaluated/p;
  a\
  d;}
}
