/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */


#include <gngeo-config.h>
#include <imagine/util/utility.hh>

extern "C"
{
	#include <gngeo/emu.h>
	#include <gngeo/memory.h>
	#include <gngeo/state.h>
}

#include <z80/z80_core.hh>

static MameZ80::z80_device Z80;
uint8_t mame_z80mem[0x10000];

static uint8_t z80ReadMem(uint16_t addr) { return mame_z80mem[addr]; }
static void z80WriteMem(uint16_t addr, uint8_t data) { mame_z80mem[addr] = data; }
static uint8_t z80ReadPort(uint16_t port) { return z80_port_read(port); }
static void z80WritePort(uint16_t port, uint8_t data) { z80_port_write(port, data); }
static const uint8_t *z80map1, *z80map2, *z80map3, *z80map4;

/* cpu interface implementation */
CLINK void cpu_z80_switchbank(uint8_t bank, uint16_t PortNo)
{
	if(bank <= 3)
		z80_bank[bank] = PortNo;
	switch (bank)
	{
		case 0:
			z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f));
			if ((0x4000 * ((PortNo >> 8) & 0x0f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0x8000, z80map1, 0x4000);
			break;
		case 1:
			z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f));
			if ((0x2000 * ((PortNo >> 8) & 0x1f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0xc000, z80map2, 0x2000);
			break;
		case 2:
			z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f));
			if ((0x1000 * ((PortNo >> 8) & 0x3f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0xe000, z80map3, 0x1000);
			break;
		case 3:
			z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f));
			if ((0x0800 * ((PortNo >> 8) & 0x7f))<memory.rom.cpu_z80.size)
				memcpy(mame_z80mem + 0xf000, z80map4, 0x0800);
			break;
	}
}

template<class T>
static int makeStateData(Stream *gzf, T &data, int mode)
{
	return mkstate_data(gzf, &data, sizeof(T), mode);
}

CLINK void cpu_z80_mkstate(Stream *gzf,int mode)
{
	// Register image for the BSD-3 MAME Z80. Older Portable Z80 states are
	// rejected by the state header before this runs.
	uint16_t pc{}, sp{}, af{}, bc{}, de{}, hl{}, ix{}, iy{}, wz{}, af2{}, bc2{}, de2{}, hl2{};
	uint8_t r{}, r2{}, i{}, im{}, halt{}, iff1{}, iff2{}, irq{}, nmi{};
	if(mode == STWRITE)
	{
		pc = Z80.pc(); sp = Z80.sp(); af = Z80.af(); bc = Z80.bc(); de = Z80.de(); hl = Z80.hl();
		ix = Z80.ix(); iy = Z80.iy(); wz = Z80.wz();
		af2 = Z80.af2(); bc2 = Z80.bc2(); de2 = Z80.de2(); hl2 = Z80.hl2();
		r = Z80.r(); r2 = Z80.r2(); i = Z80.i(); im = Z80.im(); halt = Z80.haltFlag();
		iff1 = Z80.iff1(); iff2 = Z80.iff2(); irq = Z80.irqState(); nmi = Z80.nmiState();
	}
	makeStateData(gzf, pc, mode); makeStateData(gzf, sp, mode); makeStateData(gzf, af, mode);
	makeStateData(gzf, bc, mode); makeStateData(gzf, de, mode); makeStateData(gzf, hl, mode);
	makeStateData(gzf, ix, mode); makeStateData(gzf, iy, mode); makeStateData(gzf, wz, mode);
	makeStateData(gzf, af2, mode); makeStateData(gzf, bc2, mode); makeStateData(gzf, de2, mode);
	makeStateData(gzf, hl2, mode);
	makeStateData(gzf, r, mode); makeStateData(gzf, r2, mode); makeStateData(gzf, i, mode);
	makeStateData(gzf, im, mode); makeStateData(gzf, halt, mode);
	makeStateData(gzf, iff1, mode); makeStateData(gzf, iff2, mode);
	makeStateData(gzf, irq, mode); makeStateData(gzf, nmi, mode);
	if(mode == STREAD)
	{
		Z80.setRegisters(pc, sp, af, bc, de, hl, ix, iy, wz, af2, bc2, de2, hl2,
			r, r2, i, im, halt, iff1, iff2, irq, nmi);
	}

	mkstate_data(gzf, mame_z80mem, 0x10000, mode);
	if (mode==STREAD)
	{
		z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((z80_bank[0] >> 8) & 0x0f));
		z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((z80_bank[1] >> 8) & 0x1f));
		z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((z80_bank[2] >> 8) & 0x3f));
		z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((z80_bank[3] >> 8) & 0x7f));
	}
}

CLINK void cpu_z80_init(void)
{
	Z80.setMemHandlers(z80ReadMem, z80WriteMem);
	Z80.setIoHandlers(z80ReadPort, z80WritePort);
	Z80.setIrqVectorCallback([]() -> uint8_t { return 0xff; });

	/* bank initalisation */
	z80map1 = memory.rom.cpu_z80.p + 0x8000;
	z80map2 = memory.rom.cpu_z80.p + 0xc000;
	z80map3 = memory.rom.cpu_z80.p + 0xe000;
	z80map4 = memory.rom.cpu_z80.p + 0xf000;

	z80_bank[0]=0x8000;
	z80_bank[1]=0xc000;
	z80_bank[2]=0xe000;
	z80_bank[3]=0xf000;

	memcpy(mame_z80mem, memory.rom.cpu_z80.p, 0xf800);
	Z80.reset();
}

CLINK void cpu_z80_run(int cycles)
{
	Z80.run(cycles);
}

CLINK void cpu_z80_nmi(void)
{
	Z80.pulseNmi();
}

CLINK void cpu_z80_raise_irq(int)
{
	Z80.setIrq(true);
}

CLINK void cpu_z80_lower_irq(void)
{
	Z80.setIrq(false);
}

uint16_t cpu_z80_get_pc(void)
{
	return Z80.pc();
}
