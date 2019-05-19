// Created by Dor Green on 14/05/2019.
//

#include "user.h"
#include "types.h"
#include "mmu.h"
#include "user.h"

int test_paging(int pid){
    if(pid == 0){
        int size = 6;
        char* pp[size];
        for(int i = 0 ; i < size ; i++){
            pp[i] = sbrk(PGSIZE-1);
            *pp[i] = '0' +(char) i ;
        }

        if(*pp[4] != '4'){
            printf(1,"Wrong data in pp[4]! %c\n", *pp[4]);
        }

        if(*pp[0] != '0'){
            printf(1,"Wrong data in pp[0]! %c\n", *pp[0]);
        }

        if(*pp[size-1] != '0' +(char) size-1 ){
            printf(1,"Wrong data in pp[size-1]! %c\n", *pp[size-1]);
        }

        exit();

    }

    if(pid !=  0){
        wait(pid);
        printf(1, "Done! alloce'd some pages");
        return 1;
    }
}



int main(int argc, char *argv[]){

    test_paging(fork());

    // TODO: ADD MORE TESTS!!


    return 0;
}