/* SPDX-License-Identifier: BSD-2-Clause-Patent */
#ifndef K3_DWC3_H
#define K3_DWC3_H
#define GCTL          0xc110
#define GSNPSID       0xc120
#define GUSB2PHYCFG   0xc200
#define GUSB3PIPECTL  0xc2c0
#define GEVNTADRLO    0xc400
#define GEVNTADRHI    0xc404
#define GEVNTSIZ      0xc408
#define GEVNTCOUNT    0xc40c
#define DCFG          0xc700
#define DCTL          0xc704
#define DEVTEN        0xc708
#define DSTS          0xc70c
#define DALEPENA      0xc720
#define DEPCMDPAR2(n)  (0xc800 + 16 * (n))
#define DEPCMDPAR1(n)  (0xc804 + 16 * (n))
#define DEPCMDPAR0(n)  (0xc808 + 16 * (n))
#define DEPCMD(n)      (0xc80c + 16 * (n))
#define CMD_ACTIVE    (1U << 10)
#define CMD_STATUS    (0xfU << 12)
#define CMD_CONFIG    1
#define CMD_RESOURCE  2
#define CMD_STALL     4
#define CMD_UNSTALL   5
#define CMD_START     6
#define CMD_END       8
#define CMD_NEW       9
#define CORE_RESET    (1U << 30)
#define RUN_STOP      (1U << 31)
#define CORE_HALTED   (1U << 22)
#define TRB_NORMAL    1
#define TRB_SETUP     2
#define TRB_STATUS2   3
#define TRB_STATUS3   4
#define TRB_DATA      5
#define TRB_HWO       (1U << 0)
#define TRB_LAST      (1U << 1)
#define TRB_ISP       (1U << 10)
#define TRB_IOC       (1U << 11)
#define EVENT_BYTES   4096
#define BULK_BYTES    (16 * 1024)

struct dwc3_trb {
  uint32_t    low;
  uint32_t    high;
  uint32_t    size;
  uint32_t    control;
  uint8_t     padding[48]; // One cache line per independent DMA ownership unit.
};

#endif
