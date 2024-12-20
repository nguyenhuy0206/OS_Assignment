//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt)
{
    struct vm_rg_struct *new_node;
    struct vm_rg_struct *head = mm->mmap->vm_freerg_list;

    // Check for invalid region
    if (rg_elmt.rg_start >= rg_elmt.rg_end)
        return -1;

    // Dynamically allocate memory for the new region
    new_node = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));
    if (!new_node)
        return -1; // Allocation failed

    // Copy the data into the new node
    *new_node = rg_elmt;
    new_node->rg_next = head;

    // Update the list head
    mm->mmap->vm_freerg_list = new_node;

    return 0;
}


/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
//get địa chỉ ô vma
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid)
  {
    if(pvma == NULL)
	  return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;

  /* TODO: commit the vmaid */
  rgnode.vmaid = vmaid;

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    //symrgtbl là vm_rg_struct
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

    caller->mm->symrgtbl[rgid].vmaid = rgnode.vmaid;

    *alloc_addr = rgnode.rg_start;

    return 0;
  }

  /* TODO: get_free_vmrg_area FAILED handle the region management (Fig.6)*/
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == -1 ){return -1;}
  /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
  /*Attempt to increate limit to get space */

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  int inc_limit_ret;

  /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
  int old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  if (inc_vma_limit(caller, vmaid, inc_sz, &inc_limit_ret) < 0) return -1;

    cur_vma->sbrk += inc_sz;
  /* TODO: commit the limit increment */
    if ( old_sbrk + size > cur_vma -> vm_end )
 {
 if ( inc_vma_limit ( caller , vmaid , inc_sz, &inc_limit_ret ) < 0)
 {
            struct framephy_struct * frm_lst = NULL ;
            struct vm_rg_struct * ret_rg = malloc ( sizeof ( struct vm_rg_struct ) );
            int pages = ( inc_sz / PAGING_PAGESZ );
            ret_rg -> rg_start = ret_rg -> rg_end = old_sbrk ;
            ret_rg -> rg_next = NULL ;
            alloc_pages_range ( caller , pages , & frm_lst );
            vmap_page_range ( caller , old_sbrk , pages , frm_lst , ret_rg );
            caller ->mm -> symrgtbl [ rgid ]. rg_start = ret_rg -> rg_start ;
            caller ->mm -> symrgtbl [ rgid ]. rg_end = ret_rg -> rg_end ;
            cur_vma -> sbrk += ret_rg -> rg_end - ret_rg -> rg_start ;
            return 0;
 }
 }

 /* Successful increase limit */
 caller ->mm -> symrgtbl [ rgid ]. rg_start = old_sbrk ;
 caller ->mm -> symrgtbl [ rgid ]. rg_end = old_sbrk + size ;
  /* TODO: commit the allocation address 
  // *alloc_addr = ...
  */
 *alloc_addr = rgnode.rg_start;

  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __free(struct pcb_t *caller, int rgid)
{
  struct vm_rg_struct rgnode;

  // Dummy initialization for avoding compiler dummay warning
  // in incompleted TODO code rgnode will overwrite through implementing
  // the manipulation of rgid later
  rgnode.vmaid = 0;  //dummy initialization
  rgnode.vmaid = 1;  //dummy initialization

  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  /* TODO: Manage the collect freed region to freerg_list */
  struct vm_rg_struct *sym_rg = &caller->mm->symrgtbl[rgid];
  rgnode.rg_start = sym_rg->rg_start;
  rgnode.rg_end = sym_rg->rg_end;
  rgnode.vmaid = sym_rg->vmaid;
  
  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, rgnode);

  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgmalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify vaiable in symbole table)
 */
int pgmalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 1 */
  return __alloc(proc, 1, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
   return __free(proc, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
    uint32_t pte = mm->pgd[pgn];

    if (!PAGING_PTE_PAGE_PRESENT(pte))
    { 
        // Page không tồn tại, cần đưa vào bộ nhớ
        int vicpgn, swpfpn, vicfpn;
        uint32_t vicpte;

        int tgtfpn = PAGING_PTE_SWP(pte); // Frame mục tiêu trong swap

        // Tìm victim page
        if (find_victim_page(caller->mm, &vicpgn) < 0)
            return -1;

        vicpte = caller->mm->pgd[vicpgn];
        vicfpn = PAGING_OFFST(vicpte);

        // Lấy free frame từ MEMSWP
        if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0)
        {
            struct memphy_struct **mswpit = caller->mswp;
            for (int i = 0; i < PAGING_MAX_MMSWP; i++)
            {
                struct memphy_struct *tmp_swp = mswpit[i];
                if (MEMPHY_get_freefp(tmp_swp, &swpfpn) == 0)
                {
                    __swap_cp_page(caller->mram, vicfpn, tmp_swp, swpfpn);
                    caller->active_mswp = tmp_swp;
                    break;
                }
            }
        }
        else
        {
            __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
        }

        // Swap frame từ MEMRAM sang MEMSWP và ngược lại
        __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

        // Cập nhật page table
        MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
        // swptype = 1
        pte_set_swap(&caller->mm->pgd[vicpgn], 1, swpfpn);

        // Đánh dấu trạng thái online của page mục tiêu
        pte_set_fpn(&caller->mm->pgd[pgn], vicfpn);
        pte = caller->mm->pgd[pgn];



    *fpn = PAGING_OFFST(pte);
    return 0;
}
}

    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    //__swap_cp_page();
    /* Copy target frame from swap to mem */
    //__swap_cp_page();

    /* Update page table */
    //pte_set_swap() &mm->pgd;

    /* Update its online status of the target page */
    //pte_set_fpn() & mm->pgd[pgn];
  //   pte_set_fpn(&pte, tgtfpn);

  //   enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  // }

  // *fpn = PAGING_PTE_FPN(pte);

  // return 0;


/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram,phyaddr, value);

   return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __read(struct pcb_t *caller, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{
  BYTE data;
  int val = __read(proc, source, offset, &data);

  destination = (uint32_t) data;
#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  int vmaid = currg->vmaid;

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return __write(proc, destination, offset, data);
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PTE_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */

 newrg->rg_start = cur_vma -> sbrk;

 if ( (cur_vma ->sbrk + alignedsz) > cur_vma -> vm_end ) return newrg = cur_vma -> sbrk; 
cur_vma ->sbrk = cur_vma->sbrk + alignedsz;

 newrg->rg_end = cur_vma -> sbrk;
 
return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  while(vma != NULL)
  {
    
      if ((vmastart >= vma->vm_start && vmastart < vma->vm_end) || (vmastart <= vma->vm_start && vmaend >= vma->vm_end))
      {
        return -1; 
      }
      vma = vma->vm_next;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *@inc_limit_ret: increment limit return
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz, int* inc_limit_ret)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* TODO: Obtain the new vm area based on vmaid */
  cur_vma->sbrk = area -> rg_start;


  if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                    old_end, incnumpage , newrg) < 0)
    return -1; /* Map the memory to MEMRAM */
  inc_limit_ret = cur_vma->vm_end + inc_sz;
  return 0;

}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn) 
{
  struct pgn_t *pg = mm->fifo_pgn;
  
  /* TODO: Implement the theorical mechanism to find the victim page */
  if (pg == NULL) return -1;

  *retpgn = pg->pgn;

  mm->fifo_pgn = pg->pg_next;

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  //check điều kiện bịp
  while (rgit != NULL && rgit->vmaid == vmaid)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        { /*End of free list */
          rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
    }
    else
    {
      rgit = rgit->rg_next;	// Traverse next rg
    }
  }

 if(newrg->rg_start == -1) // new region not found
   return -1;

 return 0;
}

//#endif
