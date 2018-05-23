int main(int argc) {

  int x = 1;
  if (argc > 7 )
    x = x + argc;
  else
    x = argc;
  
  return x;
}
