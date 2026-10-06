// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Spacemit
 */

#include "k3_ddr.h"

/* Group type enumeration */
typedef enum {
	DDRPHY_GROUP_SCATTERED  = 0, /* Explicit offset list */
	DDRPHY_GROUP_SEQUENTIAL = 1  /* Contiguous offsets: start, start+1, ..., start+count-1 */
} ddrphy_group_type_t;

/* Unified register group descriptor */
typedef struct {
	uint32_t           base_addr; /* Base address */
	uint16_t           count;     /* Number of registers */
	uint8_t            type;      /* SCATTERED or SEQUENTIAL */
	union {
		const uint16_t *offsets;      /* SCATTERED: pointer to offset array */
		uint32_t        start_offset; /* SEQUENTIAL: first offset (step=1) */
	};
} ddrphy_reg_group_t;

/* Group 0: base 0x10000 - 11 registers */
static const uint16_t ddrphy_offsets_group_0[] = {
	0xA3,	/* 32'h100a3 */
	0x56,	/* 32'h10056 */
	0xD9,	/* 32'h100d9 */
	0x80,	/* 32'h10080 */
	0x81,	/* 32'h10081 */
	0x82,	/* 32'h10082 */
	0x83,	/* 32'h10083 */
	0x84,	/* 32'h10084 */
	0x85,	/* 32'h10085 */
	0x86,	/* 32'h10086 */
	0x87	/* 32'h10087 */
};

/* Group 1: base 0x11000 - 11 registers (reuses ddrphy_offsets_group_0) */

/* Group 2: base 0x12000 - 11 registers (reuses ddrphy_offsets_group_0) */

/* Group 3: base 0x13000 - 11 registers (reuses ddrphy_offsets_group_0) */

/* Group 4: base 0x30000 - 11 registers */
static const uint16_t ddrphy_offsets_group_4[] = {
	0xAD,	/* 32'h300ad */
	0xAE,	/* 32'h300ae */
	0xAC,	/* 32'h300ac */
	0x90,	/* 32'h30090 */
	0x91,	/* 32'h30091 */
	0x92,	/* 32'h30092 */
	0x93,	/* 32'h30093 */
	0x94,	/* 32'h30094 */
	0x95,	/* 32'h30095 */
	0x96,	/* 32'h30096 */
	0xEEF	/* 32'h30eef */
};

/* Group 5: base 0x31000 - 11 registers (reuses ddrphy_offsets_group_4) */

/* Group 6: base 0xC0000 - 17 registers */
static const uint16_t ddrphy_offsets_group_6[] = {
	0x86,	/* 32'hc0086 */
	0xF0,	/* 32'hc00f0 */
	0xF1,	/* 32'hc00f1 */
	0xF2,	/* 32'hc00f2 */
	0xF3,	/* 32'hc00f3 */
	0xF4,	/* 32'hc00f4 */
	0xF5,	/* 32'hc00f5 */
	0xF6,	/* 32'hc00f6 */
	0xF7,	/* 32'hc00f7 */
	0xF8,	/* 32'hc00f8 */
	0xF9,	/* 32'hc00f9 */
	0xFA,	/* 32'hc00fa */
	0xFB,	/* 32'hc00fb */
	0xFC,	/* 32'hc00fc */
	0xFD,	/* 32'hc00fd */
	0xFE,	/* 32'hc00fe */
	0xFF	/* 32'hc00ff */
};

/* Group 7: base 0x20000 - 10 registers */
static const uint16_t ddrphy_offsets_group_7[] = {
	0x90,	/* 32'h20090 */
	0x51,	/* 32'h20051 */
	0x300,	/* 32'h20300 */
	0x303,	/* 32'h20303 */
	0x302,	/* 32'h20302 */
	0x328,	/* 32'h20328 */
	0x301,	/* 32'h20301 */
	0x30B,	/* 32'h2030b */
	0x77,	/* 32'h20077 */
	0x49	/* 32'h20049 */
};

/* Group 8: base 0x90000 - 1 registers */
static const uint16_t ddrphy_offsets_group_8[] = {
	0x8FF	/* 32'h908ff */
};

/* Group 9: base 0x20000 - 38 registers */
static const uint16_t ddrphy_offsets_group_9[] = {
	0x71,	/* 32'h20071 */
	0x84,	/* 32'h20084 */
	0x85,	/* 32'h20085 */
	0xA5,	/* 32'h200a5 */
	0xC6,	/* 32'h200c6 */
	0x184,	/* 32'h20184 */
	0x185,	/* 32'h20185 */
	0x188,	/* 32'h20188 */
	0x189,	/* 32'h20189 */
	0x18A,	/* 32'h2018a */
	0x200,	/* 32'h20200 */
	0x305,	/* 32'h20305 */
	0x306,	/* 32'h20306 */
	0x307,	/* 32'h20307 */
	0x308,	/* 32'h20308 */
	0x309,	/* 32'h20309 */
	0x30A,	/* 32'h2030a */
	0x30C,	/* 32'h2030c */
	0x30D,	/* 32'h2030d */
	0x30E,	/* 32'h2030e */
	0x30F,	/* 32'h2030f */
	0x318,	/* 32'h20318 */
	0x319,	/* 32'h20319 */
	0x31A,	/* 32'h2031a */
	0x31B,	/* 32'h2031b */
	0x31C,	/* 32'h2031c */
	0x31D,	/* 32'h2031d */
	0x31E,	/* 32'h2031e */
	0x31F,	/* 32'h2031f */
	0x320,	/* 32'h20320 */
	0x321,	/* 32'h20321 */
	0x322,	/* 32'h20322 */
	0x323,	/* 32'h20323 */
	0x324,	/* 32'h20324 */
	0x325,	/* 32'h20325 */
	0x326,	/* 32'h20326 */
	0x327,	/* 32'h20327 */
	0x19	/* 32'h20019 */
};

/* Group 10: base 0x90000 - 11 registers */
static const uint16_t ddrphy_offsets_group_10[] = {
	0x802,	/* 32'h90802 */
	0x8F0,	/* 32'h908f0 */
	0x8F1,	/* 32'h908f1 */
	0x8F2,	/* 32'h908f2 */
	0x8F3,	/* 32'h908f3 */
	0x8F4,	/* 32'h908f4 */
	0x8F5,	/* 32'h908f5 */
	0x8F6,	/* 32'h908f6 */
	0x8F7,	/* 32'h908f7 */
	0x801,	/* 32'h90801 */
	0x80A	/* 32'h9080a */
};

/* Group 11: base 0x20000 - 39 registers */
static const uint16_t ddrphy_offsets_group_11[] = {
	0x2,	/* 32'h20002 */
	0x0,	/* 32'h20000 */
	0x7,	/* 32'h20007 */
	0x13,	/* 32'h20013 */
	0x4,	/* 32'h20004 */
	0x3,	/* 32'h20003 */
	0x1,	/* 32'h20001 */
	0x9,	/* 32'h20009 */
	0x8,	/* 32'h20008 */
	0x14,	/* 32'h20014 */
	0x331,	/* 32'h20331 */
	0x12,	/* 32'h20012 */
	0x17,	/* 32'h20017 */
	0xA,	/* 32'h2000a */
	0x186,	/* 32'h20186 */
	0x187,	/* 32'h20187 */
	0x10,	/* 32'h20010 */
	0x11,	/* 32'h20011 */
	0x2C,	/* 32'h2002c */
	0x2D,	/* 32'h2002d */
	0x30,	/* 32'h20030 */
	0x2E,	/* 32'h2002e */
	0x2F,	/* 32'h2002f */
	0x4E,	/* 32'h2004e */
	0x4D,	/* 32'h2004d */
	0x35,	/* 32'h20035 */
	0x36,	/* 32'h20036 */
	0x37,	/* 32'h20037 */
	0x38,	/* 32'h20038 */
	0x39,	/* 32'h20039 */
	0x3A,	/* 32'h2003a */
	0x3B,	/* 32'h2003b */
	0x3C,	/* 32'h2003c */
	0x3D,	/* 32'h2003d */
	0x3E,	/* 32'h2003e */
	0x3F,	/* 32'h2003f */
	0x40,	/* 32'h20040 */
	0xC,	/* 32'h2000c */
	0x330	/* 32'h20330 */
};

/* Group 12: base 0x30000 - 23 registers */
static const uint16_t ddrphy_offsets_group_12[] = {
	0x38,	/* 32'h30038 */
	0x39,	/* 32'h30039 */
	0x3A,	/* 32'h3003a */
	0x50,	/* 32'h30050 */
	0x51,	/* 32'h30051 */
	0x52,	/* 32'h30052 */
	0x53,	/* 32'h30053 */
	0x30,	/* 32'h30030 */
	0x31,	/* 32'h30031 */
	0x35,	/* 32'h30035 */
	0x3C,	/* 32'h3003c */
	0x40,	/* 32'h30040 */
	0x41,	/* 32'h30041 */
	0x42,	/* 32'h30042 */
	0x43,	/* 32'h30043 */
	0x330,	/* 32'h30330 */
	0x48,	/* 32'h30048 */
	0x49,	/* 32'h30049 */
	0x4A,	/* 32'h3004a */
	0x4B,	/* 32'h3004b */
	0x33,	/* 32'h30033 */
	0x34,	/* 32'h30034 */
	0x2E	/* 32'h3002e */
};

/* Group 13: base 0x31000 - 23 registers (reuses ddrphy_offsets_group_12) */

/* Group 14: base 0x10000 - 67 registers */
static const uint16_t ddrphy_offsets_group_14[] = {
	0xD0,	/* 32'h100d0 */
	0xD1,	/* 32'h100d1 */
	0xD2,	/* 32'h100d2 */
	0xD3,	/* 32'h100d3 */
	0xD4,	/* 32'h100d4 */
	0xAF,	/* 32'h100af */
	0xAD,	/* 32'h100ad */
	0x0,	/* 32'h10000 */
	0x38,	/* 32'h10038 */
	0x3A,	/* 32'h1003a */
	0x3B,	/* 32'h1003b */
	0x4,	/* 32'h10004 */
	0x3,	/* 32'h10003 */
	0x30,	/* 32'h10030 */
	0x35,	/* 32'h10035 */
	0x3C,	/* 32'h1003c */
	0x3E,	/* 32'h1003e */
	0x6,	/* 32'h10006 */
	0x40,	/* 32'h10040 */
	0x42,	/* 32'h10042 */
	0x43,	/* 32'h10043 */
	0x48,	/* 32'h10048 */
	0x4A,	/* 32'h1004a */
	0x4B,	/* 32'h1004b */
	0x33,	/* 32'h10033 */
	0x2E,	/* 32'h1002e */
	0xA5,	/* 32'h100a5 */
	0x14,	/* 32'h10014 */
	0x57,	/* 32'h10057 */
	0x4E,	/* 32'h1004e */
	0x4F,	/* 32'h1004f */
	0x50,	/* 32'h10050 */
	0x51,	/* 32'h10051 */
	0x14E,	/* 32'h1014e */
	0x14F,	/* 32'h1014f */
	0x150,	/* 32'h10150 */
	0x151,	/* 32'h10151 */
	0x24E,	/* 32'h1024e */
	0x24F,	/* 32'h1024f */
	0x250,	/* 32'h10250 */
	0x251,	/* 32'h10251 */
	0x34E,	/* 32'h1034e */
	0x34F,	/* 32'h1034f */
	0x350,	/* 32'h10350 */
	0x351,	/* 32'h10351 */
	0x44E,	/* 32'h1044e */
	0x44F,	/* 32'h1044f */
	0x450,	/* 32'h10450 */
	0x451,	/* 32'h10451 */
	0x54E,	/* 32'h1054e */
	0x54F,	/* 32'h1054f */
	0x550,	/* 32'h10550 */
	0x551,	/* 32'h10551 */
	0x64E,	/* 32'h1064e */
	0x64F,	/* 32'h1064f */
	0x650,	/* 32'h10650 */
	0x651,	/* 32'h10651 */
	0x74E,	/* 32'h1074e */
	0x74F,	/* 32'h1074f */
	0x750,	/* 32'h10750 */
	0x751,	/* 32'h10751 */
	0x84E,	/* 32'h1084e */
	0x84F,	/* 32'h1084f */
	0x850,	/* 32'h10850 */
	0x851,	/* 32'h10851 */
	0xC,	/* 32'h1000c */
	0xD	/* 32'h1000d */
};

/* Group 15: base 0x11000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 16: base 0x12000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 17: base 0x13000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 18: base 0x190000 - 11 registers (reuses ddrphy_offsets_group_10) */

/* Group 19: base 0x120000 - 39 registers (reuses ddrphy_offsets_group_11) */

/* Group 20: base 0x130000 - 23 registers (reuses ddrphy_offsets_group_12) */

/* Group 21: base 0x131000 - 23 registers (reuses ddrphy_offsets_group_12) */

/* Group 22: base 0x110000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 23: base 0x111000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 24: base 0x112000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 25: base 0x113000 - 67 registers (reuses ddrphy_offsets_group_14) */

/* Group 26: base 0x30000 - 14 registers */
static const uint16_t ddrphy_offsets_group_26[] = {
	0x1,	/* 32'h30001 */
	0x101,	/* 32'h30101 */
	0x201,	/* 32'h30201 */
	0x301,	/* 32'h30301 */
	0x401,	/* 32'h30401 */
	0x501,	/* 32'h30501 */
	0x601,	/* 32'h30601 */
	0x701,	/* 32'h30701 */
	0x801,	/* 32'h30801 */
	0x901,	/* 32'h30901 */
	0x6,	/* 32'h30006 */
	0x8,	/* 32'h30008 */
	0x2,	/* 32'h30002 */
	0x5	/* 32'h30005 */
};

/* Group 27: base 0x130000 - 14 registers (reuses ddrphy_offsets_group_26) */

/* Group 28: base 0x31000 - 14 registers (reuses ddrphy_offsets_group_26) */

/* Group 29: base 0x131000 - 14 registers (reuses ddrphy_offsets_group_26) */

/* Group 30: base 0x10000 - 94 registers */
static const uint16_t ddrphy_offsets_group_30[] = {
	0x24,	/* 32'h10024 */
	0x10,	/* 32'h10010 */
	0x12,	/* 32'h10012 */
	0x26,	/* 32'h10026 */
	0x124,	/* 32'h10124 */
	0x110,	/* 32'h10110 */
	0x112,	/* 32'h10112 */
	0x126,	/* 32'h10126 */
	0x224,	/* 32'h10224 */
	0x210,	/* 32'h10210 */
	0x212,	/* 32'h10212 */
	0x226,	/* 32'h10226 */
	0x324,	/* 32'h10324 */
	0x310,	/* 32'h10310 */
	0x312,	/* 32'h10312 */
	0x326,	/* 32'h10326 */
	0x424,	/* 32'h10424 */
	0x410,	/* 32'h10410 */
	0x412,	/* 32'h10412 */
	0x426,	/* 32'h10426 */
	0x524,	/* 32'h10524 */
	0x510,	/* 32'h10510 */
	0x512,	/* 32'h10512 */
	0x526,	/* 32'h10526 */
	0x624,	/* 32'h10624 */
	0x610,	/* 32'h10610 */
	0x612,	/* 32'h10612 */
	0x626,	/* 32'h10626 */
	0x724,	/* 32'h10724 */
	0x710,	/* 32'h10710 */
	0x712,	/* 32'h10712 */
	0x726,	/* 32'h10726 */
	0x824,	/* 32'h10824 */
	0x810,	/* 32'h10810 */
	0x812,	/* 32'h10812 */
	0x826,	/* 32'h10826 */
	0x5A,	/* 32'h1005a */
	0x5C,	/* 32'h1005c */
	0x5E,	/* 32'h1005e */
	0x60,	/* 32'h10060 */
	0x62,	/* 32'h10062 */
	0x64,	/* 32'h10064 */
	0x66,	/* 32'h10066 */
	0x2A,	/* 32'h1002a */
	0x28,	/* 32'h10028 */
	0x15,	/* 32'h10015 */
	0x20,	/* 32'h10020 */
	0x25,	/* 32'h10025 */
	0x11,	/* 32'h10011 */
	0x13,	/* 32'h10013 */
	0x27,	/* 32'h10027 */
	0x125,	/* 32'h10125 */
	0x111,	/* 32'h10111 */
	0x113,	/* 32'h10113 */
	0x127,	/* 32'h10127 */
	0x225,	/* 32'h10225 */
	0x211,	/* 32'h10211 */
	0x213,	/* 32'h10213 */
	0x227,	/* 32'h10227 */
	0x325,	/* 32'h10325 */
	0x311,	/* 32'h10311 */
	0x313,	/* 32'h10313 */
	0x327,	/* 32'h10327 */
	0x425,	/* 32'h10425 */
	0x411,	/* 32'h10411 */
	0x413,	/* 32'h10413 */
	0x427,	/* 32'h10427 */
	0x525,	/* 32'h10525 */
	0x511,	/* 32'h10511 */
	0x513,	/* 32'h10513 */
	0x527,	/* 32'h10527 */
	0x625,	/* 32'h10625 */
	0x611,	/* 32'h10611 */
	0x613,	/* 32'h10613 */
	0x627,	/* 32'h10627 */
	0x725,	/* 32'h10725 */
	0x711,	/* 32'h10711 */
	0x713,	/* 32'h10713 */
	0x727,	/* 32'h10727 */
	0x825,	/* 32'h10825 */
	0x811,	/* 32'h10811 */
	0x813,	/* 32'h10813 */
	0x827,	/* 32'h10827 */
	0x5B,	/* 32'h1005b */
	0x5D,	/* 32'h1005d */
	0x5F,	/* 32'h1005f */
	0x61,	/* 32'h10061 */
	0x63,	/* 32'h10063 */
	0x65,	/* 32'h10065 */
	0x67,	/* 32'h10067 */
	0x2B,	/* 32'h1002b */
	0x29,	/* 32'h10029 */
	0x16,	/* 32'h10016 */
	0x21	/* 32'h10021 */
};

/* Group 31: base 0x110000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 32: base 0x11000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 33: base 0x111000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 34: base 0x12000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 35: base 0x112000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 36: base 0x13000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 37: base 0x113000 - 94 registers (reuses ddrphy_offsets_group_30) */

/* Group 38: base 0x20000 - 7 registers */
static const uint16_t ddrphy_offsets_group_38[] = {
	0x88,	/* 32'h20088 */
	0x89,	/* 32'h20089 */
	0x8A,	/* 32'h2008a */
	0x8C,	/* 32'h2008c */
	0x4C,	/* 32'h2004c */
	0x4A,	/* 32'h2004a */
	0x4B	/* 32'h2004b */
};

/* Group 39: base 0x120000 - 3 registers */
static const uint16_t ddrphy_offsets_group_39[] = {
	0x4C,	/* 32'h12004c */
	0x4A,	/* 32'h12004a */
	0x4B	/* 32'h12004b */
};

/* Group 40: base 0x30000 - 7 registers */
static const uint16_t ddrphy_offsets_group_40[] = {
	0x0,	/* 32'h30000 */
	0x3,	/* 32'h30003 */
	0x4,	/* 32'h30004 */
	0x7,	/* 32'h30007 */
	0x37,	/* 32'h30037 */
	0xE8,	/* 32'h300e8 */
	0xE9	/* 32'h300e9 */
};

/* Group 41: base 0x130000 - 7 registers (reuses ddrphy_offsets_group_40) */

/* Group 42: base 0x30000 - 21 registers */
static const uint16_t ddrphy_offsets_group_42[] = {
	0x1A,	/* 32'h3001a */
	0x60,	/* 32'h30060 */
	0x80,	/* 32'h30080 */
	0x81,	/* 32'h30081 */
	0x82,	/* 32'h30082 */
	0x84,	/* 32'h30084 */
	0x85,	/* 32'h30085 */
	0x86,	/* 32'h30086 */
	0x87,	/* 32'h30087 */
	0x88,	/* 32'h30088 */
	0x89,	/* 32'h30089 */
	0xA0,	/* 32'h300a0 */
	0xA1,	/* 32'h300a1 */
	0xA2,	/* 32'h300a2 */
	0xA3,	/* 32'h300a3 */
	0xA6,	/* 32'h300a6 */
	0xA7,	/* 32'h300a7 */
	0xA8,	/* 32'h300a8 */
	0xA9,	/* 32'h300a9 */
	0xAA,	/* 32'h300aa */
	0xAB	/* 32'h300ab */
};

/* Group 43: base 0x31000 - 7 registers (reuses ddrphy_offsets_group_40) */

/* Group 44: base 0x131000 - 7 registers (reuses ddrphy_offsets_group_40) */

/* Group 45: base 0x31000 - 21 registers (reuses ddrphy_offsets_group_42) */

/* Group 46: base 0x90000 - 30 registers */
static const uint16_t ddrphy_offsets_group_46[] = {
	0x803,	/* 32'h90803 */
	0x804,	/* 32'h90804 */
	0x805,	/* 32'h90805 */
	0x806,	/* 32'h90806 */
	0x807,	/* 32'h90807 */
	0x808,	/* 32'h90808 */
	0x809,	/* 32'h90809 */
	0x80B,	/* 32'h9080b */
	0x80C,	/* 32'h9080c */
	0x80D,	/* 32'h9080d */
	0x80E,	/* 32'h9080e */
	0x80F,	/* 32'h9080f */
	0x811,	/* 32'h90811 */
	0x812,	/* 32'h90812 */
	0x813,	/* 32'h90813 */
	0x814,	/* 32'h90814 */
	0x815,	/* 32'h90815 */
	0x816,	/* 32'h90816 */
	0x817,	/* 32'h90817 */
	0x818,	/* 32'h90818 */
	0x819,	/* 32'h90819 */
	0x81A,	/* 32'h9081a */
	0x81B,	/* 32'h9081b */
	0x81C,	/* 32'h9081c */
	0x81D,	/* 32'h9081d */
	0x81E,	/* 32'h9081e */
	0x81F,	/* 32'h9081f */
	0x903,	/* 32'h90903 */
	0x938,	/* 32'h90938 */
	0x90B	/* 32'h9090b */
};

/* Group 47: base 0x190000 - 30 registers (reuses ddrphy_offsets_group_46) */

/* Group 48: base 0x90000 - 1651 registers */
/* Group 48 seg 0: sequential 1536 offsets from 0x2E (step=1) - encoded as start+count */
static const uint16_t ddrphy_offsets_group_48[] = {
	0x1C,	/* 32'h9001c */
	0x1D,	/* 32'h9001d */
	0x1E,	/* 32'h9001e */
	0x1F,	/* 32'h9001f */
	0x20,	/* 32'h90020 */
	0x21,	/* 32'h90021 */
	0x22,	/* 32'h90022 */
	0x23,	/* 32'h90023 */
	0x24,	/* 32'h90024 */
	0x25,	/* 32'h90025 */
	0x26,	/* 32'h90026 */
	0x27,	/* 32'h90027 */
	0x28,	/* 32'h90028 */
	0x29,	/* 32'h90029 */
	0x2A,	/* 32'h9002a */
	0x2B,	/* 32'h9002b */
	0x2C,	/* 32'h9002c */
	0x2D,	/* 32'h9002d */
	0x700,	/* 32'h90700 */
	0x701,	/* 32'h90701 */
	0x702,	/* 32'h90702 */
	0x703,	/* 32'h90703 */
	0x704,	/* 32'h90704 */
	0x705,	/* 32'h90705 */
	0x706,	/* 32'h90706 */
	0x707,	/* 32'h90707 */
	0x708,	/* 32'h90708 */
	0x70C,	/* 32'h9070c */
	0x70D,	/* 32'h9070d */
	0x70E,	/* 32'h9070e */
	0x70F,	/* 32'h9070f */
	0x710,	/* 32'h90710 */
	0x711,	/* 32'h90711 */
	0x712,	/* 32'h90712 */
	0x713,	/* 32'h90713 */
	0x714,	/* 32'h90714 */
	0x715,	/* 32'h90715 */
	0x716,	/* 32'h90716 */
	0x717,	/* 32'h90717 */
	0x718,	/* 32'h90718 */
	0x719,	/* 32'h90719 */
	0x71A,	/* 32'h9071a */
	0x71B,	/* 32'h9071b */
	0x820,	/* 32'h90820 */
	0x821,	/* 32'h90821 */
	0x822,	/* 32'h90822 */
	0x823,	/* 32'h90823 */
	0x824,	/* 32'h90824 */
	0x825,	/* 32'h90825 */
	0x826,	/* 32'h90826 */
	0x827,	/* 32'h90827 */
	0x900,	/* 32'h90900 */
	0x901,	/* 32'h90901 */
	0x902,	/* 32'h90902 */
	0x904,	/* 32'h90904 */
	0x905,	/* 32'h90905 */
	0x908,	/* 32'h90908 */
	0x909,	/* 32'h90909 */
	0x90A,	/* 32'h9090a */
	0x90C,	/* 32'h9090c */
	0x90D,	/* 32'h9090d */
	0x90E,	/* 32'h9090e */
	0x910,	/* 32'h90910 */
	0x911,	/* 32'h90911 */
	0x912,	/* 32'h90912 */
	0x920,	/* 32'h90920 */
	0x921,	/* 32'h90921 */
	0x922,	/* 32'h90922 */
	0x923,	/* 32'h90923 */
	0x924,	/* 32'h90924 */
	0x925,	/* 32'h90925 */
	0x926,	/* 32'h90926 */
	0x927,	/* 32'h90927 */
	0x928,	/* 32'h90928 */
	0x929,	/* 32'h90929 */
	0x92A,	/* 32'h9092a */
	0x92B,	/* 32'h9092b */
	0x92C,	/* 32'h9092c */
	0x92D,	/* 32'h9092d */
	0x92E,	/* 32'h9092e */
	0x92F,	/* 32'h9092f */
	0x930,	/* 32'h90930 */
	0x931,	/* 32'h90931 */
	0x932,	/* 32'h90932 */
	0x933,	/* 32'h90933 */
	0x934,	/* 32'h90934 */
	0x935,	/* 32'h90935 */
	0x936,	/* 32'h90936 */
	0x937,	/* 32'h90937 */
	0x940,	/* 32'h90940 */
	0x941,	/* 32'h90941 */
	0x942,	/* 32'h90942 */
	0x943,	/* 32'h90943 */
	0x944,	/* 32'h90944 */
	0x945,	/* 32'h90945 */
	0x946,	/* 32'h90946 */
	0x947,	/* 32'h90947 */
	0x986,	/* 32'h90986 */
	0x987,	/* 32'h90987 */
	0xC10,	/* 32'h90c10 */
	0xC11,	/* 32'h90c11 */
	0xC12,	/* 32'h90c12 */
	0xC13,	/* 32'h90c13 */
	0xC14,	/* 32'h90c14 */
	0xC15,	/* 32'h90c15 */
	0xC16,	/* 32'h90c16 */
	0xC17,	/* 32'h90c17 */
	0xC18,	/* 32'h90c18 */
	0xC19,	/* 32'h90c19 */
	0xC1A,	/* 32'h90c1a */
	0xC1B,	/* 32'h90c1b */
	0xC1C,	/* 32'h90c1c */
	0xC1D,	/* 32'h90c1d */
	0xC1E,	/* 32'h90c1e */
	0xC1F	/* 32'h90c1f */
};

/* Group 49: base 0xC0000 - 40 registers */
static const uint16_t ddrphy_offsets_group_49[] = {
	0x0,	/* 32'hc0000 */
	0x6,	/* 32'hc0006 */
	0x7,	/* 32'hc0007 */
	0x2,	/* 32'hc0002 */
	0x3,	/* 32'hc0003 */
	0x1,	/* 32'hc0001 */
	0x36,	/* 32'hc0036 */
	0x85,	/* 32'hc0085 */
	0x88,	/* 32'hc0088 */
	0x100,	/* 32'hc0100 */
	0x101,	/* 32'hc0101 */
	0x102,	/* 32'hc0102 */
	0x104,	/* 32'hc0104 */
	0x105,	/* 32'hc0105 */
	0x108,	/* 32'hc0108 */
	0x109,	/* 32'hc0109 */
	0x10A,	/* 32'hc010a */
	0x10B,	/* 32'hc010b */
	0x10C,	/* 32'hc010c */
	0x10D,	/* 32'hc010d */
	0x10E,	/* 32'hc010e */
	0x10F,	/* 32'hc010f */
	0x110,	/* 32'hc0110 */
	0x111,	/* 32'hc0111 */
	0x112,	/* 32'hc0112 */
	0x113,	/* 32'hc0113 */
	0x114,	/* 32'hc0114 */
	0x115,	/* 32'hc0115 */
	0x116,	/* 32'hc0116 */
	0x117,	/* 32'hc0117 */
	0x118,	/* 32'hc0118 */
	0x119,	/* 32'hc0119 */
	0x11A,	/* 32'hc011a */
	0x11B,	/* 32'hc011b */
	0x11C,	/* 32'hc011c */
	0x11D,	/* 32'hc011d */
	0x11E,	/* 32'hc011e */
	0x11F,	/* 32'hc011f */
	0x120,	/* 32'hc0120 */
	0x121	/* 32'hc0121 */
};

/* Group 50: base 0x70000 - 101 registers */
static const uint16_t ddrphy_offsets_group_50[] = {
	0x24,	/* 32'h70024 */
	0x25,	/* 32'h70025 */
	0x26,	/* 32'h70026 */
	0x27,	/* 32'h70027 */
	0x28,	/* 32'h70028 */
	0x29,	/* 32'h70029 */
	0x2A,	/* 32'h7002a */
	0x2B,	/* 32'h7002b */
	0x124,	/* 32'h70124 */
	0x125,	/* 32'h70125 */
	0x126,	/* 32'h70126 */
	0x127,	/* 32'h70127 */
	0x128,	/* 32'h70128 */
	0x129,	/* 32'h70129 */
	0x12A,	/* 32'h7012a */
	0x12B,	/* 32'h7012b */
	0x224,	/* 32'h70224 */
	0x225,	/* 32'h70225 */
	0x226,	/* 32'h70226 */
	0x227,	/* 32'h70227 */
	0x228,	/* 32'h70228 */
	0x229,	/* 32'h70229 */
	0x22A,	/* 32'h7022a */
	0x22B,	/* 32'h7022b */
	0x324,	/* 32'h70324 */
	0x325,	/* 32'h70325 */
	0x326,	/* 32'h70326 */
	0x327,	/* 32'h70327 */
	0x328,	/* 32'h70328 */
	0x329,	/* 32'h70329 */
	0x32A,	/* 32'h7032a */
	0x32B,	/* 32'h7032b */
	0x424,	/* 32'h70424 */
	0x425,	/* 32'h70425 */
	0x426,	/* 32'h70426 */
	0x427,	/* 32'h70427 */
	0x428,	/* 32'h70428 */
	0x429,	/* 32'h70429 */
	0x42A,	/* 32'h7042a */
	0x42B,	/* 32'h7042b */
	0x524,	/* 32'h70524 */
	0x525,	/* 32'h70525 */
	0x526,	/* 32'h70526 */
	0x527,	/* 32'h70527 */
	0x528,	/* 32'h70528 */
	0x529,	/* 32'h70529 */
	0x52A,	/* 32'h7052a */
	0x52B,	/* 32'h7052b */
	0x624,	/* 32'h70624 */
	0x625,	/* 32'h70625 */
	0x626,	/* 32'h70626 */
	0x627,	/* 32'h70627 */
	0x628,	/* 32'h70628 */
	0x629,	/* 32'h70629 */
	0x62A,	/* 32'h7062a */
	0x62B,	/* 32'h7062b */
	0x724,	/* 32'h70724 */
	0x725,	/* 32'h70725 */
	0x726,	/* 32'h70726 */
	0x727,	/* 32'h70727 */
	0x728,	/* 32'h70728 */
	0x729,	/* 32'h70729 */
	0x72A,	/* 32'h7072a */
	0x72B,	/* 32'h7072b */
	0x824,	/* 32'h70824 */
	0x825,	/* 32'h70825 */
	0x826,	/* 32'h70826 */
	0x827,	/* 32'h70827 */
	0x828,	/* 32'h70828 */
	0x829,	/* 32'h70829 */
	0x82A,	/* 32'h7082a */
	0x82B,	/* 32'h7082b */
	0x11,	/* 32'h70011 */
	0x30,	/* 32'h70030 */
	0x31,	/* 32'h70031 */
	0x32,	/* 32'h70032 */
	0x33,	/* 32'h70033 */
	0x34,	/* 32'h70034 */
	0x35,	/* 32'h70035 */
	0x36,	/* 32'h70036 */
	0x37,	/* 32'h70037 */
	0x38,	/* 32'h70038 */
	0x39,	/* 32'h70039 */
	0x3A,	/* 32'h7003a */
	0x3B,	/* 32'h7003b */
	0x3C,	/* 32'h7003c */
	0x3D,	/* 32'h7003d */
	0x3E,	/* 32'h7003e */
	0x3F,	/* 32'h7003f */
	0x60,	/* 32'h70060 */
	0x61,	/* 32'h70061 */
	0x62,	/* 32'h70062 */
	0x63,	/* 32'h70063 */
	0x64,	/* 32'h70064 */
	0x65,	/* 32'h70065 */
	0xD2,	/* 32'h700d2 */
	0xD3,	/* 32'h700d3 */
	0xD4,	/* 32'h700d4 */
	0xD5,	/* 32'h700d5 */
	0xD6,	/* 32'h700d6 */
	0xD7	/* 32'h700d7 */
};

/* Group 51: base 0x20000 - 10 registers */
static const uint16_t ddrphy_offsets_group_51[] = {
	0xD,	/* 32'h2000d */
	0x16,	/* 32'h20016 */
	0x22,	/* 32'h20022 */
	0x23,	/* 32'h20023 */
	0x43,	/* 32'h20043 */
	0x45,	/* 32'h20045 */
	0x46,	/* 32'h20046 */
	0x15,	/* 32'h20015 */
	0x41,	/* 32'h20041 */
	0x1B	/* 32'h2001b */
};

/* Group 52: base 0x120000 - 10 registers (reuses ddrphy_offsets_group_51) */

/* Group 53: base 0x20000 - 234 registers */
/* Group 53 seg 0: sequential 96 offsets from 0x124 (step=1) - encoded as start+count */
/* Group 53 seg 1: sequential 96 offsets from 0x18B (step=1) - encoded as start+count */
static const uint16_t ddrphy_offsets_group_53[] = {
	0x18,	/* 32'h20018 */
	0x1C,	/* 32'h2001c */
	0x20,	/* 32'h20020 */
	0x25,	/* 32'h20025 */
	0x26,	/* 32'h20026 */
	0x27,	/* 32'h20027 */
	0x28,	/* 32'h20028 */
	0x2A,	/* 32'h2002a */
	0x31,	/* 32'h20031 */
	0x32,	/* 32'h20032 */
	0x33,	/* 32'h20033 */
	0x34,	/* 32'h20034 */
	0x42,	/* 32'h20042 */
	0x44,	/* 32'h20044 */
	0x57,	/* 32'h20057 */
	0x59,	/* 32'h20059 */
	0x60,	/* 32'h20060 */
	0x72,	/* 32'h20072 */
	0x73,	/* 32'h20073 */
	0x74,	/* 32'h20074 */
	0x78,	/* 32'h20078 */
	0x79,	/* 32'h20079 */
	0x7A,	/* 32'h2007a */
	0x7E,	/* 32'h2007e */
	0xA0,	/* 32'h200a0 */
	0xA6,	/* 32'h200a6 */
	0xA7,	/* 32'h200a7 */
	0xA8,	/* 32'h200a8 */
	0xBD,	/* 32'h200bd */
	0xC0,	/* 32'h200c0 */
	0xC1,	/* 32'h200c1 */
	0xC2,	/* 32'h200c2 */
	0xEF,	/* 32'h200ef */
	0xF0,	/* 32'h200f0 */
	0xF1,	/* 32'h200f1 */
	0xF2,	/* 32'h200f2 */
	0x11B,	/* 32'h2011b */
	0x11C,	/* 32'h2011c */
	0x11D,	/* 32'h2011d */
	0x11E,	/* 32'h2011e */
	0x122,	/* 32'h20122 */
	0x311	/* 32'h20311 */
};

/* Group 54: base 0x10000 - 134 registers */
static const uint16_t ddrphy_offsets_group_54[] = {
	0x6C,	/* 32'h1006c */
	0x6D,	/* 32'h1006d */
	0x6A,	/* 32'h1006a */
	0x6B,	/* 32'h1006b */
	0x68,	/* 32'h10068 */
	0x69,	/* 32'h10069 */
	0x6E,	/* 32'h1006e */
	0x6F,	/* 32'h1006f */
	0x1E,	/* 32'h1001e */
	0x1F,	/* 32'h1001f */
	0x1C,	/* 32'h1001c */
	0x1D,	/* 32'h1001d */
	0x16C,	/* 32'h1016c */
	0x16D,	/* 32'h1016d */
	0x16A,	/* 32'h1016a */
	0x16B,	/* 32'h1016b */
	0x168,	/* 32'h10168 */
	0x169,	/* 32'h10169 */
	0x16E,	/* 32'h1016e */
	0x16F,	/* 32'h1016f */
	0x11E,	/* 32'h1011e */
	0x11F,	/* 32'h1011f */
	0x11C,	/* 32'h1011c */
	0x11D,	/* 32'h1011d */
	0x26C,	/* 32'h1026c */
	0x26D,	/* 32'h1026d */
	0x26A,	/* 32'h1026a */
	0x26B,	/* 32'h1026b */
	0x268,	/* 32'h10268 */
	0x269,	/* 32'h10269 */
	0x26E,	/* 32'h1026e */
	0x26F,	/* 32'h1026f */
	0x21E,	/* 32'h1021e */
	0x21F,	/* 32'h1021f */
	0x21C,	/* 32'h1021c */
	0x21D,	/* 32'h1021d */
	0x36C,	/* 32'h1036c */
	0x36D,	/* 32'h1036d */
	0x36A,	/* 32'h1036a */
	0x36B,	/* 32'h1036b */
	0x368,	/* 32'h10368 */
	0x369,	/* 32'h10369 */
	0x36E,	/* 32'h1036e */
	0x36F,	/* 32'h1036f */
	0x31E,	/* 32'h1031e */
	0x31F,	/* 32'h1031f */
	0x31C,	/* 32'h1031c */
	0x31D,	/* 32'h1031d */
	0x46C,	/* 32'h1046c */
	0x46D,	/* 32'h1046d */
	0x46A,	/* 32'h1046a */
	0x46B,	/* 32'h1046b */
	0x468,	/* 32'h10468 */
	0x469,	/* 32'h10469 */
	0x46E,	/* 32'h1046e */
	0x46F,	/* 32'h1046f */
	0x41E,	/* 32'h1041e */
	0x41F,	/* 32'h1041f */
	0x41C,	/* 32'h1041c */
	0x41D,	/* 32'h1041d */
	0x56C,	/* 32'h1056c */
	0x56D,	/* 32'h1056d */
	0x56A,	/* 32'h1056a */
	0x56B,	/* 32'h1056b */
	0x568,	/* 32'h10568 */
	0x569,	/* 32'h10569 */
	0x56E,	/* 32'h1056e */
	0x56F,	/* 32'h1056f */
	0x51E,	/* 32'h1051e */
	0x51F,	/* 32'h1051f */
	0x51C,	/* 32'h1051c */
	0x51D,	/* 32'h1051d */
	0x66C,	/* 32'h1066c */
	0x66D,	/* 32'h1066d */
	0x66A,	/* 32'h1066a */
	0x66B,	/* 32'h1066b */
	0x668,	/* 32'h10668 */
	0x669,	/* 32'h10669 */
	0x66E,	/* 32'h1066e */
	0x66F,	/* 32'h1066f */
	0x61E,	/* 32'h1061e */
	0x61F,	/* 32'h1061f */
	0x61C,	/* 32'h1061c */
	0x61D,	/* 32'h1061d */
	0x76C,	/* 32'h1076c */
	0x76D,	/* 32'h1076d */
	0x76A,	/* 32'h1076a */
	0x76B,	/* 32'h1076b */
	0x768,	/* 32'h10768 */
	0x769,	/* 32'h10769 */
	0x76E,	/* 32'h1076e */
	0x76F,	/* 32'h1076f */
	0x71E,	/* 32'h1071e */
	0x71F,	/* 32'h1071f */
	0x71C,	/* 32'h1071c */
	0x71D,	/* 32'h1071d */
	0x86C,	/* 32'h1086c */
	0x86D,	/* 32'h1086d */
	0x86A,	/* 32'h1086a */
	0x86B,	/* 32'h1086b */
	0x868,	/* 32'h10868 */
	0x869,	/* 32'h10869 */
	0x86E,	/* 32'h1086e */
	0x86F,	/* 32'h1086f */
	0x81E,	/* 32'h1081e */
	0x81F,	/* 32'h1081f */
	0x81C,	/* 32'h1081c */
	0x81D,	/* 32'h1081d */
	0xF,	/* 32'h1000f */
	0x19,	/* 32'h10019 */
	0x1B,	/* 32'h1001b */
	0x22,	/* 32'h10022 */
	0x23,	/* 32'h10023 */
	0x1,	/* 32'h10001 */
	0x5,	/* 32'h10005 */
	0x7,	/* 32'h10007 */
	0x9,	/* 32'h10009 */
	0xB,	/* 32'h1000b */
	0x17,	/* 32'h10017 */
	0x18,	/* 32'h10018 */
	0x2F,	/* 32'h1002f */
	0x36,	/* 32'h10036 */
	0x37,	/* 32'h10037 */
	0x39,	/* 32'h10039 */
	0x3D,	/* 32'h1003d */
	0x44,	/* 32'h10044 */
	0x45,	/* 32'h10045 */
	0x4C,	/* 32'h1004c */
	0x4D,	/* 32'h1004d */
	0xAE,	/* 32'h100ae */
	0xE8,	/* 32'h100e8 */
	0xE9,	/* 32'h100e9 */
	0xEA,	/* 32'h100ea */
	0xEB	/* 32'h100eb */
};

/* Group 55: base 0x110000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 56: base 0x10000 - 61 registers */
static const uint16_t ddrphy_offsets_group_56[] = {
	0x2,	/* 32'h10002 */
	0xA,	/* 32'h1000a */
	0x1A,	/* 32'h1001a */
	0x72,	/* 32'h10072 */
	0x75,	/* 32'h10075 */
	0x89,	/* 32'h10089 */
	0x8A,	/* 32'h1008a */
	0x8B,	/* 32'h1008b */
	0x8C,	/* 32'h1008c */
	0x8E,	/* 32'h1008e */
	0x91,	/* 32'h10091 */
	0x92,	/* 32'h10092 */
	0x93,	/* 32'h10093 */
	0x94,	/* 32'h10094 */
	0x95,	/* 32'h10095 */
	0x96,	/* 32'h10096 */
	0x97,	/* 32'h10097 */
	0x9E,	/* 32'h1009e */
	0xA2,	/* 32'h100a2 */
	0xA4,	/* 32'h100a4 */
	0xA6,	/* 32'h100a6 */
	0xA7,	/* 32'h100a7 */
	0xA8,	/* 32'h100a8 */
	0xAC,	/* 32'h100ac */
	0xB0,	/* 32'h100b0 */
	0xB1,	/* 32'h100b1 */
	0xB2,	/* 32'h100b2 */
	0xB3,	/* 32'h100b3 */
	0xB4,	/* 32'h100b4 */
	0xB9,	/* 32'h100b9 */
	0xBA,	/* 32'h100ba */
	0xBB,	/* 32'h100bb */
	0xDA,	/* 32'h100da */
	0xDE,	/* 32'h100de */
	0xBE,	/* 32'h100be */
	0xB5,	/* 32'h100b5 */
	0xAA,	/* 32'h100aa */
	0x1BE,	/* 32'h101be */
	0x1B5,	/* 32'h101b5 */
	0x1AA,	/* 32'h101aa */
	0x2BE,	/* 32'h102be */
	0x2B5,	/* 32'h102b5 */
	0x2AA,	/* 32'h102aa */
	0x3BE,	/* 32'h103be */
	0x3B5,	/* 32'h103b5 */
	0x3AA,	/* 32'h103aa */
	0x4BE,	/* 32'h104be */
	0x4B5,	/* 32'h104b5 */
	0x4AA,	/* 32'h104aa */
	0x5BE,	/* 32'h105be */
	0x5B5,	/* 32'h105b5 */
	0x5AA,	/* 32'h105aa */
	0x6BE,	/* 32'h106be */
	0x6B5,	/* 32'h106b5 */
	0x6AA,	/* 32'h106aa */
	0x7BE,	/* 32'h107be */
	0x7B5,	/* 32'h107b5 */
	0x7AA,	/* 32'h107aa */
	0x8BE,	/* 32'h108be */
	0x8B5,	/* 32'h108b5 */
	0x8AA	/* 32'h108aa */
};

/* Group 57: base 0x11000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 58: base 0x111000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 59: base 0x11000 - 61 registers (reuses ddrphy_offsets_group_56) */

/* Group 60: base 0x12000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 61: base 0x112000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 62: base 0x12000 - 61 registers (reuses ddrphy_offsets_group_56) */

/* Group 63: base 0x13000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 64: base 0x113000 - 134 registers (reuses ddrphy_offsets_group_54) */

/* Group 65: base 0x13000 - 61 registers (reuses ddrphy_offsets_group_56) */

/*
 * Flat register group array - one entry per segment, in source order.
 * Dispatch on .type at runtime: SCATTERED uses .offsets,
 *                               SEQUENTIAL uses .start_offset.
 */
static const ddrphy_reg_group_t ddrphy_reg_groups[] = {
	/* Group 0 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x10000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_0,
	},
	/* Group 1 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x11000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_0,
	},
	/* Group 2 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x12000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_0,
	},
	/* Group 3 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x13000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_0,
	},
	/* Group 4 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x30000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_4,
	},
	/* Group 5 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x31000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_4,
	},
	/* Group 6 seg 0: scattered 17 offsets */
	{
		.base_addr = 0xC0000U,
		.count     = 17U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_6,
	},
	/* Group 7 seg 0: scattered 10 offsets */
	{
		.base_addr = 0x20000U,
		.count     = 10U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_7,
	},
	/* Group 8 seg 0: scattered 1 offsets */
	{
		.base_addr = 0x90000U,
		.count     = 1U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_8,
	},
	/* Group 9 seg 0: scattered 38 offsets */
	{
		.base_addr = 0x20000U,
		.count     = 38U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_9,
	},
	/* Group 10 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x90000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_10,
	},
	/* Group 11 seg 0: scattered 39 offsets */
	{
		.base_addr = 0x20000U,
		.count     = 39U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_11,
	},
	/* Group 12 seg 0: scattered 23 offsets */
	{
		.base_addr = 0x30000U,
		.count     = 23U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_12,
	},
	/* Group 13 seg 0: scattered 23 offsets */
	{
		.base_addr = 0x31000U,
		.count     = 23U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_12,
	},
	/* Group 14 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x10000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 15 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x11000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 16 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x12000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 17 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x13000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 18 seg 0: scattered 11 offsets */
	{
		.base_addr = 0x190000U,
		.count     = 11U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_10,
	},
	/* Group 19 seg 0: scattered 39 offsets */
	{
		.base_addr = 0x120000U,
		.count     = 39U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_11,
	},
	/* Group 20 seg 0: scattered 23 offsets */
	{
		.base_addr = 0x130000U,
		.count     = 23U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_12,
	},
	/* Group 21 seg 0: scattered 23 offsets */
	{
		.base_addr = 0x131000U,
		.count     = 23U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_12,
	},
	/* Group 22 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x110000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 23 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x111000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 24 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x112000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 25 seg 0: scattered 67 offsets */
	{
		.base_addr = 0x113000U,
		.count     = 67U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_14,
	},
	/* Group 26 seg 0: scattered 14 offsets */
	{
		.base_addr = 0x30000U,
		.count     = 14U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_26,
	},
	/* Group 27 seg 0: scattered 14 offsets */
	{
		.base_addr = 0x130000U,
		.count     = 14U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_26,
	},
	/* Group 28 seg 0: scattered 14 offsets */
	{
		.base_addr = 0x31000U,
		.count     = 14U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_26,
	},
	/* Group 29 seg 0: scattered 14 offsets */
	{
		.base_addr = 0x131000U,
		.count     = 14U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_26,
	},
	/* Group 30 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x10000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 31 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x110000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 32 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x11000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 33 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x111000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 34 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x12000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 35 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x112000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 36 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x13000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 37 seg 0: scattered 94 offsets */
	{
		.base_addr = 0x113000U,
		.count     = 94U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_30,
	},
	/* Group 38 seg 0: scattered 7 offsets */
	{
		.base_addr = 0x20000U,
		.count     = 7U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_38,
	},
	/* Group 39 seg 0: scattered 3 offsets */
	{
		.base_addr = 0x120000U,
		.count     = 3U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_39,
	},
	/* Group 40 seg 0: scattered 7 offsets */
	{
		.base_addr = 0x30000U,
		.count     = 7U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_40,
	},
	/* Group 41 seg 0: scattered 7 offsets */
	{
		.base_addr = 0x130000U,
		.count     = 7U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_40,
	},
	/* Group 42 seg 0: scattered 21 offsets */
	{
		.base_addr = 0x30000U,
		.count     = 21U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_42,
	},
	/* Group 43 seg 0: scattered 7 offsets */
	{
		.base_addr = 0x31000U,
		.count     = 7U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_40,
	},
	/* Group 44 seg 0: scattered 7 offsets */
	{
		.base_addr = 0x131000U,
		.count     = 7U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_40,
	},
	/* Group 45 seg 0: scattered 21 offsets */
	{
		.base_addr = 0x31000U,
		.count     = 21U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_42,
	},
	/* Group 46 seg 0: scattered 30 offsets */
	{
		.base_addr = 0x90000U,
		.count     = 30U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_46,
	},
	/* Group 47 seg 0: scattered 30 offsets */
	{
		.base_addr = 0x190000U,
		.count     = 30U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_46,
	},
	/* Group 48 seg 0: sequential 1536 offsets from 0x2E */
	{
		.base_addr    = 0x90000U,
		.count        = 1536U,
		.type         = DDRPHY_GROUP_SEQUENTIAL,
		.start_offset = 0x2EU,
	},
	/* Group 48 seg 1: scattered 115 offsets */
	{
		.base_addr = 0x90000U,
		.count     = 115U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_48,
	},
	/* Group 49 seg 0: scattered 40 offsets */
	{
		.base_addr = 0xC0000U,
		.count     = 40U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_49,
	},
	/* Group 50 seg 0: scattered 101 offsets */
	{
		.base_addr = 0x70000U,
		.count     = 101U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_50,
	},
	/* Group 51 seg 0: scattered 10 offsets */
	{
		.base_addr = 0x20000U,
		.count     = 10U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_51,
	},
	/* Group 52 seg 0: scattered 10 offsets */
	{
		.base_addr = 0x120000U,
		.count     = 10U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_51,
	},
	/* Group 53 seg 0: sequential 96 offsets from 0x124 */
	{
		.base_addr    = 0x20000U,
		.count        = 96U,
		.type         = DDRPHY_GROUP_SEQUENTIAL,
		.start_offset = 0x124U,
	},
	/* Group 53 seg 1: sequential 96 offsets from 0x18B */
	{
		.base_addr    = 0x20000U,
		.count        = 96U,
		.type         = DDRPHY_GROUP_SEQUENTIAL,
		.start_offset = 0x18BU,
	},
	/* Group 53 seg 2: scattered 42 offsets */
	{
		.base_addr = 0x20000U,
		.count     = 42U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_53,
	},
	/* Group 54 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x10000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 55 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x110000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 56 seg 0: scattered 61 offsets */
	{
		.base_addr = 0x10000U,
		.count     = 61U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_56,
	},
	/* Group 57 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x11000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 58 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x111000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 59 seg 0: scattered 61 offsets */
	{
		.base_addr = 0x11000U,
		.count     = 61U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_56,
	},
	/* Group 60 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x12000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 61 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x112000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 62 seg 0: scattered 61 offsets */
	{
		.base_addr = 0x12000U,
		.count     = 61U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_56,
	},
	/* Group 63 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x13000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 64 seg 0: scattered 134 offsets */
	{
		.base_addr = 0x113000U,
		.count     = 134U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_54,
	},
	/* Group 65 seg 0: scattered 61 offsets */
	{
		.base_addr = 0x13000U,
		.count     = 61U,
		.type      = DDRPHY_GROUP_SCATTERED,
		.offsets   = ddrphy_offsets_group_56,
	},
};

static uint32_t get_lpddr_training_message(uint32_t dphy_base, uint16_t* training_msg,
	uint32_t msg_size)
{
	int i;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	for (i = 0; i < msg_size / sizeof(uint16_t); i++) {
		training_msg[i] = phy_reg[DDRPHY_DMEM_BASE_ADDR + i];
	}

	return i;
}

static uint32_t get_lpddr_training_param(uint32_t dphy_base, uint16_t* phy_param)
{
	int i, j, k;
	uint32_t reg_base_addr;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;
	const ddrphy_reg_group_t* reg_group;

	for (i = 0, k = 0; i < ARRAY_SIZE(ddrphy_reg_groups); i++) {
		reg_group = &ddrphy_reg_groups[i];

		if (DDRPHY_GROUP_SEQUENTIAL == reg_group->type) {
			reg_base_addr = reg_group->base_addr + reg_group->start_offset;
		} else {
			reg_base_addr = reg_group->base_addr;
		}

		for (j = 0; j < reg_group->count; j++, k++) {
			// Process each register in the group
			if (DDRPHY_GROUP_SEQUENTIAL == reg_group->type) {
				phy_param[k] = phy_reg[reg_base_addr + j];
			} else {
				phy_param[k] = phy_reg[reg_base_addr + reg_group->offsets[j]];
			}
		}
	}

	if (k != DDR_TRAINING_PHYPARA_HWORDS) {
		pr_err("Training data size mismatch %d != %d\n", k, DDR_TRAINING_PHYPARA_HWORDS);
		return k;
	}

	return DDR_TRAINING_PHYPARA_HWORDS;
}

static uint32_t get_acsm_sram_training_data(uint32_t dphy_base, uint16_t* training_data)
{
	int i;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	for (i = 0; i < DDR_TRAINING_ACSMSRAM_HWORDS; i++) {
		training_data[i] = phy_reg[ACSM_SRAM_BASE_ADDR + i];
	}

	return i;
}

void save_snps_ddrc_training_result(uint32_t ddrc_base, ddr_training_info_t* training_info)
{
	uint32_t count;
	unsigned long dphy_base = ddrc_base + 0x800000;
	volatile uint32_t* phy_reg = (uint32_t*)dphy_base;

	// set csrACSMWckWriteToggleDelayReserved[0] = 1
	phy_reg[0x20037] |= BIT(6);

	phy_reg[MICRO_CONT_MUX_SEL] = 0;
	phy_reg[UCCLK_HCLK_ENABLES] = 3;

	count = get_lpddr_training_message(dphy_base, (uint16_t*)&training_info->msg,
		sizeof(training_info->msg));
	pr_info("LPDDR5 Training message: %ld bytes\n", count * sizeof(uint16_t));
	count = get_lpddr_training_param(dphy_base, training_info->phypara);
	pr_info("LPDDR5 Training param: %ld bytes\n", count * sizeof(uint16_t));
	count = get_acsm_sram_training_data(dphy_base, training_info->acsm);
	pr_info("LPDDR5 acsm sram: %ld bytes\n", count * sizeof(uint16_t));

	phy_reg[MICRO_CONT_MUX_SEL] = 0;
	phy_reg[UCCLK_HCLK_ENABLES] = 1;
}
