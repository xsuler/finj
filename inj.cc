extern "C" {
  int willInject(int uid);
  long long int fault_table;
}

int willInject(int uid){
  return (fault_table>>uid)&1;
}
