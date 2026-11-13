// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
//
// Standalone host for MAME's ZiLOG Z80 core (BSD-3-Clause).
// The opcode helpers and z80.hxx execution loop are MAME's; this file only
// replaces the device/address-space layer so Neo Geo can run the core on
// every ABI, including 32-bit ARM.

#pragma once

#include <cstdint>
#include <cstddef>

namespace MameZ80
{

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;

static constexpr int CLEAR_LINE = 0;
static constexpr int ASSERT_LINE = 1;

union PAIR16
{
	struct { u8 l, h; } b;
	u16 w{};
};

struct MemOps
{
	u8 (*read)(u16){};
	void (*write)(u16, u8){};

	u8 read_byte(u16 addr) const { return read ? read(addr) : 0; }
	u8 read_interruptible(u16 addr) const { return read_byte(addr); }
	void write_interruptible(u16 addr, u8 value) const { if(write) write(addr, value); }
};

struct LineCallback
{
	constexpr bool isunset() const { return true; }
	void operator()(int) const {}
	void operator()(int, int, int) const {}
};

struct DebugIface
{
	void interrupt_hook(int, unsigned) {}
};

struct DebugDevice
{
	DebugIface dbg{};
	DebugIface *debug() { return &dbg; }
};

struct device_execute_interface
{
	static DebugDevice &device()
	{
		static DebugDevice d;
		return d;
	}
};

struct device_z80daisy_interface
{
	int z80daisy_irq_ack() { return 0xff; }
};

class z80_device
{
public:
	z80_device();

	void reset();
	void run(int cycles);
	void setIrq(bool asserted);
	void pulseNmi();
	void setMemHandlers(u8 (*read)(u16), void (*write)(u16, u8));
	void setIoHandlers(u8 (*read)(u16), void (*write)(u16, u8));
	void setIrqVectorCallback(u8 (*cb)());

	u16 pc() const { return m_pc.w; }
	u16 sp() const { return m_sp.w; }
	u16 af() const;
	u16 bc() const { return m_bc.w; }
	u16 de() const { return m_de.w; }
	u16 hl() const { return m_hl.w; }
	u16 ix() const { return m_ix.w; }
	u16 iy() const { return m_iy.w; }
	u16 wz() const { return m_wz.w; }
	u16 af2() const { return m_af2.w; }
	u16 bc2() const { return m_bc2.w; }
	u16 de2() const { return m_de2.w; }
	u16 hl2() const { return m_hl2.w; }
	u8 r() const { return m_r; }
	u8 r2() const { return m_r2; }
	u8 i() const { return m_i; }
	u8 im() const { return m_im; }
	u8 haltFlag() const { return m_halt; }
	u8 iff1() const { return m_iff1; }
	u8 iff2() const { return m_iff2; }
	u8 irqState() const { return m_irq_state; }
	u8 nmiState() const { return m_nmi_state; }

	void setRegisters(u16 pc, u16 sp, u16 af, u16 bc, u16 de, u16 hl,
		u16 ix, u16 iy, u16 wz, u16 af2, u16 bc2, u16 de2, u16 hl2,
		u8 r, u8 r2, u8 i, u8 im, u8 haltFlag, u8 iff1, u8 iff2, u8 irq, u8 nmi);

	u8 get_f();
	void set_f(u8 f);

private:
	void execute_run();
	void halt();
	void leave_halt();
	void inc(u8 &r);
	void dec(u8 &r);
	void rlca();
	void rrca();
	void rla();
	void rra();
	void add_a(u8 value);
	void adc_a(u8 value);
	void sub_a(u8 value);
	void sbc_a(u8 value);
	void neg();
	void daa();
	void and_a(u8 value);
	void or_a(u8 value);
	void xor_a(u8 value);
	void cp(u8 value);
	void exx();
	u8 rlc(u8 value);
	u8 rrc(u8 value);
	u8 rl(u8 value);
	u8 rr(u8 value);
	u8 sla(u8 value);
	u8 sra(u8 value);
	u8 sll(u8 value);
	u8 srl(u8 value);
	void bit(int bit, u8 value);
	void bit_hl(int bit, u8 value);
	void bit_xy(int bit, u8 value);
	u8 res(int bit, u8 value);
	u8 set(int bit, u8 value);
	void ei();
	void block_io_interrupted_flags();
	void illegal_1();
	void illegal_2();

	template <u8 Bit, bool State> void set_service_attention()
	{
		static_assert(Bit < 8);
		if(State) m_service_attention |= u8(1 << Bit);
		else m_service_attention &= u8(~(1 << Bit));
	}
	template <u8 Bit> bool get_service_attention() const
	{
		static_assert(Bit < 8);
		return m_service_attention & (1 << Bit);
	}

	u8 data_read(u16 addr);
	void data_write(u16 addr, u8 value);
	u8 stack_read(u16 addr) { return data_read(addr); }
	void stack_write(u16 addr, u8 value) { data_write(addr, value); }
	u8 opcode_read();
	u8 arg_read();

	bool access_to_be_redone() const { return false; }
	bool debugger_enabled() const { return false; }
	template <class T> void debugger_instruction_hook(T) const {}
	void debugger_wait_hook() const {}
	device_z80daisy_interface *daisy_get_irq_device() { return nullptr; }
	void daisy_call_reti_device() {}
	int standard_irq_callback(int, u16);
	const char *tag() const { return "z80"; }

	static constexpr u8 SA_BUSREQ = 0;
	static constexpr u8 SA_NMI_PENDING = 1;
	static constexpr u8 SA_IRQ_ON = 2;
	static constexpr u8 SA_HALT = 3;
	static constexpr u8 SA_AFTER_EI = 4;
	static constexpr u8 SA_AFTER_LDAIR = 5;

	MemOps m_data{};
	MemOps m_opcodes{};
	MemOps m_args{};
	MemOps m_io{};
	LineCallback m_irqack_cb{};
	LineCallback m_refresh_cb{};
	LineCallback m_nomreq_cb{};
	LineCallback m_halt_cb{};
	LineCallback m_busack_cb{};
	u8 (*m_irqVector)(){};

	u8 m_service_attention{};
	PAIR16 m_prvpc{};
	PAIR16 m_pc{};
	PAIR16 m_sp{};
	PAIR16 m_af{};
	PAIR16 m_bc{};
	PAIR16 m_de{};
	PAIR16 m_hl{};
	PAIR16 m_ix{};
	PAIR16 m_iy{};
	PAIR16 m_wz{};
	PAIR16 m_af2{};
	PAIR16 m_bc2{};
	PAIR16 m_de2{};
	PAIR16 m_hl2{};
	u8 m_r{};
	u8 m_r2{};
	bool m_iff1{};
	bool m_iff2{};
	u8 m_halt{};
	u8 m_im{};
	u8 m_i{};
	u8 m_nmi_state{};
	u8 m_irq_state{};
	int m_wait_state{};
	int m_busreq_state{};
	u8 m_busack_state{};
	u16 m_ea{};

	struct Flags
	{
		u8 s_val{};
		u8 z_val{};
		u8 yx_val{};
		u8 h_val{};
		u8 pv_val{};
		bool n{};
		bool c{};
		u8 q{};
		u8 qtemp{};
		u8 s() const { return s_val & 0x80; }
		u8 z() const { return z_val ? 0 : 0x40; }
		u8 yx() const { return yx_val & 0x28; }
		u8 h() const { return h_val & 0x10; }
		u8 pv() const
		{
			u8 val = pv_val;
			val ^= val >> 4;
			val ^= val << 2;
			val ^= val >> 1;
			return ~val & 0x04;
		}
	} m_f{};

	int m_icount{};
	int m_tmp_irq_vector{};
	PAIR16 m_shared_data{};
	PAIR16 m_shared_data2{};
	PAIR16 m_irtmp{};
	u32 m_ref{0xffff00};
	u8 m_m1_cycles{4};
	u8 m_mreq_cycles{3};
	u8 m_iorq_cycles{4};
};

#define LOGMASKED(...) do {} while(0)
#define logerror(...) do {} while(0)
#define LOG_INT 0
#define LOG_UNDOC 0

}
