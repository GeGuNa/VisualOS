/*
 * File:		sys.c
 * *****************************************************************************
 * Copyright 2021 Scott Maday
 * Check the LICENSE file that came with this program for licensing terms
 */

#include "sys.h"


#include "shell/text.h"
#include "debug/debug.h"

static struct SysHandler s_handlers[] = {
	{ SYS_NUM_WRITE,	sys_write },
	{ SYS_NUM_EXIT,		sys_exit }
};


// Because this function gets called during precarious situations, we need the special attribute
// so we can save the state of any registers this function uses
__attribute__((no_caller_saved_registers)) uint64_t (*syshandler_get(uint64_t num))() {
	for(size_t i = 0; i < sizeof(s_handlers)/sizeof(struct SysHandler); i++) {
		if(s_handlers[i].num == num) {
			return s_handlers[i].handler;
		}
	}
	return syshandler_stub;
}

uint64_t syshandler_stub(){
	debug_options((struct DebugOptions){DEBUG_TYPE_WARNING, false}, "Sys stub reached\n");
	return 0;
}