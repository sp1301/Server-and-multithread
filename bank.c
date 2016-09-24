#include <stdio.h>
#include <stdlib.h>
#include "bank.h"
#include <string.h>
bank createBank(char* name){
  bank temp = (struct bank*) malloc(sizeof(struct bank));
  temp->balance = 0;
  temp->inFlag = 1;
  temp->name = strdup(name);
  return temp;
}

