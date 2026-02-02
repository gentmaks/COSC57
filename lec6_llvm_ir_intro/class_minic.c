extern void print(int);
extern int read();

int func(int i) {
  int n;
  int max;
  n = read();
  max = 0;
  for (int i = 0; i < n; i++) {
    int tmp;
    tmp = read();
    if (tmp > max) {
      max = tmp;
    }
  }
  print(max);
  return max;
}