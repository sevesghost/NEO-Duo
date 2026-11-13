/* This file is part of Gngeo.
 * Copyright (C) the Gngeo team.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// 68000 glue for Karl Stenerud's Musashi (MIT), used on every ABI.
// Save states written by the previous Musashi 3.32 / Cyclone cores do not load.

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif
#include <imagine/util/macros.h>

extern "C"
{
	#include "memory.h"
	#include "emu.h"
	#include "state.h"
	#include <musashi/m68k.h>
}

static int cyclesThisSlice;

extern "C" unsigned int m68k_read_memory_8(unsigned int address)
{
	unsigned int a = address & 0xffffff;
	unsigned int b = (a >> 16) & 0xf;
	switch(a >> 20)
	{
		case 0x0: return mem68k_fetch_cpu_byte(a) & 0xff;
		case 0x1: return mem68k_fetch_ram_byte(a) & 0xff;
		case 0x2: return mem68k_fetch_bk_normal_byte(a) & 0xff;
		case 0x3:
			if(b == 0xc) return mem68k_fetch_video_byte(a) & 0xff;
			if(b == 0) return mem68k_fetch_ctl1_byte(a) & 0xff;
			if(b == 4) return mem68k_fetch_ctl2_byte(a) & 0xff;
			if(b == 8) return mem68k_fetch_ctl3_byte(a) & 0xff;
			if(b == 2) return mem68k_fetch_coin_byte(a) & 0xff;
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: return mem68k_fetch_pal_byte(a) & 0xff;
		case 0x8:
			if(b == 0) return mem68k_fetch_memcrd_byte(a) & 0xff;
			break;
		case 0xc: return mem68k_fetch_bios_byte(a) & 0xff;
		case 0xd: return mem68k_fetch_sram_byte(a) & 0xff;
	}
	return mem68k_fetch_invalid_byte(a) & 0xff;
}

extern "C" unsigned int m68k_read_memory_16(unsigned int address)
{
	unsigned int a = address & 0xffffff;
	unsigned int b = (a >> 16) & 0xf;
	switch(a >> 20)
	{
		case 0x0: return mem68k_fetch_cpu_word(a) & 0xffff;
		case 0x1: return mem68k_fetch_ram_word(a) & 0xffff;
		case 0x2: return mem68k_fetch_bk_normal_word(a) & 0xffff;
		case 0x3:
			if(b == 0xc) return mem68k_fetch_video_word(a) & 0xffff;
			if(b == 0) return mem68k_fetch_ctl1_word(a) & 0xffff;
			if(b == 4) return mem68k_fetch_ctl2_word(a) & 0xffff;
			if(b == 8) return mem68k_fetch_ctl3_word(a) & 0xffff;
			if(b == 2) return mem68k_fetch_coin_word(a) & 0xffff;
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: return mem68k_fetch_pal_word(a) & 0xffff;
		case 0x8:
			if(b == 0) return mem68k_fetch_memcrd_word(a) & 0xffff;
			break;
		case 0xc: return mem68k_fetch_bios_word(a) & 0xffff;
		case 0xd: return mem68k_fetch_sram_word(a) & 0xffff;
	}
	return mem68k_fetch_invalid_word(a) & 0xffff;
}

extern "C" unsigned int m68k_read_memory_32(unsigned int address)
{
	unsigned int a = address & 0xffffff;
	unsigned int b = (a >> 16) & 0xf;
	switch(a >> 20)
	{
		case 0x0: return mem68k_fetch_cpu_long(a);
		case 0x1: return mem68k_fetch_ram_long(a);
		case 0x2: return mem68k_fetch_bk_normal_long(a);
		case 0x3:
			if(b == 0xc) return mem68k_fetch_video_long(a);
			if(b == 0) return mem68k_fetch_ctl1_long(a);
			if(b == 4) return mem68k_fetch_ctl2_long(a);
			if(b == 8) return mem68k_fetch_ctl3_long(a);
			if(b == 2) return mem68k_fetch_coin_long(a);
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: return mem68k_fetch_pal_long(a);
		case 0x8:
			if(b == 0) return mem68k_fetch_memcrd_long(a);
			break;
		case 0xc: return mem68k_fetch_bios_long(a);
		case 0xd: return mem68k_fetch_sram_long(a);
	}
	return mem68k_fetch_invalid_long(a);
}

extern "C" void m68k_write_memory_8(unsigned int address, unsigned int value)
{
	unsigned int a = address & 0xffffff;
	unsigned int b = (a >> 16) & 0xf;
	unsigned int d = value & 0xff;
	switch(a >> 20)
	{
		case 0x1: mem68k_store_ram_byte(a, d); return;
		case 0x2: if(b == 0xf) { mem68k_store_bk_normal_byte(a, d); return; } break;
		case 0x3:
			if(b == 0xc) { mem68k_store_video_byte(a, d); return; }
			if(b == 8) { mem68k_store_pd4990_byte(a, d); return; }
			if(b == 2) { mem68k_store_z80_byte(a, d); return; }
			if(b == 0xa) { mem68k_store_setting_byte(a, d); return; }
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: mem68k_store_pal_byte(a, d); return;
		case 0x8: if(b == 0) { mem68k_store_memcrd_byte(a, d); return; } break;
		case 0xd: mem68k_store_sram_byte(a, d); return;
	}
	mem68k_store_invalid_byte(a, d);
}

extern "C" void m68k_write_memory_16(unsigned int address, unsigned int value)
{
	unsigned int a = address & 0xffffff;
	unsigned int b = (a >> 16) & 0xf;
	unsigned int d = value & 0xffff;
	switch(a >> 20)
	{
		case 0x1: mem68k_store_ram_word(a, d); return;
		case 0x2: if(b == 0xf) { mem68k_store_bk_normal_word(a, d); return; } break;
		case 0x3:
			if(b == 0xc) { mem68k_store_video_word(a, d); return; }
			if(b == 8) { mem68k_store_pd4990_word(a, d); return; }
			if(b == 2) { mem68k_store_z80_word(a, d); return; }
			if(b == 0xa) { mem68k_store_setting_word(a, d); return; }
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: mem68k_store_pal_word(a, d); return;
		case 0x8: if(b == 0) { mem68k_store_memcrd_word(a, d); return; } break;
		case 0xd: mem68k_store_sram_word(a, d); return;
	}
	mem68k_store_invalid_word(a, d);
}

extern "C" void m68k_write_memory_32(unsigned int address, unsigned int value)
{
	unsigned int a = address & 0xffffff;
	unsigned int b = (a >> 16) & 0xf;
	switch(a >> 20)
	{
		case 0x1: mem68k_store_ram_long(a, value); return;
		case 0x2: if(b == 0xf) { mem68k_store_bk_normal_long(a, value); return; } break;
		case 0x3:
			if(b == 0xc) { mem68k_store_video_long(a, value); return; }
			if(b == 8) { mem68k_store_pd4990_long(a, value); return; }
			if(b == 2) { mem68k_store_z80_long(a, value); return; }
			if(b == 0xa) { mem68k_store_setting_long(a, value); return; }
			break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: mem68k_store_pal_long(a, value); return;
		case 0x8: if(b == 0) { mem68k_store_memcrd_long(a, value); return; } break;
		case 0xd: mem68k_store_sram_long(a, value); return;
	}
	mem68k_store_invalid_long(a, value);
}

CLINK void cpu_68k_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	bankaddress = 0;
	if(memory.rom.cpu_m68k.size > 0x100000)
		bankaddress = 0x100000;
}

CLINK void cpu_68k_reset(void)
{
	m68k_pulse_reset();
}

CLINK int cpu_68k_run(Uint32 nb_cycle)
{
	if(conf.raster)
	{
		int used = m68k_execute(int(nb_cycle));
		cyclesThisSlice = used;
		return used - int(nb_cycle);
	}
	memory.vid.current_line = 0;
	const int cyclesPerLine = int(nb_cycle / 264.0);
	int extra = 0;
	for(int i = 0; i < 265; i++)
	{
		int used = m68k_execute(cyclesPerLine);
		extra = used - cyclesPerLine;
		memory.vid.current_line++;
	}
	cyclesThisSlice = int(nb_cycle) + extra;
	return 0;
}

CLINK void cpu_68k_interrupt(int level)
{
	m68k_set_irq(level < 0 ? 0 : unsigned(level));
}

CLINK Uint32 cpu_68k_getpc(void)
{
	return m68k_get_reg(nullptr, M68K_REG_PC);
}

CLINK int cpu_68k_getcycle(void)
{
	return cyclesThisSlice;
}

CLINK void cpu_68k_mkstate(Stream *gzf, int mode)
{
	auto transfer = [&](m68k_register_t reg, int bytes)
	{
		unsigned int value = 0;
		if(mode == STWRITE)
			value = m68k_get_reg(nullptr, reg);
		mkstate_data(gzf, &value, bytes, mode);
		if(mode == STREAD)
			m68k_set_reg(reg, value);
	};
	int cycles = cyclesThisSlice;
	mkstate_data(gzf, &cycles, sizeof(cycles), mode);
	if(mode == STREAD)
		cyclesThisSlice = cycles;
	transfer(M68K_REG_D0, 4); transfer(M68K_REG_D1, 4);
	transfer(M68K_REG_D2, 4); transfer(M68K_REG_D3, 4);
	transfer(M68K_REG_D4, 4); transfer(M68K_REG_D5, 4);
	transfer(M68K_REG_D6, 4); transfer(M68K_REG_D7, 4);
	transfer(M68K_REG_A0, 4); transfer(M68K_REG_A1, 4);
	transfer(M68K_REG_A2, 4); transfer(M68K_REG_A3, 4);
	transfer(M68K_REG_A4, 4); transfer(M68K_REG_A5, 4);
	transfer(M68K_REG_A6, 4); transfer(M68K_REG_A7, 4);
	transfer(M68K_REG_PC, 4);
	transfer(M68K_REG_SR, 2);
	transfer(M68K_REG_USP, 4);
	transfer(M68K_REG_ISP, 4);
}

CLINK void cpu_68k_disassemble(int, int) {}
CLINK void cpu_68k_dumpreg(void) {}
CLINK int cpu_68k_run_step(void) { return m68k_execute(1); }
CLINK void cpu_68k_fill_state(M68K_STATE *) {}
CLINK void cpu_68k_set_state(M68K_STATE *) {}
CLINK int cpu_68k_debuger(void (*)(int, void *, void *, void *), void (*)(void)) { return 0; }
