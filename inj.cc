#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdint.h>

extern "C" {
  int64_t willInject(int64_t uid);
}
int64_t willInject(int64_t uid){
  char* tenv=getenv("FAULTS");
  printf(tenv);
  char *env=(char*)malloc(sizeof(char)*strlen(tenv));;
  char *rt=env;
  strcpy(env,tenv);
  char* c=env;
  while(true){
    int flag=0;
    if(((*env)==',')||((*env)==0)){
      if((*env)==0)
        flag=1;
      *env=0;
      if(uid==atoi(c)){
        free(rt);
        return true;
      }
      c=env+1;
      if(flag==0)
        *env=',';
    }
    if(*env==0)
      break;
    env++;
  }
  free(rt);
  return false;
}

