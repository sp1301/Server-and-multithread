struct bank{
  float balance;
  int  inFlag;
  char *name;//[100];
};

typedef struct bank* bank;

bank createBank(char*);
