# SpacemiT K3 Pico ITX

The port boots the Pico ITX board from SPI NOR or a BootROM USB recovery
image. Coreboot owns SRAM entry, early EC cooling, LPDDR4X/LPDDR5 training and
payload loading. OpenSBI supplies machine-mode services; EDK2 supplies
UEFI device protocols, Setup and Fastboot.

## Build

Initialize the normal coreboot dependencies, including the pinned vboot
submodule. Build OpenSBI using the generic `defconfig` and an entry
address of `0x100000000`, then select the board with:

```sh
cp configs/config.spacemit_k3_pico_itx_normal .config
CROSS_COMPILE_riscv=riscv64-linux-gnu- make menuconfig
CROSS_COMPILE_riscv=riscv64-linux-gnu- make olddefconfig
CROSS_COMPILE_riscv=riscv64-linux-gnu- make
```

Set `CONFIG_OPENSBI_BLOB_PATH` to the built OpenSBI ELF in menuconfig.
The configuration does not assume a source or output directory layout.
The normal profile uses `fw_dynamic.elf`. Its link address places OpenSBI
in the reserved DRAM window; coreboot passes the payload entry and mode
separately through `fw_dynamic_info` at runtime.
The configuration builds coreboot with an external OpenSBI image; it
does not package an EDK2 payload or write a board.

## Ownership

Keep carrier wiring and firmware media layout in the board directory.
The SoC code owns common PMU, eSPI, QSPI, timer and DDR operations.
The EC fan full-duty request is an early prerequisite for the native
boot path. OpenSBI owns machine-mode services and X100/A100 hart startup.
EDK2 owns its UEFI device protocols and setup.

## CPU selection

UEFI Setup selects X100 only (default), X100 + A100, or A100 only.
X100 only preserves the original CPU, topology and interrupt description.
Mixed mode adds harts 8 through 15 and removes vector and hypervisor ISA
extensions from every CPU. CPU nodes remain in ascending hart order.

A100 descriptions follow the vendor Linux CPU tree at commit
`0bef28bfeb51b20a62d82232de44556df942f4dc`: `spacemit,a100`, 32 KiB L1
instruction/data caches and a shared 1 MiB L2 cache per four-hart cluster.
Its ISA list is independent of X100 and filtered only for the selected mode.

A100-only mode retains A100 vectors and omits H. OpenSBI initially receives
all sixteen harts because K3 cold initialization runs on X100 hart 0.
Coreboot sets `/chosen/boot-hartid` to 8 and builds an A100-only CPU map.
Before creating its HOBs, EDK2 uses standard SBI HSM calls to transfer to
hart 8 and stop hart 0. The A100 entry waits for hart 0 to stop before
reusing the SEC stack. EDK2 disables X100 nodes in its private DT copy;
Linux consequently boots on A100 with only harts 8 through 15 available.

CPU configuration occupies the 4 KiB NOR sector at `0x20f000`, at the end
of `EARLY_RESERVED`. Coreboot reads physical NOR even during USB RAM boot.
The 16-byte little-endian record is `K3CP` (4 bytes), version 1 (u16),
record size 16 (u16), mode (u32: 0=X100, 1=mixed, 2=A100) and
CRC-32/ISO-HDLC (u32, over the first 12 bytes). Erased, unsupported or
invalid records select X100 only. Firmware updates reset this sector
independently of the preserved UEFI variable store.

The S-mode IMSIC aperture covers sixteen physical hart slots in both
A100 modes. Its `riscv,guest-index-bits` remains 6 because this also
defines the physical hart stride. IRQ lists retain the disabled X100
slots in A100-only mode so the A100 interrupt files keep their addresses.

## Power and memory

APMU power-domain operations live in the K3 SoC code. Both boot
profiles ensure AUDIO is on before handoff; Linux treats it as an
always-on domain. EDK2 and Linux own their device consumers in their
respective stages. These operations preserve unrelated APMU clock/reset
fields and never power off the APMU controller itself.

Coreboot reserves the 2 MiB OpenSBI load window. Each X100 cluster uses
its own PLL at 2.4 GHz: PLL3 for cluster 0 and PLL4 for cluster 1. Both
clusters fall back to PLL1 / 3 if the operating point cannot be set.

The board selects the CPU regulator from CRC-validated EEPROM PMIC Type
(TLV 0x80). Pico ITX uses AU4562 on TWSI8 address 0x75: page 1 supplies
X100, with a 5 mV VOUT_COMMAND step. The CPU startup requests 1.0 V and
verifies the command before changing PLLs. Page 0 supplies A100 and is
initialized to 950 mV before the hart startup handoff. The previous page
selection is restored after access.
Records without PMIC Type retain the MPQ identity-checked path used on
DEB1; the `mpq8655` TLV name selects that path. Other declared regulator
types cannot enable the maximum CPU operating point.

The common P1 PMIC enables BUCK1..6, ALDO1/2/4 and DLDO1..7 before
payload handoff. These are the board's boot-on rails. Their voltage
selectors are preserved, ALDO3 remains untouched, and external supply
shutdown on standby is disabled without changing unrelated controls.

Pico ITX identifies itself through EEPROM Product Name (TLV 0x21).
For this board, coreboot checks EC GPIO8 (`EC_SW_USB2_PWREN1`) and enables
the FE1.1S USB2 hub's 3.3 V supply if it is off. The EC reports the pin's
name and direction before a name-based GPIO command is issued. Readback
must confirm the enable, and an already powered hub is not cycled.
DEB1 does not use this board-specific operation. Firmware retains hub
power at OS handoff; this does not add Linux control of the EC GPIO.

## DDR source provenance

The controller/PHY training sources, register tables and compressed firmware
are unchanged from revision `e4043c70d6e921238fe43e485f1efc71b30b00d0`.
[k3-ddr-source.json](k3-ddr-source.json) records the original hashes and the
coreboot API adaptations. Both LPDDR4X and LPDDR5, including quickboot,
are built. Board EEPROM TLV 0x45 selects the part; if unavailable, the board
uses the same default part number as its SPL device tree. The original
part table supplies capacity, rank count, bus organization and data rate.

The PHY firmware is decompressed into audio SRAM at `0xc08d0000` after
restoring its PMA attributes. Training messages, waits, PLL status return
and controller order follow the original functions. No overall controller
timeout is added. The original small memory pattern check is retained.

Normal NOR boot reads the training record at SPI offset `0x10000`.
USB download and SD boot force full training. A valid record must match
the selected DDR type, rank count and data rate and pass the original CRC.
EDK2 saves a new cold-training record in the same `0x9800`-byte format
only when BootROM reports an actual NOR boot;
quickboot clears the marker and does not request another save. Firmware
updates erase the record, causing full training on the following boot.
RAM recovery does not write a training record. It fits inside BOOTINFO,
before the bootblock at `0x20000`, and is separate from UEFI variables at `0x700000`.

The BootROM entry is extracted from the packaged SPI image. USB boot
receives that same complete image over Fastboot into the ROM staging
window. NOR boot reads the firmware regions from flash. Both paths then
use the same FMAP/CBFS verification, stages and FW_DYNAMIC payload loading.
The entry and full image must come from the same build because the entry
contains the CBFS metadata hash anchor. Coreboot's USB transport does not
implement storage writes; the shared EDK2 Fastboot application owns them.

## Validation limits

Earlier testing on a 16 GiB LPDDR5 DEB1 covered cold training, persistent
quickboot, firmware updates preserving NV, USB image loading, standard
CBFS handoff and automatic RAM Fastboot. DP/GOP and the
Fastboot ready screen pass when the capture receiver is active before
boot. Late receiver connection does not trigger a new DP training attempt.

On 32 GiB Pico ITX, EEPROM identification, cold training, the original
memory pattern check, AU4562 command readback, both X100 clusters at
2.4 GHz and RAM Fastboot have passed. HSM start/stop and shared-data
checks passed twice on harts 1-7, followed by 32768 concurrent atomic
updates across all eight harts. These checks do not establish sustained
load stability.

On a 16 GiB Pico ITX, X100-only and A100-only modes booted the installed
RVA23U64 root filesystem. Per-core RVV and secondary-hart hotplug passed
with VLEN 256 on X100 and VLEN 1024 on A100. Mixed mode passed per-core
scalar checks and all fifteen secondary-hart hotplug cycles with a
standalone RV64GC initramfs. Its device tree omitted V and H and retained
ascending hart order. DEBUG firmware installation preserved UEFI variables,
passed full SPI readback, and booted the installed OS from NOR.

LPDDR4X populations still require hardware validation. RAM recovery
preserves existing SPI training records and uses emulated variables.
Keep the SPI offsets and UEFI variable reservation when packaging an image.
