#include <cernel/mm/vmm.h>
#include <cernel/mm/pmm.h>
#include <cernel/lib/print.h>
#include <cernel/lib/memory.h>
#include <debug/debug.h>
#include <stddef.h>
#include <stdint.h>

struct PageTable *pt_kernel;

void create_address_indexer(struct AddressIndexer *indexer, uintptr_t virt_addr) 
{
	indexer->offset = virt_addr & 0xfff;
	virt_addr >>= 12;
    indexer->pml1 = virt_addr & 0x1ff;
    virt_addr >>= 9;
    indexer->pml2 = virt_addr & 0x1ff;
    virt_addr >>= 9;
    indexer->pml3 = virt_addr & 0x1ff;
    virt_addr >>= 9;
    indexer->pml4 = virt_addr & 0x1ff;
}

uintptr_t vmm_translate(struct PageTable *page_table, uintptr_t virt_addr) 
{
	struct AddressIndexer indexer;
	create_address_indexer(&indexer, virt_addr);

	struct PageTable *pt = (struct PageTable *)page_table;

	if (!pt->entries[indexer.pml4].present) {
		return 0;
	}

	pt = (struct PageTable *)((uintptr_t)pt->entries[indexer.pml4].addr << 12);

	if (!pt->entries[indexer.pml3].present) {
		return 0;
	}

	pt = (struct PageTable *)((uintptr_t)pt->entries[indexer.pml3].addr << 12);

	if (!pt->entries[indexer.pml2].present) {
		return 0;
	}

	pt = (struct PageTable *)((uintptr_t)pt->entries[indexer.pml2].addr << 12);

	if (!pt->entries[indexer.pml1].present) {
		return 0;
	}

	return pt->entries[indexer.pml1].addr + indexer.offset;
}

void vmm_init(struct stivale_mmap_entry *mmap, uint64_t mmap_count)
{
	pt_kernel = (struct PageTable *)pmm_allocz();

	for (size_t i = 0; i < mmap_count; i++) {
		// align the base address to the PAGE_SIZE
		uintptr_t base = mmap[i].base - mmap[i].base % PAGE_SIZE;
		uint64_t length = mmap[i].length;

		for (uint64_t k = 0; k < length + PAGE_SIZE; k += PAGE_SIZE) {
			vmm_map(pt_kernel, base + k, base + k);

			//dbg_printf("%x -> %x\n", base + k, 
			//					vmm_translate(pt_kernel, (uintptr_t)base + k));
		}
	}

	for (size_t i = 0; i < 0x100000000; i += PAGE_SIZE) {
		vmm_map(pt_kernel, i + HIGHER_HALF, i);

		//dbg_printf("%x -> %x\n", i + HIGHER_HALF, 
		//				vmm_translate(pt_kernel, (uintptr_t)i + HIGHER_HALF));
	}


	for (size_t i = 0; i < 0x80000000; i += PAGE_SIZE) {
		vmm_map(pt_kernel, i + KERNEL_PHYS_OFFSET, i);
		
		//dbg_printf("%x -> %x\n", i + KERNEL_PHYS_OFFSET, 
		//					vmm_translate(pt_kernel, (uintptr_t)i + KERNEL_PHYS_OFFSET));
	}

	kprintf("Loading page table...\n");

	cr3_set((uintptr_t)pt_kernel);
}

void vmm_map(struct PageTable *page_table, uintptr_t virt_addr, uintptr_t phys_addr) 
{
	struct AddressIndexer indexer;
	create_address_indexer(&indexer, virt_addr);

	struct PageTableEntry pte;
	pte = page_table->entries[indexer.pml4];

	struct PageTable *pdp;

	if (!pte.present) {
		pdp = (struct PageTable *)pmm_allocz();
		pte.addr = (uintptr_t)pdp >> 12;
		pte.present = 1;
		pte.writable = 1;
		page_table->entries[indexer.pml4] = pte;
	} else {
		pdp = (struct PageTable *)((uintptr_t)pte.addr << 12);
	}

	pte = pdp->entries[indexer.pml3];
	struct PageTable *pd;

	if (!pte.present) {
		pd = (struct PageTable *)pmm_allocz();
		pte.addr = (uintptr_t)pd >> 12;
		pte.present = 1;
		pte.writable = 1;
		pdp->entries[indexer.pml3] = pte;
	} else {
		pd = (struct PageTable *)((uintptr_t)pte.addr << 12);
	}

	pte = pd->entries[indexer.pml2];
	struct PageTable *pt;

	if (!pte.present) {
		pt = (struct PageTable *)pmm_allocz();
		pte.addr = (uintptr_t)pt >> 12;
		pte.present = 1;
		pte.writable = 1;
		pd->entries[indexer.pml2] = pte;
	} else {
		pt = (struct PageTable *)((uintptr_t)pte.addr << 12);
	}

	pte = pt->entries[indexer.pml1];
	pte.addr = (uint64_t)phys_addr;
	pte.present = 1;
	pte.writable = 1;
	pt->entries[indexer.pml1] = pte;
}

void vmm_unmap(struct PageTable *page_table, void *virt) 
{

}

struct PageTable *vmm_create_new_address_space() 
{
	return NULL;
}

void *cr3_read() 
{
	void *ret;
	asm("\t mov %%cr3,%0" : "=r"(ret));

	return ret;
}

void cr3_set(uintptr_t value) {
	asm ("mov %0, %%cr3" : : "r" (value));
}

void tlb_flush(unsigned long addr) {
	asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
