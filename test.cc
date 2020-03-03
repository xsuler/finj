#include<iostream>

using namespace std;

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
  func();
  if(a>2){
    cout<<0<<endl;
    goto fail;
  }
  return 0;
  
 fail:
  cout<<2<<endl;
  return 0;

}
