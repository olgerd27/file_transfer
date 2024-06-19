#include <stdio.h>

// Calculate the number of digits in a integer type number
int numb_digits(long val)
{
  int numb = 1;
  while ((val /= 10) > 0) ++numb;
  return numb;
}

int main(int argc, char const *argv[])
{
    long val_init = 5;
    long val;
    for (val = val_init; val < 1e+10; val *= 10) {
        printf("val = %ld,  digits number: %d\n", val, numb_digits(val));
    }
    return 0;
}
