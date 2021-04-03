/*
 * File:		kernel.h
 * Description:	Kernel-specific and auto-included definitions
 * *****************************************************************************
 * Copyright 2020-2021 Scott Maday
 * Check the LICENSE file that came with this program for licensing terms
 */

#pragma once

#ifndef KERNEL
#define KERNEL
#endif

// Rounds up [n] to the nearest [multiple]
#define ROUND_UP(n, multiple)	((n) % (multiple) == 0 ? (n) : (n) - (n) % (multiple) + (multiple))

// Returns [n]'s [bit] set to [value]
#define SET_BIT(n, bit, value)	((n) ^ (-(value) ^ (n)) & 1 << (bit))

// Returns [n]'s value of [bit]
#define GET_BIT(n, bit)			((1 << (bit) & (n)) > 0)

// Puts character [chr] in [buffer] at [position]
#define __PUTC(chr, buffer, position)								\
									{								\
										if(buffer != nullptr) {		\
											buffer[position] = chr;	\
										}							\
										position++;					\
									}

typedef struct LinkedNode {
	void*		data;
	struct LinkedNode*	next;
} LinkedNode;


// From kernel.ld linker script
extern void** _kernel_start;
extern void** _kernel_end;