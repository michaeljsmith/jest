#n
/^import / {
  =;
  s/^import[ \t]*/{r /;
  s/[ \t]*$/.jest/p;
  a\
  d;}
}
