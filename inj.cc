extern "C" {
  int willInject(int uid);
}

void getFileContent(char* path, char* buf){

  asm(
       "movq %0, %%rdi\n\t"
       "movq $2, %%rax\n\t"
       "movq $0, %%rsi\n\t"
       "syscall\n\t"

       "mov %%rax, %%rdi\n\t"
       "mov $0, %%rax\n\t"
       "mov %1, %%rsi\n\t"
       "mov $1000, %%rdx\n\t"
       "syscall\n\t"

       :
       :"g" (path), "g" (buf)
       :"memory"
       );
}


int willInject(int uid){
  char buf[1001];
  getFileContent("/home/sule/faults",buf);
  if(buf[uid]=='1'){
    return 1;
  }
  return 0;
}
