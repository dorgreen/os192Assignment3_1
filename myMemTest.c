
// Created by Dor Green on 14/05/2019.
//
#include "user.h"
#include "types.h"
#include "mmu.h"
#include "user.h"

int test_no = 0;

void test3(void){
    sbrk(10*PGSIZE);
    exit();
}

void test2(void) {
    char *bla = sbrk(20 * PGSIZE);

    for (int i = 0; i < 10; ++i) {
        bla[i] = i;
        bla[20 - i] = i;
    }

    //reading
    for (int i = 0; i < 10; ++i) {
        if (bla[i] != i || bla[20 - i] != i) {
            printf(1, "failed\n");
        }
    }
}

void test1(int flag){
    // request 20 pages.
    char * bla = sbrk(20*PGSIZE);

    // write i to page i
    for(int i = 0; i < 20; i++)
        bla[i*PGSIZE] = i;

    //read
    for(int i = 0; i < 20 ; i++){
        if(bla[i*PGSIZE] != i){
            printf(1, "Simple Test failed");
        }
    }
    if (flag) exit();
}

void mySimpleTets(void){
    int pid;

    if((pid = fork()) == -1) {
        printf(1, "Fail on forking!..\n");
        goto bad;
    }

    if(pid == 0)
        test1(1);
    else{
        wait();
        printf(1, "done\n");
    }
    return;

    bad:
    exit();
}

void doubleProcess(){
    int pid = fork();
    if(!pid){
        test2();
        exit();
    }
    int pid2 = fork();
    if(!pid2){
        test2();
        exit();
    }
    wait();
    wait();
    printf(1,"done\n");
}


int very_simple(int pid){
    if(pid == 0){
        printf(1, "rest...\n");
        sleep(100);
        printf(1, "exit...\n");
        exit();
    }
    if(pid > 0){
        sleep(50);
        printf(1, "Parent waiting on test %d\n", test_no);
        wait();
        printf(1, "Done!\n");
        return 1;
    }
    return 0;
}


int simple(int pid){
    if(pid == 0){
        printf(1, "try alloc, access, free...\n");
        char* trash = malloc(3);
        trash[1] = 't' ;
        if(trash[1] != 't'){
            printf(1,"Wrong data in simple! %c\n", trash[1]);
        }
        free(trash);
        printf(1, "Done malloc dealloc, exiting...\n");
        exit();
    }
    if(pid > 0){
        sleep(50);
        printf(1, "Parent waiting on test %d\n", test_no);
        wait();
        printf(1, "Done! malloc dealloc\n");
        return 1;
    }
    return 0;
}


int test_paging(int pid, int pages){
    if(pid == 0){
        int size = pages;
        char* page;
        for(int i = 0 ; i < size ; i++){
            printf(1, "Call %d for sbrk\n", i);
            page = sbrk(PGSIZE);
            printf(1, "ok %d\n", i);
            printf(1, "page addr: %x\n", (uint)page);
            //*page = '0' +(char) i ;
        }
//        printf(1, "try accessing this data...\n");
//
//        if(size > 4 && *pp[4] != '4'){
//            printf(1,"Wrong data in pp[4]! %c\n", *pp[4]);
//        }
//
//        if(*pp[0] != '0'){
//            printf(1,"Wrong data in pp[0]! %c\n", *pp[0]);
//        }
//
//        if(*pp[size-1] != '0' +(char) size-1 ){
//            printf(1,"Wrong data in pp[size-1]! %c\n", *pp[size-1]);
//        }

        printf(1, "Done allocing, exiting...\n");
        exit();

    }

    if(pid >  0){
        sleep(50);
        printf(1, "Parent waiting on test %d\n", test_no);
        wait();
        printf(1, "Done! alloce'd some pages\n");
        return 1;
    }
    return 0;
}

int test_pmalloc(){
    char * p = pmalloc();
    pfree((void*) p);
    return 0;
}

int test_pmalloc2(int pid){
    if(pid == 0){
        printf(1, "try palloc, lock, free...\n");
        char* p = (char *) pmalloc();
        p[1] = 't' ;
        if(p[1] != 't'){
            printf(1,"Wrong data in test_pmalloc2!\n");
        }

        printf(1, "try protect\n");
        if(!protect_page((void*) p)){
            printf(1,"Can't protect_pmalloc2!\n");
            exit();
        }

        printf(1, "try free\n");
        if(!pfree((void*) p)){
            printf(1,"Can't pfree_pmalloc2!\n");
            exit();
        }

        exit();
    }
    if(pid > 0){
        sleep(50);
        printf(1, "Parent waiting on test %d\n", test_no);
        wait();
        printf(1, "Done! malloc dealloc\n");
        return 1;
    }
    return 0;
}

int test_pmalloc3(int pid){
    if(pid == 0){
        printf(1, "try palloc, lock, access, free...\n");
        char* p = (char *) pmalloc();
        p[1] = 't' ;
        if(p[1] != 't'){
            printf(1,"Wrong data in test_pmalloc3!\n");
        }

        if(!protect_page((void*) p)){
            printf(1,"Can't protect_pmalloc3!\n");
            exit();
        }

        // SHOULD BE A PAGEFAULT!!!
        p[1] = 'e' ;
        if(p[1] != 't'){
            printf(1,"Succseess!! Can't write to locked data!\n");
        }

        if(!pfree((void*) p)){
            printf(1,"Can't pfree_pmalloc3!\n");
            exit();
        }

        exit();
    }

    if(pid > 0){
        sleep(50);
        printf(1, "Parent waiting on test %d\n", test_no);
        wait();
        printf(1, "Done! pmalloc3 (should raise PGFAULT!)\n");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]){

    printf(1, "--------- START TESTING! ---------\n");

//    printf(1, "------- test%d -------\n", test_no);
//    test_pmalloc();
//    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    test_pmalloc2(fork());
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    test_pmalloc3(fork());
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    very_simple(fork());
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    simple(fork());
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    test_paging(fork(),2);
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    test_paging(fork(),6);
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    test_paging(fork(),13);
    test_no++;

#ifdef NONE
    printf(1, "------- test%d -------\n", test_no);
    mySimpleTets();
    test_no++;

    printf(1, "------- test%d -------\n", test_no);
    doubleProcess();
    test_no++;
#endif




    printf(1, "--------- DONE  TESTING! ---------\n");
    return 0;
}