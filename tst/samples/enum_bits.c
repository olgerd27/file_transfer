#include <stdio.h>

enum Features {
    ftr_none     = 0 << 0
  , ftr_network  = 1 << 1
  , ftr_ssh      = 1 << 2
  , ftr_cmd      = 1 << 3
  , ftr_user_mng = 1 << 4
  , ftr_time     = 1 << 5
  , ftr_interact = 1 << 6
};

int main()
{
  printf("ftr_none:     dec=%i hex=%#010x\n", ftr_none, ftr_none);
  printf("ftr_network:  dec=%i hex=%#010x\n", ftr_network, ftr_network);
  printf("ftr_ssh:      dec=%i hex=%#010x\n", ftr_ssh, ftr_ssh);
  printf("ftr_cmd:      dec=%i hex=%#010x\n", ftr_cmd, ftr_cmd);
  printf("ftr_user_mng: dec=%i hex=%#010x\n", ftr_user_mng, ftr_user_mng);
  printf("ftr_time:     dec=%i hex=%#010x\n", ftr_time, ftr_time);
  printf("ftr_interact: dec=%i hex=%#010x\n", ftr_interact, ftr_interact);
  return 0;
}
