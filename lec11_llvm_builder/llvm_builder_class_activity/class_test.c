extern void print(int);
extern int read();
int func(int p) {
    int a;
    a = read();
    if (a > p) {
        return p;
    } else {
        return a;
    }
}
