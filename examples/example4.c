#include <stdio.h>
int main() {
    int x = 3;
    int y = 4;
    int z = 8;
    if (x < z) {
      y = 4;
      printf("%d", z);
    }
    if (x < y) {
        if (y > 2) {
            x = y + 1;
        } else {
            x = 0;
        }
    }
    return x;
}
