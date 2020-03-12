#include<iostream>
#include "test.h"

int willInject(int uid){
  return (fault_table>>uid)&1;
}

using namespace std;
int func(){
  int a=0;
  if(a>2){
    cout<<1<<endl;
    goto fail;
  }
  return 0;
 fail:
    cout<<2<<endl;
  return 0;

}

int main(){

  int a=0;
  func();
  if(a>2){
    cout<<1<<endl;
    goto fail;
  }
  return 0;
  
 fail:
    cout<<2<<endl;
  return 0;
}
