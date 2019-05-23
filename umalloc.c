#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"
#include "memlayout.h"
#include "x86.h"


// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
    struct {
        union header *ptr;
        uint size;
    } s;
    Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap) {
    Header *bp, *p;

    bp = (Header *) ap - 1;
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            break;
    if (bp + bp->s.size == p->s.ptr) {
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else
        bp->s.ptr = p->s.ptr;
    if (p + p->s.size == bp) {
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
    freep = p;
}

static Header *
morecore(uint nu) {
    char *p;
    Header *hp;

    if (nu < 4096)
        nu = 4096;
    p = sbrk(nu * sizeof(Header));
    if (p == (char *) -1)
        return 0;
    hp = (Header *) p;
    hp->s.size = nu;
    free((void *) (hp + 1));
    return freep;
}

// The same as morecore, but won't update nu to 4096 if it's smaller than 4k
// Used only in pmalloc
static Header*
morecore1(uint nu){
    char *p;
    Header *hp;
//    if (nu < 4096)
//        nu = 4096;
    p = sbrk(nu * sizeof(Header));
    if (p == (char *) -1)
        return 0;
    hp = (Header *) p;
    hp->s.size = nu;
    free((void *) (hp + 1));
    return freep;
}

void *
malloc(uint nbytes) {
    Header *p, *prevp;
    uint nunits;

    nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
    if ((prevp = freep) == 0) {
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }
    for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {
            if (p->s.size == nunits)
                prevp->s.ptr = p->s.ptr;
            else {
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return (void *) (p + 1);
        }
        if (p == freep)
            if ((p = morecore(nunits)) == 0)
                return 0;
    }
}


// #TASK1
// Allocates exactly one page, starting from a new page (page-aligned)
// A PTE can only
// refer to a physical address that is aligned on a 4096-byte boundary
// We can use PGROUNDUP to round the virtual address up to a page boundary.
// Start at myproc->pgdir, allocuvm to allocate a new page
// then,  mark it with PTE_PM
void *pmalloc(void) {
    //uint page_index;
    Header* head;
    head = morecore1(512);

    // try to set flags!
    if(!set_flags((uint)head->s.ptr, PTE_PM, 0)){
      free((void*)head->s.ptr);
      return 0;
    }

    return (void*) head->s.ptr;
}

// #TASK1
// If *ap is the start of a page allocated with PMALLOC, protect if (return 1)
// else, return -1
// we could protect by setting the "WRITABLE" bit (bit 1) of address ( PTE_W )
int protect_page(void* ap){
    // if pmalloc'd and start of page
    if((get_flags((uint)ap) & PTE_PM)){
        return set_flags((uint) ap, ~PTE_W, 1);
    }
    return -1;
}

// #TASK1
// release a protected page. return 1 upon success
// if page is unprotected (or not a page) do nothing and return -1
int pfree(void* ap){
    // if not pmalloc'd or if not start of page
    // TODO: is that the right way to make sure it's page alligned?
    if(!(get_flags((uint)ap) & PTE_PM) || (uint)ap != PGROUNDUP((uint) ap)){
        return -1;
    }
    // Clear PMALLOC flag
    set_flags((uint)ap, ~PTE_PM, 1);
    // Set writable flag to ON
    set_flags((uint)ap, PTE_W, 0);

    free(ap);
    return 1;
}



