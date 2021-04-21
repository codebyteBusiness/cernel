#pragma once

#include <stdint.h>
#include <stivale/stivale.h>

#define PAGE_SIZE 4096

struct usable_ram {
	uint64_t base;
	uint64_t page_count;
	uint64_t length;
};

void pmm_init(struct stivale_mmap_entry *memory_map_addr, uint32_t count);
void *pmm_alloc();
void *pmm_allocz();
void pmm_free(void * page_addr);
