extern "C" {
  int willInject(int uid);
  extern long long int fault_table;
}

int willInject(int uid){
  return (fault_table>>uid)&1;
}
