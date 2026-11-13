/*
 * Neo Geo calendar chip (NEC uPD4990A) on the three-wire control port.
 *
 * The shift register and strobe follow the serial model in MAME's
 * BSD-3-Clause upd1990a device (Copyright Curt Coder, hap, and the MAME
 * team): a clock edge walks a right-shifting register, and the strobe
 * edge latches the command nibble that arrived on the data pin.
 * Timekeeping, the retrace-timed TP output, and this file are original.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include <time.h>
#include <string.h>
#include <stdint.h>
#include "pd4990a.h"
#include "state.h"

struct pd4990a_s pd4990a;

static int cmd_nibble;

enum
{
	MODE_HOLD = 0,
	MODE_SHIFT = 1,
	MODE_TIME_SET = 2,
	MODE_TIME_READ = 3,
	MODE_TP_RETRACE = 7,
	MODE_TP_30 = 8
};

#define DATA_BIT  0x1
#define CLOCK_BIT 0x2
#define END_BIT   0x4

static int from_bcd(int value)
{
	return ((value >> 4) & 0xf) * 10 + (value & 0xf);
}

static int to_bcd(int value)
{
	if(value < 0)
		value = 0;
	return ((value / 10) << 4) | (value % 10);
}

static int bcd_tick(int *field, int limit)
{
	int next = from_bcd(*field) + 1;
	if(next >= limit)
	{
		*field = 0;
		return 1;
	}
	*field = to_bcd(next);
	return 0;
}

static void present_lsb(void)
{
	pd4990a.outputbit = pd4990a.shiftlo & 1;
}

static void shift_bit(int bit)
{
	uint32_t cross = pd4990a.shifthi & 1u;
	pd4990a.shiftlo = (pd4990a.shiftlo >> 1) | (cross << 31);
	pd4990a.shifthi = ((pd4990a.shifthi >> 1) & 0x7ffffu) | ((uint32_t)(bit & 1) << 19);
	pd4990a.outputbit = pd4990a.shiftlo & 1;
	pd4990a.bitno++;
}

static void load_shift_from_time(void)
{
	uint64_t packed = 0;
	packed |= (uint64_t)(pd4990a.seconds & 0xff);
	packed |= (uint64_t)(pd4990a.minutes & 0xff) << 8;
	packed |= (uint64_t)(pd4990a.hours & 0xff) << 16;
	packed |= (uint64_t)(pd4990a.days & 0xff) << 24;
	packed |= (uint64_t)(((pd4990a.month & 0xf) << 4) | (pd4990a.weekday & 0xf)) << 32;
	packed |= (uint64_t)(pd4990a.year & 0xff) << 40;
	pd4990a.shiftlo = (uint32_t)packed;
	pd4990a.shifthi = (uint32_t)(packed >> 32);
	pd4990a.bitno = 0;
	present_lsb();
}

static void store_time_from_shift(void)
{
	uint64_t packed = pd4990a.shiftlo | ((uint64_t)pd4990a.shifthi << 32);
	pd4990a.seconds = (int)(packed & 0xff);
	pd4990a.minutes = (int)((packed >> 8) & 0xff);
	pd4990a.hours = (int)((packed >> 16) & 0xff);
	pd4990a.days = (int)((packed >> 24) & 0xff);
	pd4990a.weekday = (int)((packed >> 32) & 0xf);
	pd4990a.month = (int)((packed >> 36) & 0xf);
	if(pd4990a.month < 1 || pd4990a.month > 12)
		pd4990a.month = 1;
	pd4990a.year = (int)((packed >> 40) & 0xff);
}

static void latch_command(void)
{
	int command = cmd_nibble & 0xf;
	pd4990a.writing = (char)command;
	switch(command)
	{
		case MODE_SHIFT:
			pd4990a.reading = 0;
			present_lsb();
			break;
		case MODE_TIME_SET:
			store_time_from_shift();
			pd4990a.reading = 0;
			break;
		case MODE_TIME_READ:
			load_shift_from_time();
			pd4990a.reading = 1;
			break;
		case MODE_TP_RETRACE:
			pd4990a.maxwaits = 1;
			break;
		case MODE_TP_30:
			pd4990a.maxwaits = 30;
			break;
		default:
			if(command >= 4 && command <= 6)
				pd4990a.maxwaits = 1 << (command - 4);
			else if(command == 9)
				pd4990a.maxwaits = 60;
			present_lsb();
			break;
	}
}

void pd4990a_addretrace(void)
{
	if(pd4990a.maxwaits < 1)
		pd4990a.maxwaits = 1;
	if(++pd4990a.testwaits >= pd4990a.maxwaits)
	{
		pd4990a.testbit ^= 1;
		pd4990a.testwaits = 0;
	}
	if(++pd4990a.retraces < 60)
		return;
	pd4990a.retraces = 0;
	if(bcd_tick(&pd4990a.seconds, 60) && bcd_tick(&pd4990a.minutes, 60) && bcd_tick(&pd4990a.hours, 24))
		pd4990a_increment_day();
}

void pd4990a_mkstate(Stream *gzf, int mode)
{
	mkstate_data(gzf, &pd4990a, sizeof(pd4990a), mode);
}

void pd4990a_init(void)
{
	time_t now;
	struct tm *today;
	memset(&pd4990a, 0, sizeof(pd4990a));
	cmd_nibble = 0;
	pd4990a.maxwaits = 1;
	pd4990a.days = 0x09;
	pd4990a.month = 9;
	pd4990a.year = 0x73;
	pd4990a.weekday = 1;
	time(&now);
	today = localtime(&now);
	if(!today)
		return;
	pd4990a.seconds = to_bcd(today->tm_sec);
	pd4990a.minutes = to_bcd(today->tm_min);
	pd4990a.hours = to_bcd(today->tm_hour);
	pd4990a.days = to_bcd(today->tm_mday);
	pd4990a.month = today->tm_mon + 1;
	pd4990a.year = to_bcd(today->tm_year % 100);
	pd4990a.weekday = today->tm_wday;
}

void pd4990a_increment_month(void)
{
	int year;
	if(++pd4990a.month <= 12)
		return;
	pd4990a.month = 1;
	year = from_bcd(pd4990a.year) + 1;
	if(year > 99)
		year = 0;
	pd4990a.year = to_bcd(year);
}

void pd4990a_increment_day(void)
{
	static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int month = pd4990a.month;
	int year = from_bcd(pd4990a.year);
	int length;
	int day;
	if(month < 1 || month > 12)
		month = 1;
	length = days_in_month[month];
	if(month == 2 && (year % 4) == 0)
		length = 29;
	day = from_bcd(pd4990a.days) + 1;
	pd4990a.weekday = (pd4990a.weekday + 1) % 7;
	if(day > length)
	{
		pd4990a.days = to_bcd(1);
		pd4990a_increment_month();
	}
	else
		pd4990a.days = to_bcd(day);
}

int read_4990_testbit(void)
{
	return pd4990a.testbit;
}

int read_4990_databit(void)
{
	return pd4990a.outputbit;
}

void write_4990_control_w(Uint32 address, Uint32 data)
{
	int clock;
	int strobe;
	int mode;
	(void)address;
	data &= 7;
	clock = data & CLOCK_BIT;
	strobe = data & END_BIT;
	if(pd4990a.clock_line && !clock)
	{
		int bit = (data & DATA_BIT) ? 1 : 0;
		cmd_nibble = ((cmd_nibble >> 1) | (bit << 3)) & 0xf;
		mode = (unsigned char)pd4990a.writing;
		if(mode == MODE_SHIFT || mode == MODE_TIME_SET || pd4990a.reading)
			shift_bit(bit);
	}
	if(pd4990a.command_line && !strobe)
		latch_command();
	pd4990a.clock_line = clock;
	pd4990a.command_line = strobe;
}
