/*
 * File:		gdt.h
 * Description:	x86_64 global descriptor table
 * *****************************************************************************
 * Copyright 2020-2021 Scott Maday
 * Check the LICENSE file that came with this program for licensing terms
 */

#pragma once

#define GDT_SIZE		8	// the null segment + 4 code / data segments + TSS descriptor is the size of 2 GDT entries
#define GDT_TSS_SIZE	1	// only 1 TSS

#define GDT_TSS_STACK_SIZE		4096
#define GDT_TSS_RING_STACKS_NUM	3
#define GDT_TSS_IST_STACKS_NUM	7


struct GDTEntry {
	uint16_t	limit_low;	// Limit (lower 16 bits)
	uint16_t	base_low;	// Base (lower 16 bits)
	uint8_t		base_mid;	// Base (next 8 bits)
	uint8_t		access;		// present(1), ring(2), type(1), executable(1), conforming(1), writable(1), accessed(1)
	uint8_t		flags;		// granularity(1), protected-mode(1), long-mode(1), available(1), limit_high(4)
	uint8_t		base_high;	// Base (highest 8 bits)
} __attribute__((packed));

struct GDTDescriptor {
	uint16_t			size;	// max size of 65 bit gdt, minus 1
	struct GDTEntry*	offset;	// linear address of the table
} __attribute__((packed));

enum GDTAccess {
	GDT_ACCESS_WRITABLE		= 0b00000010,
	GDT_ACCESS_TYPE			= 0b00010000,
	GDT_ACCESS_PRESENT		= 0b10000000,
	GDT_ACCESS_EXECUTABLE	= 0b00001000,
	GDT_ACCESS_DPL			= 0b01100000,	// segment privilege level is ring 3
};

enum GDTFlags {
	GDT_FLAG_LONG_MODE			= 0b00100000,
	GDT_FLAG_PAGE_GRANULARITY	= 0b10000000,
};

struct TSS {
	uint32_t	reserved0;
	uint64_t	rsp0;		// ring stacks
	uint64_t	rsp1;
	uint64_t	rsp2;
	uint64_t	reserved1;
	uint64_t	ist1;		// interrupt stack tables
	uint64_t	ist2;
	uint64_t	ist3;
	uint64_t	ist4;
	uint64_t	ist5;
	uint64_t	ist6;
	uint64_t	ist7;
	uint64_t	reserved2;
	uint16_t	reserved3;
	uint16_t	io_map_base;
} __attribute__((packed));

struct TSSDescriptor {
	uint16_t	limit_low;	// Segment limit (lower 16 bits)
	uint16_t	base_low;	// Base (lower 16 bits)
	uint8_t		base_mid;	// Base (next 8 bits)
	uint8_t		access;		// present(1), ring(2), reserved(1), type(4)
	uint8_t		flags;		// granularity(1), reserved(2), available(1), limit_high(4)
	uint8_t		base_mid2;	// Base (next 8 bits)
	uint32_t	base_high;	// Base (highest 32 bits)
	uint32_t	reserved;
} __attribute__((packed));

// Carrier for the gdt at runtime
struct GDTBlock {
	struct GDTDescriptor	gdt_descriptor;
	struct GDTEntry			gdt[GDT_SIZE];
	struct TSS				tss[GDT_TSS_SIZE];
};


// Sets the [gdt_block] gdt entry pointer and [gdt_block] gdt descriptor
void gdt_init(struct GDTBlock* gdt_block);

// Sets the [gdt_block] gdt entry at [index] with flags of [access] and [flags]
void gdt_set_entry(struct GDTBlock* gdt_block, size_t index, enum GDTAccess access, enum GDTFlags flags);

// Sets up a new task state segment and at [gdt_index] on the [gdt_block] gdt for the descriptor
void gdt_set_tss(struct GDTBlock* gdt_block, size_t gdt_offset);

// Loads the gdt described at [gdt_descriptor], this will also clear interrupts.
void gdt_load(struct GDTDescriptor* gdt_descriptor);

// Sets the kernel stack to [stack] in [gdt_block]
void gdt_set_ring0_stack(struct GDTBlock* gdt_block, void* stack);

// Gets TSS ring0 stack in [gdt_block]
void* gdt_get_ring0_stack(struct GDTBlock* gdt_block);