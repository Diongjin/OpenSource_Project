#include <stdlib.h>
#include <stdio.h>

void exitfunc1(void);
void exitfunc2(void);

int main(){
    atexit(exitfunc1);
    atexit(exitfunc2);
    printf("This is main function.\n");
}

void exitfunc1(void){
    printf("This is exit function1.\n");
}
void exitfunc2(void){
    printf("This is exit function2.\n");
}