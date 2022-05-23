#include "stdio.h"

int main (int argc, char *argv[])
{
  // int teste = 0x77777777;
  int *teste = new int;
  *teste = 0x11223344;
  printf("Starting Game...\n");
  while (*teste == 0x11223344) {
    
  }
  printf("You've hack the game!!!\n");
  return 0;
}
