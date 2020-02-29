#include<iostream>
#include<stdlib.h>

using namespace std;
extern "C" {
  int willInject(int uid);
}
int willInject(int uid){
  char* env=getenv("FAULT_INJECTION");
  char* c=env;
  while(true){
    if(*env==','||*env==0){
      *env=0;
      if(uid==atoi(c)){
        return true;
      }
      c=env+1;
      *env=',';
    }
    if(*env==0)
      break;
    env++;
  }
  return false;
}

int func(){
  int a;
  cin>>a;
  if(a>2){
    cout<<0<<endl;
    goto fail;
  }
  return 0;
 fail:
  cout<<2<<endl;
  return 0;

}

int main(){

  int a;
  cin>>a;
  if(a>2){
    cout<<0<<endl;
    goto fail;
  }
  return 0;

  fail:
  cout<<2<<endl;
  return 0;

}
