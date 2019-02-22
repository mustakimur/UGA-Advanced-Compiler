#include <stdio.h>

typedef void (*vfnptr)();
typedef void (*ifnptr)(int);

void CallA() {}

void CallB() {}

void CallC() {}

void CallD(int a) {}

void CallE() {}

void CallF(int a) {}

void CallG(int a) {}

void CallH(int a) {}

void CallI() {}

class Hello {
private:
  vfnptr fp1 = &CallA;
  vfnptr fp2;

public:
  void (Hello::*x)();
  Hello(vfnptr f) { fp2 = f; }
  void ptofn() {}
  virtual void vFunc() {}
};

typedef struct ST {
  int a;
  ifnptr fp;
} st;

st st_arr[] = {{10, &CallD}, {20, &CallF}};
vfnptr gl = &CallE;

int main() {
  Hello *h = new Hello(&CallI);
  static vfnptr sfp[] = {&CallC, &CallE};
  st lc;
  lc.fp = &CallG;
  h->x = &Hello::ptofn;
  CallB();
  lc.fp(10);
  h->vFunc();
  return 0;
}