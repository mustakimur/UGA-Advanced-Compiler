#include <stdio.h>

long long doSum(int a, long b) {
  long long sum;
  sum = a + b;
  return sum;
}
long long wrap_doSum(int a, long b)
{
    printf("%d\n", a);
    printf("%ld\n", b);
    return doSum(a, b);
}
int main() { return wrap_doSum(10, 20); }
