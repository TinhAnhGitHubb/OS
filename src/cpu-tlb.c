/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include "cpu-tlbcache.c"
#include <stdlib.h>
#include <stdio.h>

int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */

  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/
  if(mp == NULL){
    return -1;
  }
  int size_tlb = mp->maxsz;
  for(int i = 0; i < size_tlb; i++){
    mp->storage[i]  = -1;

  }

  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  
  //* the value of the int addr: *alloc_addr = rgnode.rg_start;
  int page_number = PAGING_PGN(addr);
  int inc_byte = PAGING_PAGE_ALIGNSZ(size); //* if size is 300, then it returns 512
  int numpage = inc_byte / PAGING_PAGESZ;
  for(int i = 0; i < numpage; i++){
    uint32_t pte = proc->mm->pgd[(page_number + i) % proc->tlb->maxsz];
    if(PAGING_PAGE_PRESENT(pte)){ //* checking with the following pagenumber, if the bit present in the pte is set to 1, if not, doing some swapping
      uint32_t framenumTmp = PAGING_FPN(pte);
      int statusCacheWrite = tlb_cache_write(proc->tlb, proc->pid, (page_number + i) % proc->tlb->maxsz, framenumTmp); //* handling flush for tlb_cache_read and write 
      if (statusCacheWrite != 0){
        //* do not have that pgn of the process in the tlb
        proc->tlb->storage[(page_number + i) % proc->tlb->maxsz] = framenumTmp;
      }
      
    }else{
      //* handling swap page
    }
  }
  return val; 
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/

  /* by using tlb_cache_read()/tlb_cache_write()*/

  int start_region = proc->mm->symrgtlb[reg_index].rg_start;
  int end_region = proc->mm->symrgtlb[reg_index].rg_end;
  int size = eng_region - start_region;
  int pgn = PAGING_PGN(start_region);
  int deallocate_sz = PAGING_PAGE_ALIGNSZ(size);
  int freed_pages = deallocate_sz / PAGING_PAGESZ;
  for(int i = 0; i < freed_pages; i++){
    int index_addr = (pgn + i) % proc->tlb->maxsz;
    uint32_t pte = proc->mm->pgd[index_addr];
    if(PAGING_PAGE_PRESENT(pte)){
      uint32_t framenumTmp = PAGING_FPN(pte);
      int statusCacheWrite = tlb_cache_write(proc->tlb, proc->pid, index_addr, framenumTmp);
      if(statusCacheWrite != 0){
        proc->tlb->storage[index_addr] = framenumTmp;
      }
    }else{
      //* handling swap page
    }
  }

  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  BYTE data, frmnum = -1;
	
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/

  int start_region = proc->mm->symrgtlb[source].rg_start + offset;
  
  int pgn = PAGING_PGN(start_region);

  uint32_t fpn_retrieved_from_tlb = 0;
  frmnum = tlb_cache_read(proc->tlb, proc->pid, pgn, fpn_retrieved_from_tlb);

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  int val = __read(proc, 0, source, offset, &data);

  

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  int start_destination = proc->mm->symrgtlb[destination].rg_start;

  int pgn = PAGING_PGN(start_destination);
  int fpn = PAGING_FPN(data);
  int status = tlb_cache_write(proc->tlb, proc->pid, pgn, fpn);
  if (status < 0){
    //* cache failed, pagenum tum lum
    proc->tlb->storage[pgn] = fpn;
  }


  return val;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val;
  BYTE frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  return val;
}

//#endif
