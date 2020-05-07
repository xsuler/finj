#include <stdio.h>
#include <stdint.h>


void enter_func(char* name, char* file){

}
 
void leave_func(){

}

void mark_hp_flag(char* addr, int64_t size){
 printf("hp addr: %p, size: %ld\n",addr,size);
}

void mark_hp_flag_r(char* addr, int64_t size){
 printf("hpr addr: %p, size: %ld\n",addr,size);

}
void mark_valid(char* addr, int64_t size){
}
void mark_write_flag(char* addr, int64_t size){
 printf("write flag addr: %p, size: %ld\n",addr,size);
}
void mark_write_flag_r(char* addr, int64_t size){
 printf("writer flag addr: %p, size: %ld\n",addr,size);
}
void mark_invalid(char* addr, int64_t size, char type){
}


void report_xasan(int64_t* addr, int64_t size, int64_t type){
 printf("report usage of addr %p\n",addr);
}

int willInject(int uid){
	return 0;
}

struct ff{
    char x[5];
};

int func(int a){
	if(a>100)
		goto fail;
fail:
	printf("fault");
	return 0;
}

int main(){
	int a;
  scanf("%d",&a);
  func(a);
  return 0;
}
