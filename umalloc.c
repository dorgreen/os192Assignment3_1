#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"


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

typedef struct plist {
    uint va;
    int used;
    struct plist* next;
} plist;

static plist* plist_head = 0;


void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

// Now only allocates page-alligned pages
static Header *
morecore(uint nu) {
  char *p;
  Header *hp;

  if (nu < 4096)
    nu = 4096;
  // Will ONLY allocate page-alligned chunks
  p = sbrk(PGROUNDUP(nu * sizeof(Header)));
  if (p == (char *) -1)
    return 0;
  hp = (Header *) p;
  // Set the right size for this chunk
  hp->s.size = PGROUNDUP(nu * sizeof(Header)) / sizeof(Header);
  free((void *) (hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits)) == 0)
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
  // Allocate first link if missing..
  if(plist_head == 0){
    plist_head = (plist*) malloc(sizeof(struct plist));
    plist_head->next = 0;
    plist_head->used = 0;
    plist_head->va = (uint)sbrk(PGSIZE);
    // we record the head of the list inside the proc so that we could deep-copy the list to forked proc
    //myproc()->plist_head = plist_head; // TODO: UNCOMMENT AND MAKE IT WORK, SHOULD DEEP-COPY LIST ON FORK
  }

  struct plist* free_node = plist_head;

  // We're looking for an unused node
  // If not found, allocate a node and a page for it.
  while(free_node->used != 0){
    // This is the last node, and all are used. allocate another!
    if(free_node->used == 1 && free_node->next == 0){
      // Allocate a new node, add it to the list
      free_node->next = (plist*) malloc(sizeof(struct plist));
      // Skip to the newly crreated node
      free_node = free_node->next;
      free_node->used = 0;
      free_node->next = 0;
      free_node->va = (uint)sbrk(PGSIZE);
      break;
    }
    else{
      free_node = free_node->next;
    }
  }

  // By now, free_node is really free and free_node->va points to a page.
  free_node->used = 1;


  // try to set flags!
  if(!set_flags(free_node->va, PTE_PM & PTE_P & PTE_U & PTE_W, 0)){
    // If failed, mark as free and turn off PRESENT flag
    free_node->used = 0;
    set_flags(free_node->va, ~PTE_P, 1);
    return 0;
  }

  return (void*) free_node->va;
}

// #TASK1
// If *ap is the start of a page allocated with PMALLOC, protect if (return 1)
// else, return -1
// we could protect by setting the "WRITABLE" bit (bit 1) of address ( PTE_W )
int protect_page(void* ap){
  // if pmalloc'd and start of page

  struct plist* free_list = plist_head;

  while(free_list != 0){
    if(free_list->va == (uint) ap)
      break;
    else{
        free_list = free_list->next;
    }
  }

    // make sure it's page-alligned
  if((uint) ap != PGROUNDDOWN((uint)ap)){ return -1;}

  // If not found on internal list, it's a dud
  if(free_list == 0 || free_list->va != (uint) ap) return -1;

  // if flag is set as allocated with PMALLOC, protect the page
  if((get_flags((uint)ap) & PTE_PM)){
    return set_flags((uint) ap, ~PTE_W, 1);
  }
  return -1;
}

// #TASK1
// release a protected page. return 1 upon success
// if page is unprotected (or not a page) do nothing and return -1
int pfree(void* ap){
  // if pmalloc'd and start of page
  struct plist* free_list = plist_head;
    while(free_list != 0){
        if(free_list->va == (uint) ap)
            break;
        else{
            free_list = free_list->next;
        }
    }

  // If not found on internal list, it's a dud
  if(free_list->va != (uint) ap) return -1;

  // if not pmalloc'd or if not start of page
  if(!(get_flags((uint)ap) & PTE_PM) || (uint)ap != PGROUNDUP((uint) ap)){
    return -1;
  }

  // Clear PMALLOC flag
  //set_flags((uint)ap, ~PTE_PM, 1);

  // Set writable flag to ON
  set_flags((uint)ap, PTE_W, 0);

  // Set present flag to OFF so that this memory won't be availablev
  set_flags((uint)ap, ~PTE_P, 1);

  // Set internal linkedlist node to free
  free_list->used = 0;
  return 1;
}
