#include <stdio.h>
#include <stdlib.h>

int fib(int n){
    if(n<=1){
        return n;
    }
    return fib(n-1)+fib(n-2);
}

int main(int argc, char *argv[]){

    int n = atoi(argv[1]);

    int res = fib(n);
    printf("Fib(%d): %d\n", n, res);

    return 0;
}