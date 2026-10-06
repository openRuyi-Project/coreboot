// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Spacemit
 */

#ifndef _K3_LPDDR_MSG_BLOCK_H_
#define _K3_LPDDR_MSG_BLOCK_H_

typedef struct _PMU_SMB_LPDDR5_1D_t {
	uint8_t  Reserved00;	// Byte offset 0x00, CSR Addr 0x58000, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MsgMisc;	// Byte offset 0x01, CSR Addr 0x58000, Direction=In
				// Contains various global options for training.
				//
				// Bit fields:
				//
				// MsgMisc[0] MTESTEnable
				//      0x1 = Pulse primary digital test output bump at the end of each major training stage. This enables observation of training stage completion by observing the digital test output.
				//      0x0 = Do not pulse primary digital test output bump
				//
				// MsgMisc[1] SimulationOnlyReset
				//      0x1 = Verilog only simulation option to shorten duration of DRAM reset pulse length to 1ns.
				//                Must never be set to 1 in silicon.
				//      0x0 = Use reset pulse length specifed by JEDEC standard
				//
				// MsgMisc[2] SimulationOnlyTraining
				//      0x1 = Verilog only simulation option to shorten the duration of the training steps by performing fewer iterations.
				//                Must never be set to 1 in silicon.
				//      0x0 = Use standard training duration.
				//
				// MsgMisc[3] Disable Boot Clock
				//      0x1 = Disable boot frequency clock when initializing DRAM. (not recommended)
				//      0x0 = Use Boot Frequency Clock
				//
				// MsgMisc[4] Suppress streaming messages, including assertions, regardless of HdtCtrl setting.
				//            Stage Completion messages, as well as training completion and error messages are
				//            Still sent depending on HdtCtrl setting.
				//
				// MsgMisc[5] RFU, must be zero
				//
				// MsgMisc[6] Average out WrLvl delay values and PHY DCA values
				//      0x1 = FW calculates average of training results across both the ranks and stages and loads this new value to delay CSRs for both ranks, i.e.
				//            TxWckDly = (TxWckDlyTg0 + TxWckDlyTg1) /2 ;
				//            If TxDCA + RxDCA enabled and dual RANKs
				//                Tx/RxWCKDcaFinePDT = (min(TxWCKDcaFinePDTTg0, TxWCKDcaFinePDTTg1, RxWCKDcaFinePDTTg0, RxWCKDcaFinePDTTg1) + max(TxWCKDcaFinePDTTg0, TxWCKDcaFinePDTTg1, RxWCKDcaFinePDTTg0, RxWCKDcaFinePDTTg1)) /2;
				//            If TxDCA + RxDCA enabled and single RANK
				//                Tx/RxWCKDcaFinePDT = (TxWCKDcaFinePDTTg0 + RxWCKDcaFinePDTTg0) /2;
				//            If TxDCA enabled and dual RANKs
				//                Tx/RxWCKDcaFinePDT = (TxWCKDcaFinePDTTg0 + TxWCKDcaFinePDTTg1) /2;
				//            If RxDCA enabled and dual RANKs
				//                Tx/RxWCKDcaFinePDT = (RxWCKDcaFinePDTTg0 + RxWCKDcaFinePDTTg1) /2;
				//      0x0 = Delay CSRs for each rank and stage has independent value which is based on its training result (default mode)
				//
				//
				// MsgMisc[7] RFU, must be zero
				//
				// Notes:
				//
				// - SimulationOnlyReset and SimulationOnlyTraining can be used to speed up simulation run times, and must never be used in real silicon. Some VIPs may have checks on DRAM reset parameters that may need to be disabled when using SimulationOnlyReset.
	uint16_t PmuRevision;	// Byte offset 0x02, CSR Addr 0x58001, Direction=Out
				// PMU firmware revision ID
				// After training is run, this address will contain the revision ID of the firmware
	uint8_t  Pstate;	// Byte offset 0x04, CSR Addr 0x58002, Direction=In
				// Must be set to the target Pstate to be trained up to 15
				//  Pstate [7] = when set will use 15 Pstate Mode DMA transfer
	uint8_t  PllBypassEn;	// Byte offset 0x05, CSR Addr 0x58002, Direction=In
				// Set according to whether target Pstate uses PHY PLL bypass
				//    0x0 = PHY PLL is enabled for target Pstate
				//    0x1 = PHY PLL is bypassed for target Pstate
	uint16_t DRAMFreq;	// Byte offset 0x06, CSR Addr 0x58003, Direction=In
				// DDR data rate for the target Pstate in units of MT/s.
				// For example enter 0x0640 for DDR1600.
	uint8_t  DfiFreqRatio;	// Byte offset 0x08, CSR Addr 0x58004, Direction=In
				// Frequency ratio betwen MemClk and SDRAM WCK.
				//    0x2 = 1:2
				//    0x4 = 1:4
	uint8_t  BitTimeControl;	// Byte offset 0x09, CSR Addr 0x58004, Direction=In
				// BitTimeControl[0-2]:
				// Input for the amount of data bits 1D/2D WFF/RFF per DQ before deciding if any specific voltage and delay setting passes or fails. Every time this input increases by 1, the number of 1D/2D data comparisons is doubled. The 1D/2D run time will increase proportionally to the number of bit times requested per point.
				// 0 = 2^0 times of basic amount (default behavior)
				// 1 = 2^1 times of basic amount
				// 2 = 2^2 times of basic amount
				//  . . .
				// 7 = 2^7 times of basic amount
				//
				// [3-7]: RFU, must be zero
	uint16_t Train2DMisc;	// Byte offset 0x0a, CSR Addr 0x58005, Direction=In
				// 2D Training Miscellaneous Control
				//
				// Bit fields:
				// Train2DMisc[0]: Print Verbose 2D Eye Contour
				//   0 = Do Not Print Verbose Eye Contour  (default behavior)
				//   1 = Print Verbose Eye Contour
				//
				// Train2DMisc[1]: Print Verbose Eye Optimization Output
				//   0 = Do Not Print Verbose Eye Optimization Output  (default behavior)
				//   1 = Print Verbose Eye Optimization Output
				//
				// Train2DMisc[5:2]: Iteration Count for Optimization Algorithm
				// Iteration count = Train2DMisc[5:2] << 1
				// Iteration count == 0 is default count = 16
				// iteration count == 2 early termination
				//
				// Train2DMisc[7:6]: Number of Seeds for Optimization Algorithm
				// 0 = 2 seeds, left and right of center, default behavior
				// 1 = 1 seed, center seed
				// 2 = 2 seeds, left and right of center
				// 3 = 3 seeds, left, center and right
				//
				// Train2DMisc[8]: Print Eye Contours prior to optimization
				// 0 = Do Not Print Eye Contours prior to optimization (default behavior)
				// 1 = Print Eye Contours prior to optimization
				//
				// Train2DMisc[9]: Print full eye contours (instead of half)
				// 0 = Print Half Eye Contours (default behavior)
				// 1 = Print Full Eye Contours
				//
				// Train2DMisc[10]: Use weighted mean algorithm for optimization of RX compounded eyes with DFE
				// 0 = Use largest empty circle hill climb (default behavior)
				// 1 = Use weighted mean
				//
				// Train2DMisc[12:11]: Weighted mean algorithm bias function.
				// 0 = Use regular weighted mean
				// 1 = Use weighted mean with voltage squared
				// 2 = Use weighted mean with log2 voltage
				//
				// Train2DMisc[13]: Override RxVref runtime improvement scheme
				// 0 = runtime scheme with RxVref range set by VrefStart and Vref End
				// 1 = runtime speed scheme with range set by number of points before and after Si Friendly trained point
	uint8_t  Reserved0C;	// Byte offset 0x0c, CSR Addr 0x58006, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Misc;	// Byte offset 0x0d, CSR Addr 0x58006, Direction=In
				// Lp4/5 specific options for training.
				//
				// Bit fields:
				//
				// Misc[0] Enable dfi_reset_n
				//
				// 0x0 = (Recommended) PHY internal registers control BP_MEMRESET_L pin until end of training.
				//  See PUB databook for requirement of dfi_reset_n control by MC before 1st dfi_init_start sequence.
				//
				// 0x1 = Enables dfi_reset_n to control BP_MEMRESET_L pin during training.
				//  To ensure that no glitches occur on BP_MEMRESET at the end of training,
				//  The MC must drive dfi_reset_n=1'b1 prior to starting training and keep its value until the end of training.
				//
				//
				// Misc[1] unused
				// Misc[2] unused
				// Misc[3] Enable 4UI Si Friendly Scan
				// 0: 4UI scan
				// 1: 2UI scan
				// Misc[4] PRBS Read training seeding
				// 0: Use si friendly trained result
				// 1: Use RxReplica Estimate
				//  Misc[5] Pre Compute RxClk Coarse bit
				//  0: compute RxClk coarse bit after generating both sets of eyes
				//  1: estimate RxClk Coarse bit before RxClk training
				//  Misc[6] Single RxClk scan in SI Friendly Read
				//  0: Run both RxClkT and RxClkC scan
				//  1: Run only RxClkT scan
				//  Please note that Misc[6] should be set to 0 for datarate lower than 3200Mbps
				// Misc[7] RFU, must be zero
	int8_t   SIFriendlyDlyOffset;	// Byte offset 0x0e, CSR Addr 0x58007, Direction=In
				// SI Friendly Delay Offset
				// SIFriendlyDlyOffset[7:1]
				// This field can be used to modify the trained delay of an eye to be equal to an offset from the edge of that eye for the trained value of the voltage. This can be useful when performing SI friendly 2D training and encountering eye collapse in later training.
				// SIFriendlyDlyOffset[7:1] = 0  Disable this mechanism
				// SIFriendlyDlyOffset[7:1] > 0  Add offset to delay left edge of eye
				// SIFriendlyDlyOffset[7:1] < 0 Subtract offset from delay right edge of eye
				//
				// TruncV
				// SIFriendlyDlyOffset[0]
				//    0 = 2D Normal optimization. Treat any point outside of tested eye rectangle as failing.
				//    1 = If eye is truncated at low voltages treat points at voltages lower than the minimum tested voltage as passing. The trained point will always be at a voltage above the minimum tested voltage.
	uint8_t  CsTestFail;	// Byte offset 0x0f, CSR Addr 0x58007, Direction=Out
				// This field will be set if training fails on any rank.
				//    0x0 = No failures
				//    non-zero = one or more ranks failed training
	uint16_t SequenceCtrl;	// Byte offset 0x10, CSR Addr 0x58008, Direction=In
				// Controls the training steps to be run. Each bit corresponds to a training step.
				//
				// If the bit is set to 1, the training step will run.
				// If the bit is set to 0, the training step will be skipped.
				//
				// Training step to bit mapping:
				//    SequenceCtrl[0] = Run DevInit - Device/phy initialization. Should always be set.
				//    SequenceCtrl[1] = Run WrLvl - Write leveling
				//    SequenceCtrl[2] = Run RxEn - Read gate training
				//    SequenceCtrl[3] = Run RdDQS - read dqs training
				//    SequenceCtrl[4] = Run WrDq - write dq training
				//    SequenceCtrl[5] = RFU, must be zero
				//    SequenceCtrl[6] = Run DRAM DCA - Dram duty cycle adjustment
				//    SequenceCtrl[7] = Run PHY RdDCA - PHY read duty cycle adjustment
				//    SequenceCtrl[8] = Run PHT WrDCA - PHY write duty cycle adjustment
				//    SequenceCtrl[9] = Run MxRdLat - Max read latency training
				//    SequenceCtrl[10] = Run TxDFE - DRAM Tx DFE
				//    SequenceCtrl[11] = RFU, must be zero
				//    SequenceCtrl[12] = Run LPCA - CA Training
				//    SequenceCtrl[15-13] = RFU, must be zero
	uint8_t  HdtCtrl;	// Byte offset 0x12, CSR Addr 0x58009, Direction=In
				// To control the total number of debug messages, a verbosity subfield (HdtCtrl, Hardware Debug Trace Control) exists in the message block. Every message has a verbosity level associated with it, and as the HdtCtrl value is increased, less important s messages stop being sent through the mailboxes. The meanings of several major HdtCtrl thresholds are explained below:
				//
				//    0x05 = Detailed debug messages (e.g. Eye delays)
				//    0x0A = Coarse debug messages (e.g. rank information)
				//    0xC8 = Stage completion
				//    0xC9 = Assertion messages
				//    0xFF = Firmware completion messages only
				//
				// See Training App Note for more detailed information on what messages are included for each threshold.
				//
	uint8_t  Reserved13;	// Byte offset 0x13, CSR Addr 0x58009, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint16_t InternalStatus;	// Byte offset 0x14, CSR Addr 0x5800a, Direction=Out
				// RFU
	uint8_t  DFIMRLMargin;	// Byte offset 0x16, CSR Addr 0x5800b, Direction=In
				// Margin added to smallest passing trained DFI Max Read Latency value, in units of DFI clocks. Recommended to be >= 1. See the Training App Note for more details on the training process and the use of this value.
				//
				// This margin must include the maximum positive drift expected in tDQSCK over the target temperature and voltage range of the users system.
	uint8_t  TX2D_Delay_Weight;	// Byte offset 0x17, CSR Addr 0x5800b, Direction=In
				// [0-4] 0 ... 31
				// During TX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * TX_Delay_Weight2D + voltage margin * TX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  TX2D_Voltage_Weight;	// Byte offset 0x18, CSR Addr 0x5800c, Direction=In
				// [0-4] 0 ... 31
				// During TX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * TX_Delay_Weight2D + voltage margin * TX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  Quickboot;	// Byte offset 0x19, CSR Addr 0x5800c, Direction=In
				// Enable Quickboot.
	uint8_t  Reserved1A;	// Byte offset 0x1a, CSR Addr 0x5800d, Direction=In
	uint8_t  CATrainOpt;	// Byte offset 0x1b, CSR Addr 0x5800d, Direction=In
				// CA training option bit field
				// [0] CA VREF Training
				//        1 = Enable CA VREF Training
				//        0 = Disable CA VREF Training
				// [1] LP5 CS training
				//        1 = Enable LP5 CS Training
				//        0 = Disable LP5 CS Training
				//      This bit is don't care for LP4 training
				// [2] RFU must be zero
				// [3] Delayed clock feature
				//        0 = Use delayed clock
				//        1 = Use normal clock
				//  [4-7] Value by which ACTxDly is to be incremented during CA/CS training:
				//       If bit 7 is set, delay is incremented by 8,
				//       If bit 6 is set, delay is incremented by 4,
				//       if bit 5 is set, delay is incremented by 2
				//       else delay is incremented by 1
				//       This helps in reducing test run time during simulations. For silicon, it is recommended to increment delay by steps of 1 only
	uint8_t  X8Mode;	// Byte offset 0x1c, CSR Addr 0x5800e, Direction=In
				// X8Mode is encoded as a bit field for channel and rank.
				// Bit = 0 means x16 devices are connected.
				// Bit = 1 means 2 x8 devices are connected.
				// This field should be treated as if you have a 2 channel system. Valid Settings are 0xf/0xa/0x0.
				// X8Mode [0] - Channel A Rank 0
				// X8Mode [1] - Channel A Rank 1
				// X8Mode [2] - Channel B Rank 0
				// X8Mode [3] - Channel B Rank 1
				//
	uint8_t  RX2D_TrainOpt;	// Byte offset 0x1d, CSR Addr 0x5800e, Direction=In
				// RxClk Training Option
				// [0] Scan RxClkCDly during RxClkT training
				//        1 = Do not scan RxClkCDly during RxClkT training
				//        0 = Scan RxClkCDly during RxClkT training(Default)
				// [1-7] RFU must be zero
				// It is recommended to set RX2D_TrainOpt to 0
	uint8_t  TX2D_TrainOpt;	// Byte offset 0x1e, CSR Addr 0x5800f, Direction=In
				// RFU
	uint8_t  Reserved1F;	// Byte offset 0x1f, CSR Addr 0x5800f, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  RX2D_Delay_Weight;	// Byte offset 0x20, CSR Addr 0x58010, Direction=In
				// [0-4] 0 ... 31
				// During RX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * RX_Delay_Weight2D + voltage margin * RX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  RX2D_Voltage_Weight;	// Byte offset 0x21, CSR Addr 0x58010, Direction=In
				// [0-4] 0 ... 31
				// During RX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * RX_Delay_Weight2D + voltage margin * RX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint16_t PhyConfigOverride;	// Byte offset 0x22, CSR Addr 0x58011, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  EnabledDQsChA;	// Byte offset 0x24, CSR Addr 0x58012, Direction=In
				// Total number of DQ bits enabled in PHY Channel A
	uint8_t  CsPresentChA;	// Byte offset 0x25, CSR Addr 0x58012, Direction=In
				// Indicates presence of DRAM at each chip select for PHY channel A.
				//
				//  0x1 = CS0 is populated with DRAM
				//  0x3 = CS0 and CS1 are populated with DRAM
				//
				// All other encodings are illegal
				//
	int8_t   CDD_ChA_RR_1_0;	// Byte offset 0x26, CSR Addr 0x58013, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RR_0_1;	// Byte offset 0x27, CSR Addr 0x58013, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_1_1;	// Byte offset 0x28, CSR Addr 0x58014, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_1_0;	// Byte offset 0x29, CSR Addr 0x58014, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_0_1;	// Byte offset 0x2a, CSR Addr 0x58015, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_0_0;	// Byte offset 0x2b, CSR Addr 0x58015, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs0 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_1_1;	// Byte offset 0x2c, CSR Addr 0x58016, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_1_0;	// Byte offset 0x2d, CSR Addr 0x58016, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_0_1;	// Byte offset 0x2e, CSR Addr 0x58017, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_0_0;	// Byte offset 0x2f, CSR Addr 0x58017, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WW_1_0;	// Byte offset 0x30, CSR Addr 0x58018, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WW_0_1;	// Byte offset 0x31, CSR Addr 0x58018, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	uint8_t  CATerminatingRankChA;	// Byte offset 0x32, CSR Addr 0x58019, Direction=In
				// Terminating Rank for CA bus on Channel A
				//    0x0 = Rank 0 is terminating rank
				//    0x1 = Rank 1 is terminating rank
	uint8_t  TrainedVREFCA_A0;	// Byte offset 0x33, CSR Addr 0x58019, Direction=Out
				// Trained CA Vref setting for Ch A Rank 0
	uint8_t  TrainedVREFCA_A1;	// Byte offset 0x34, CSR Addr 0x5801a, Direction=Out
				// Trained CA Vref setting for Ch A Rank 1
	uint8_t  TrainedVREFDQ_A0;	// Byte offset 0x35, CSR Addr 0x5801a, Direction=Out
				// Trained DQ Vref setting for Ch A Rank 0
	uint8_t  TrainedVREFDQ_A1;	// Byte offset 0x36, CSR Addr 0x5801b, Direction=Out
				// Trained DQ Vref setting for Ch A Rank 1
	uint8_t  RxClkDly_Margin_A0;	// Byte offset 0x37, CSR Addr 0x5801b, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_A0;	// Byte offset 0x38, CSR Addr 0x5801c, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_A0;	// Byte offset 0x39, CSR Addr 0x5801c, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_A0;	// Byte offset 0x3a, CSR Addr 0x5801d, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  RxClkDly_Margin_A1;	// Byte offset 0x3b, CSR Addr 0x5801d, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_A1;	// Byte offset 0x3c, CSR Addr 0x5801e, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_A1;	// Byte offset 0x3d, CSR Addr 0x5801e, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_A1;	// Byte offset 0x3e, CSR Addr 0x5801f, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  EnabledDQsChB;	// Byte offset 0x3f, CSR Addr 0x5801f, Direction=In
				// Total number of DQ bits enabled in PHY Channel B
	uint8_t  CsPresentChB;	// Byte offset 0x40, CSR Addr 0x58020, Direction=In
				// Indicates presence of DRAM at each chip select for PHY channel B.
				//
				//    0x0 = No chip selects are populated with DRAM
				//    0x1 = CS0 is populated with DRAM
				//    0x3 = CS0 and CS1 are populated with DRAM
				//
				// All other encodings are illegal
				//
	int8_t   CDD_ChB_RR_1_0;	// Byte offset 0x41, CSR Addr 0x58020, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RR_0_1;	// Byte offset 0x42, CSR Addr 0x58021, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_1_1;	// Byte offset 0x43, CSR Addr 0x58021, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_1_0;	// Byte offset 0x44, CSR Addr 0x58022, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_0_1;	// Byte offset 0x45, CSR Addr 0x58022, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_0_0;	// Byte offset 0x46, CSR Addr 0x58023, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs01 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_1_1;	// Byte offset 0x47, CSR Addr 0x58023, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_1_0;	// Byte offset 0x48, CSR Addr 0x58024, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_0_1;	// Byte offset 0x49, CSR Addr 0x58024, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_0_0;	// Byte offset 0x4a, CSR Addr 0x58025, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WW_1_0;	// Byte offset 0x4b, CSR Addr 0x58025, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WW_0_1;	// Byte offset 0x4c, CSR Addr 0x58026, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	uint8_t  CATerminatingRankChB;	// Byte offset 0x4d, CSR Addr 0x58026, Direction=In
				// Terminating Rank for CA bus on Channel B
				//    0x0 = Rank 0 is terminating rank
				//    0x1 = Rank 1 is terminating rank
	uint8_t  TrainedVREFCA_B0;	// Byte offset 0x4e, CSR Addr 0x58027, Direction=Out
				// Trained CA Vref setting for Ch B Rank 0
	uint8_t  TrainedVREFCA_B1;	// Byte offset 0x4f, CSR Addr 0x58027, Direction=Out
				// Trained CA Vref setting for Ch B Rank 1
	uint8_t  TrainedVREFDQ_B0;	// Byte offset 0x50, CSR Addr 0x58028, Direction=Out
				// Trained DQ Vref setting for Ch B Rank 0
	uint8_t  TrainedVREFDQ_B1;	// Byte offset 0x51, CSR Addr 0x58028, Direction=Out
				// Trained DQ Vref setting for Ch B Rank 1
	uint8_t  RxClkDly_Margin_B0;	// Byte offset 0x52, CSR Addr 0x58029, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_B0;	// Byte offset 0x53, CSR Addr 0x58029, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_B0;	// Byte offset 0x54, CSR Addr 0x5802a, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_B0;	// Byte offset 0x55, CSR Addr 0x5802a, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  RxClkDly_Margin_B1;	// Byte offset 0x56, CSR Addr 0x5802b, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_B1;	// Byte offset 0x57, CSR Addr 0x5802b, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_B1;	// Byte offset 0x58, CSR Addr 0x5802c, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_B1;	// Byte offset 0x59, CSR Addr 0x5802c, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  MR1_A0;	// Byte offset 0x5a, CSR Addr 0x5802d, Direction=in
				// Value to be programmed in DRAM Mode Register 1 {Channel A, Rank 0}
	uint8_t  MR1_A1;	// Byte offset 0x5b, CSR Addr 0x5802d, Direction=in
				// Value to be programmed in DRAM Mode Register 1 {Channel A, Rank 1}
	uint8_t  MR1_B0;	// Byte offset 0x5c, CSR Addr 0x5802e, Direction=in
				// Value to be programmed in DRAM Mode Register 1 {Channel B, Rank 0}
	uint8_t  MR1_B1;	// Byte offset 0x5d, CSR Addr 0x5802e, Direction=in
				// Value to be programmed in DRAM Mode Register 1 {Channel B, Rank 1}
	uint8_t  MR2_A0;	// Byte offset 0x5e, CSR Addr 0x5802f, Direction=in
				// Value to be programmed in DRAM Mode Register 2 {Channel A, Rank 0}
	uint8_t  MR2_A1;	// Byte offset 0x5f, CSR Addr 0x5802f, Direction=in
				// Value to be programmed in DRAM Mode Register 2 {Channel A, Rank 1}
	uint8_t  MR2_B0;	// Byte offset 0x60, CSR Addr 0x58030, Direction=in
				// Value to be programmed in DRAM Mode Register 2 {Channel B, Rank 0}
	uint8_t  MR2_B1;	// Byte offset 0x61, CSR Addr 0x58030, Direction=in
				// Value to be programmed in DRAM Mode Register 2 {Channel B, Rank 1}
	uint8_t  MR3_A0;	// Byte offset 0x62, CSR Addr 0x58031, Direction=in
				// Value to be programmed in DRAM Mode Register 3 {Channel A, Rank 0}
	uint8_t  MR3_A1;	// Byte offset 0x63, CSR Addr 0x58031, Direction=in
				// Value to be programmed in DRAM Mode Register 3 {Channel A, Rank 1}
	uint8_t  MR3_B0;	// Byte offset 0x64, CSR Addr 0x58032, Direction=in
				// Value to be programmed in DRAM Mode Register 3 {Channel B, Rank 0}
	uint8_t  MR3_B1;	// Byte offset 0x65, CSR Addr 0x58032, Direction=in
				// Value to be programmed in DRAM Mode Register 3 {Channel B, Rank 1}
	uint8_t  MR10_A0;	// Byte offset 0x66, CSR Addr 0x58033, Direction=in
				// Value to be programmed in DRAM Mode Register 10 {Channel A, Rank 0}
	uint8_t  MR10_A1;	// Byte offset 0x67, CSR Addr 0x58033, Direction=in
				// Value to be programmed in DRAM Mode Register 10 {Channel A, Rank 1}
	uint8_t  MR10_B0;	// Byte offset 0x68, CSR Addr 0x58034, Direction=in
				// Value to be programmed in DRAM Mode Register 10 {Channel B, Rank 0}
	uint8_t  MR10_B1;	// Byte offset 0x69, CSR Addr 0x58034, Direction=in
				// Value to be programmed in DRAM Mode Register 10 {Channel B, Rank 1}
	uint8_t  MR11_A0;	// Byte offset 0x6a, CSR Addr 0x58035, Direction=in
				// Value to be programmed in DRAM Mode Register 11 {Channel A, Rank 0}
	uint8_t  MR11_A1;	// Byte offset 0x6b, CSR Addr 0x58035, Direction=in
				// Value to be programmed in DRAM Mode Register 11 {Channel A, Rank 1}
	uint8_t  MR11_B0;	// Byte offset 0x6c, CSR Addr 0x58036, Direction=in
				// Value to be programmed in DRAM Mode Register 11 {Channel B, Rank 0}
	uint8_t  MR11_B1;	// Byte offset 0x6d, CSR Addr 0x58036, Direction=in
				// Value to be programmed in DRAM Mode Register 11 {Channel B, Rank 1}
	uint8_t  MR12_A0;	// Byte offset 0x6e, CSR Addr 0x58037, Direction=in
				// Value to be programmed in DRAM Mode Register 12 {Channel A, Rank 0}
	uint8_t  MR12_A1;	// Byte offset 0x6f, CSR Addr 0x58037, Direction=in
				// Value to be programmed in DRAM Mode Register 12 {Channel A, Rank 1}
	uint8_t  MR12_B0;	// Byte offset 0x70, CSR Addr 0x58038, Direction=in
				// Value to be programmed in DRAM Mode Register 12 {Channel B, Rank 0}
	uint8_t  MR12_B1;	// Byte offset 0x71, CSR Addr 0x58038, Direction=in
				// Value to be programmed in DRAM Mode Register 12 {Channel B, Rank 1}
	uint8_t  MR13_A0;	// Byte offset 0x72, CSR Addr 0x58039, Direction=in
				// Value to be programmed in DRAM Mode Register 13 {Channel A, Rank 0}
	uint8_t  MR13_A1;	// Byte offset 0x73, CSR Addr 0x58039, Direction=in
				// Value to be programmed in DRAM Mode Register 13 {Channel A, Rank 1}
	uint8_t  MR13_B0;	// Byte offset 0x74, CSR Addr 0x5803a, Direction=in
				// Value to be programmed in DRAM Mode Register 13 {Channel B, Rank 0}
	uint8_t  MR13_B1;	// Byte offset 0x75, CSR Addr 0x5803a, Direction=in
				// Value to be programmed in DRAM Mode Register 13 {Channel B, Rank 1}
	uint8_t  MR14_A0;	// Byte offset 0x76, CSR Addr 0x5803b, Direction=in
				// Value to be programmed in DRAM Mode Register 14 {Channel A, Rank 0}
	uint8_t  MR14_A1;	// Byte offset 0x77, CSR Addr 0x5803b, Direction=in
				// Value to be programmed in DRAM Mode Register 14 {Channel A, Rank 1}
	uint8_t  MR14_B0;	// Byte offset 0x78, CSR Addr 0x5803c, Direction=in
				// Value to be programmed in DRAM Mode Register 14 {Channel B, Rank 0}
	uint8_t  MR14_B1;	// Byte offset 0x79, CSR Addr 0x5803c, Direction=in
				// Value to be programmed in DRAM Mode Register 14 {Channel B, Rank 1}
	uint8_t  MR15_A0;	// Byte offset 0x7a, CSR Addr 0x5803d, Direction=in
				// Value to be programmed in DRAM Mode Register 15 {Channel A, Rank 0}
	uint8_t  MR15_A1;	// Byte offset 0x7b, CSR Addr 0x5803d, Direction=in
				// Value to be programmed in DRAM Mode Register 15 {Channel A, Rank 1}
	uint8_t  MR15_B0;	// Byte offset 0x7c, CSR Addr 0x5803e, Direction=in
				// Value to be programmed in DRAM Mode Register 15 {Channel B, Rank 0}
	uint8_t  MR15_B1;	// Byte offset 0x7d, CSR Addr 0x5803e, Direction=in
				// Value to be programmed in DRAM Mode Register 15 {Channel B, Rank 1}
	uint8_t  MR16_A0;	// Byte offset 0x7e, CSR Addr 0x5803f, Direction=in
				// Value to be programmed in DRAM Mode Register 16 {Channel A, Rank 0}
	uint8_t  MR16_A1;	// Byte offset 0x7f, CSR Addr 0x5803f, Direction=in
				// Value to be programmed in DRAM Mode Register 16 {Channel A, Rank 1}
	uint8_t  MR16_B0;	// Byte offset 0x80, CSR Addr 0x58040, Direction=in
				// Value to be programmed in DRAM Mode Register 16 {Channel B, Rank 0}
	uint8_t  MR16_B1;	// Byte offset 0x81, CSR Addr 0x58040, Direction=in
				// Value to be programmed in DRAM Mode Register 16 {Channel B, Rank 1}
	uint8_t  MR17_A0;	// Byte offset 0x82, CSR Addr 0x58041, Direction=in
				// Value to be programmed in DRAM Mode Register 17 {Channel A, Rank 0}
	uint8_t  MR17_A1;	// Byte offset 0x83, CSR Addr 0x58041, Direction=in
				// Value to be programmed in DRAM Mode Register 17 {Channel A, Rank 1}
	uint8_t  MR17_B0;	// Byte offset 0x84, CSR Addr 0x58042, Direction=in
				// Value to be programmed in DRAM Mode Register 17 {Channel B, Rank 0}
	uint8_t  MR17_B1;	// Byte offset 0x85, CSR Addr 0x58042, Direction=in
				// Value to be programmed in DRAM Mode Register 17 {Channel B, Rank 1}
	uint8_t  MR18_A0;	// Byte offset 0x86, CSR Addr 0x58043, Direction=in
				// Value to be programmed in DRAM Mode Register 18 {Channel A, Rank 0}
	uint8_t  MR18_A1;	// Byte offset 0x87, CSR Addr 0x58043, Direction=in
				// Value to be programmed in DRAM Mode Register 18 {Channel A, Rank 1}
	uint8_t  MR18_B0;	// Byte offset 0x88, CSR Addr 0x58044, Direction=in
				// Value to be programmed in DRAM Mode Register 18 {Channel B, Rank 0}
	uint8_t  MR18_B1;	// Byte offset 0x89, CSR Addr 0x58044, Direction=in
				// Value to be programmed in DRAM Mode Register 18 {Channel B, Rank 1}
	uint8_t  MR19_A0;	// Byte offset 0x8a, CSR Addr 0x58045, Direction=in
				// Value to be programmed in DRAM Mode Register 19 {Channel A, Rank 0}
	uint8_t  MR19_A1;	// Byte offset 0x8b, CSR Addr 0x58045, Direction=in
				// Value to be programmed in DRAM Mode Register 19 {Channel A, Rank 1}
	uint8_t  MR19_B0;	// Byte offset 0x8c, CSR Addr 0x58046, Direction=in
				// Value to be programmed in DRAM Mode Register 19 {Channel B, Rank 0}
	uint8_t  MR19_B1;	// Byte offset 0x8d, CSR Addr 0x58046, Direction=in
				// Value to be programmed in DRAM Mode Register 19 {Channel B, Rank 1}
	uint8_t  MR20_A0;	// Byte offset 0x8e, CSR Addr 0x58047, Direction=in
				// Value to be programmed in DRAM Mode Register 20 {Channel A, Rank 0}
	uint8_t  MR20_A1;	// Byte offset 0x8f, CSR Addr 0x58047, Direction=in
				// Value to be programmed in DRAM Mode Register 20 {Channel A, Rank 1}
	uint8_t  MR20_B0;	// Byte offset 0x90, CSR Addr 0x58048, Direction=in
				// Value to be programmed in DRAM Mode Register 20 {Channel B, Rank 0}
	uint8_t  MR20_B1;	// Byte offset 0x91, CSR Addr 0x58048, Direction=in
				// Value to be programmed in DRAM Mode Register 20 {Channel B, Rank 1}
	uint8_t  MR21_A0;	// Byte offset 0x92, CSR Addr 0x58049, Direction=in
				// Value to be programmed in DRAM Mode Register 21 {Channel A, Rank 0}
	uint8_t  MR21_A1;	// Byte offset 0x93, CSR Addr 0x58049, Direction=in
				// Value to be programmed in DRAM Mode Register 21 {Channel A, Rank 1}
	uint8_t  MR21_B0;	// Byte offset 0x94, CSR Addr 0x5804a, Direction=in
				// Value to be programmed in DRAM Mode Register 21 {Channel B, Rank 0}
	uint8_t  MR21_B1;	// Byte offset 0x95, CSR Addr 0x5804a, Direction=in
				// Value to be programmed in DRAM Mode Register 21 {Channel B, Rank 1}
	uint8_t  MR22_A0;	// Byte offset 0x96, CSR Addr 0x5804b, Direction=in
				// Value to be programmed in DRAM Mode Register 22 {Channel A, Rank 0}
	uint8_t  MR22_A1;	// Byte offset 0x97, CSR Addr 0x5804b, Direction=in
				// Value to be programmed in DRAM Mode Register 22 {Channel A, Rank 1}
	uint8_t  MR22_B0;	// Byte offset 0x98, CSR Addr 0x5804c, Direction=in
				// Value to be programmed in DRAM Mode Register 22 {Channel B, Rank 0}
	uint8_t  MR22_B1;	// Byte offset 0x99, CSR Addr 0x5804c, Direction=in
				// Value to be programmed in DRAM Mode Register 22 {Channel B, Rank 1}
	uint8_t  MR24_A0;	// Byte offset 0x9a, CSR Addr 0x5804d, Direction=in
				// Value to be programmed in DRAM Mode Register 24 {Channel A, Rank 0}
	uint8_t  MR24_A1;	// Byte offset 0x9b, CSR Addr 0x5804d, Direction=in
				// Value to be programmed in DRAM Mode Register 24 {Channel A, Rank 1}
	uint8_t  MR24_B0;	// Byte offset 0x9c, CSR Addr 0x5804e, Direction=in
				// Value to be programmed in DRAM Mode Register 24 {Channel B, Rank 0}
	uint8_t  MR24_B1;	// Byte offset 0x9d, CSR Addr 0x5804e, Direction=in
				// Value to be programmed in DRAM Mode Register 24 {Channel B, Rank 1}
	uint8_t  MR25_A0;	// Byte offset 0x9e, CSR Addr 0x5804f, Direction=in
				// Value to be programmed in DRAM Mode Register 25 {Channel A, Rank 0}
	uint8_t  MR25_A1;	// Byte offset 0x9f, CSR Addr 0x5804f, Direction=in
				// Value to be programmed in DRAM Mode Register 25 {Channel A, Rank 1}
	uint8_t  MR25_B0;	// Byte offset 0xa0, CSR Addr 0x58050, Direction=in
				// Value to be programmed in DRAM Mode Register 25 {Channel B, Rank 0}
	uint8_t  MR25_B1;	// Byte offset 0xa1, CSR Addr 0x58050, Direction=in
				// Value to be programmed in DRAM Mode Register 25 {Channel B, Rank 1}
	uint8_t  MR26_A0;	// Byte offset 0xa2, CSR Addr 0x58051, Direction=in
				// Value to be programmed in DRAM Mode Register 26 {Channel A, Rank 0}
	uint8_t  MR26_A1;	// Byte offset 0xa3, CSR Addr 0x58051, Direction=in
				// Value to be programmed in DRAM Mode Register 26 {Channel A, Rank 1}
	uint8_t  MR26_B0;	// Byte offset 0xa4, CSR Addr 0x58052, Direction=in
				// Value to be programmed in DRAM Mode Register 26 {Channel B, Rank 0}
	uint8_t  MR26_B1;	// Byte offset 0xa5, CSR Addr 0x58052, Direction=in
				// Value to be programmed in DRAM Mode Register 26 {Channel B, Rank 1}
	uint8_t  MR27_A0;	// Byte offset 0xa6, CSR Addr 0x58053, Direction=in
				// Value to be programmed in DRAM Mode Register 27 {Channel A, Rank 0}
	uint8_t  MR27_A1;	// Byte offset 0xa7, CSR Addr 0x58053, Direction=in
				// Value to be programmed in DRAM Mode Register 27 {Channel A, Rank 1}
	uint8_t  MR27_B0;	// Byte offset 0xa8, CSR Addr 0x58054, Direction=in
				// Value to be programmed in DRAM Mode Register 27 {Channel B, Rank 0}
	uint8_t  MR27_B1;	// Byte offset 0xa9, CSR Addr 0x58054, Direction=in
				// Value to be programmed in DRAM Mode Register 27 {Channel B, Rank 1}
	uint8_t  MR28_A0;	// Byte offset 0xaa, CSR Addr 0x58055, Direction=in
				// Value to be programmed in DRAM Mode Register 28 {Channel A, Rank 0}
	uint8_t  MR28_A1;	// Byte offset 0xab, CSR Addr 0x58055, Direction=in
				// Value to be programmed in DRAM Mode Register 28 {Channel A, Rank 1}
	uint8_t  MR28_B0;	// Byte offset 0xac, CSR Addr 0x58056, Direction=in
				// Value to be programmed in DRAM Mode Register 28 {Channel B, Rank 0}
	uint8_t  MR28_B1;	// Byte offset 0xad, CSR Addr 0x58056, Direction=in
				// Value to be programmed in DRAM Mode Register 28 {Channel B, Rank 1}
	uint8_t  MR30_A0;	// Byte offset 0xae, CSR Addr 0x58057, Direction=in
				// Value to be programmed in DRAM Mode Register 30 {Channel A, Rank 0}
	uint8_t  MR30_A1;	// Byte offset 0xaf, CSR Addr 0x58057, Direction=in
				// Value to be programmed in DRAM Mode Register 30 {Channel A, Rank 1}
	uint8_t  MR30_B0;	// Byte offset 0xb0, CSR Addr 0x58058, Direction=in
				// Value to be programmed in DRAM Mode Register 30 {Channel B, Rank 0}
	uint8_t  MR30_B1;	// Byte offset 0xb1, CSR Addr 0x58058, Direction=in
				// Value to be programmed in DRAM Mode Register 30 {Channel B, Rank 1}
	uint8_t  MR31_A0;	// Byte offset 0xb2, CSR Addr 0x58059, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR31_A1;	// Byte offset 0xb3, CSR Addr 0x58059, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR31_B0;	// Byte offset 0xb4, CSR Addr 0x5805a, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR31_B1;	// Byte offset 0xb5, CSR Addr 0x5805a, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR32_A0;	// Byte offset 0xb6, CSR Addr 0x5805b, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR32_A1;	// Byte offset 0xb7, CSR Addr 0x5805b, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR32_B0;	// Byte offset 0xb8, CSR Addr 0x5805c, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR32_B1;	// Byte offset 0xb9, CSR Addr 0x5805c, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR33_A0;	// Byte offset 0xba, CSR Addr 0x5805d, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR33_A1;	// Byte offset 0xbb, CSR Addr 0x5805d, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR33_B0;	// Byte offset 0xbc, CSR Addr 0x5805e, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR33_B1;	// Byte offset 0xbd, CSR Addr 0x5805e, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR34_A0;	// Byte offset 0xbe, CSR Addr 0x5805f, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR34_A1;	// Byte offset 0xbf, CSR Addr 0x5805f, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR34_B0;	// Byte offset 0xc0, CSR Addr 0x58060, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR34_B1;	// Byte offset 0xc1, CSR Addr 0x58060, Direction=in
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MR37_A0;	// Byte offset 0xc2, CSR Addr 0x58061, Direction=in
				// Value to be programmed in DRAM Mode Register 37 {Channel A, Rank 0}
	uint8_t  MR37_A1;	// Byte offset 0xc3, CSR Addr 0x58061, Direction=in
				// Value to be programmed in DRAM Mode Register 37 {Channel A, Rank 1}
	uint8_t  MR37_B0;	// Byte offset 0xc4, CSR Addr 0x58062, Direction=in
				// Value to be programmed in DRAM Mode Register 37 {Channel B, Rank 0}
	uint8_t  MR37_B1;	// Byte offset 0xc5, CSR Addr 0x58062, Direction=in
				// Value to be programmed in DRAM Mode Register 37 {Channel B, Rank 1}
	uint8_t  MR40_A0;	// Byte offset 0xc6, CSR Addr 0x58063, Direction=in
				// Value to be programmed in DRAM Mode Register 40 {Channel A, Rank 0}
	uint8_t  MR40_A1;	// Byte offset 0xc7, CSR Addr 0x58063, Direction=in
				// Value to be programmed in DRAM Mode Register 40 {Channel A, Rank 1}
	uint8_t  MR40_B0;	// Byte offset 0xc8, CSR Addr 0x58064, Direction=in
				// Value to be programmed in DRAM Mode Register 40 {Channel B, Rank 0}
	uint8_t  MR40_B1;	// Byte offset 0xc9, CSR Addr 0x58064, Direction=in
				// Value to be programmed in DRAM Mode Register 40 {Channel B, Rank 1}
	uint8_t  MR41_A0;	// Byte offset 0xca, CSR Addr 0x58065, Direction=in
				// Value to be programmed in DRAM Mode Register 41 {Channel A, Rank 0}
	uint8_t  MR41_A1;	// Byte offset 0xcb, CSR Addr 0x58065, Direction=in
				// Value to be programmed in DRAM Mode Register 41 {Channel A, Rank 1}
	uint8_t  MR41_B0;	// Byte offset 0xcc, CSR Addr 0x58066, Direction=in
				// Value to be programmed in DRAM Mode Register 41 {Channel B, Rank 0}
	uint8_t  MR41_B1;	// Byte offset 0xcd, CSR Addr 0x58066, Direction=in
				// Value to be programmed in DRAM Mode Register 41 {Channel B, Rank 1}
	uint8_t  Disable2D;	// Byte offset 0xce, CSR Addr 0x58067, Direction=In
				// Set to disable 2D training
				// When this field is set to 1, it is not recommended to set RxDfeMode to DFE enabled with 1 previous bit lookup
	uint8_t  VrefSamples;	// Byte offset 0xcf, CSR Addr 0x58067, Direction=In
				// Number of vrefs to scan
				// 0 =  scan default vref
	uint8_t  TrainedVREFDQU_A0;	// Byte offset 0xd0, CSR Addr 0x58068, Direction=Out
				// Trained CA DQ Upper setting for Ch A Rank 0
	uint8_t  TrainedDRAMDFE_A0;	// Byte offset 0xd1, CSR Addr 0x58068, Direction=Out
				// Trained DRAM DFE setting for Ch A Rank 0
				//   Upper byte [6:4]
				//   Lower byte  [2:0]
	uint8_t  TrainedDRAMDCA_A0;	// Byte offset 0xd2, CSR Addr 0x58069, Direction=Out
				// Trained DRAM DCA
				//   Upper byte [7:4]
				//   Lower byte [3:0]
	uint8_t  ReservedD3;	// Byte offset 0xd3, CSR Addr 0x58069, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedD4;	// Byte offset 0xd4, CSR Addr 0x5806a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  TrainedVREFDQU_A1;	// Byte offset 0xd5, CSR Addr 0x5806a, Direction=Out
				// Trained CA DQ Upper setting for Ch A Rank 1
	uint8_t  TrainedDRAMDFE_A1;	// Byte offset 0xd6, CSR Addr 0x5806b, Direction=Out
				// Trained DRAM DFE setting for Ch A Rank 1
				//   Upper byte [6:4]
				//   Lower byte  [2:0]
	uint8_t  TrainedDRAMDCA_A1;	// Byte offset 0xd7, CSR Addr 0x5806b, Direction=Out
				// Trained DRAM DCA
				//   Upper byte [7:4]
				//   Lower byte [3:0]
	uint8_t  ReservedD8;	// Byte offset 0xd8, CSR Addr 0x5806c, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedD9;	// Byte offset 0xd9, CSR Addr 0x5806c, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  TrainedVREFDQU_B0;	// Byte offset 0xda, CSR Addr 0x5806d, Direction=Out
				// Trained CA DQ Upper setting for Ch B Rank 0
	uint8_t  TrainedDRAMDFE_B0;	// Byte offset 0xdb, CSR Addr 0x5806d, Direction=Out
				// Trained DRAM DFE setting for Ch B Rank 0
				//   Upper byte [6:4]
				//   Lower byte  [2:0]
	uint8_t  TrainedDRAMDCA_B0;	// Byte offset 0xdc, CSR Addr 0x5806e, Direction=Out
				// Trained DRAM DCA
				//   Upper byte [7:4]
				//   Lower byte [3:0]
	uint8_t  ReservedDD;	// Byte offset 0xdd, CSR Addr 0x5806e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedDE;	// Byte offset 0xde, CSR Addr 0x5806f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  TrainedVREFDQU_B1;	// Byte offset 0xdf, CSR Addr 0x5806f, Direction=Out
				// Trained CA DQ Upper setting for Ch B Rank 1
	uint8_t  TrainedDRAMDFE_B1;	// Byte offset 0xe0, CSR Addr 0x58070, Direction=Out
				// Trained DRAM DFE setting for Ch B Rank 1
				//   Upper byte [6:4]
				//   Lower byte  [2:0]
	uint8_t  TrainedDRAMDCA_B1;	// Byte offset 0xe1, CSR Addr 0x58070, Direction=Out
				// Trained DRAM DCA
				//   Upper byte [7:4]
				//   Lower byte [3:0]
	uint8_t  ReservedE2;	// Byte offset 0xe2, CSR Addr 0x58071, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE3;	// Byte offset 0xe3, CSR Addr 0x58071, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  RdWrPatternA;	// Byte offset 0xe4, CSR Addr 0x58072, Direction=In
				// Lower-byte read pattern for training
				// When RdWrPatternA, PdWrPatternB and RdWrInvert are all set to 0 uses default patterns
				// When set RxDfeMode to 0x4 and set Disable2D to 0, it is recommended to set this filed to 0
	uint8_t  RdWrPatternB;	// Byte offset 0xe5, CSR Addr 0x58072, Direction=In
				// Upper-byte read pattern for training
				// When RdWrPatternA, PdWrPatternB and RdWrInvert are all set to 0 uses default patterns
				// When set RxDfeMode to 0x4 and set Disable2D to 0, it is recommended to set this filed to 0
	uint8_t  RdWrInvert;	// Byte offset 0xe6, CSR Addr 0x58073, Direction=In
				// Per-byte per bit  invert for read pattern for training
				// Must be used together with RdWrPatternA/RdWrPatternB
				// When set RxDfeMode to 0x4 and set Disable2D to 0, it is recommended to set this filed to 0
	uint8_t  LdffMode;	// Byte offset 0xe7, CSR Addr 0x58073, Direction=In
				// In LDFF mode raw PATN/PRBS sequences driven on DBI & EDC  lanes. If this is set to 0 pattern follows MR settings
				// [0] = 1 Force DBI like patterns on all lanes
				// [1] = 1 Force non DBI patterns on all lanes
	uint16_t FCDfi0AcsmStart;	// Byte offset 0xe8, CSR Addr 0x58074, Direction=In
				// Start Address for MRW commands for DFI0
	uint16_t FCDfi1AcsmStart  ; // Byte offset 0xea, CSR Addr 0x58075, Direction=In
				// Start Address for MRW commands for DFI1
	uint16_t FCDfi0AcsmStartPSY;	// Byte offset 0xec, CSR Addr 0x58076, Direction=In
				// Start Address for MRW commands for DFI0 for the previous PState
	uint16_t FCDfi1AcsmStartPSY;	// Byte offset 0xee, CSR Addr 0x58077, Direction=In
				// Start Address for MRW commands for DFI1 for the previous PState
	uint16_t FCDMAStartMR;	// Byte offset 0xf0, CSR Addr 0x58078, Direction=In
				// Start DMA Address for FCDfi0AcsmStart
	uint16_t FCDMAStartCsr;	// Byte offset 0xf2, CSR Addr 0x58079, Direction=In
				// Start DMA Address for Starting CSR
	uint8_t  EnCustomSettings;	// Byte offset 0xf4, CSR Addr 0x5807a, Direction=In
				// Enable Custome TxSlew and TxImpedance Settings
				//
				// When this field is set to 1, the following LS_ values shall be used in the corresponding AC CSRs during low speed operations.
				// The values are programmed as it is in the CSRs by the firmware, so these should be set very carefully
				//
	uint8_t  LS_TxSlewSE0;	// Byte offset 0xf5, CSR Addr 0x5807a, Direction=In
				// Custom Low Speed AC TxSlew for SE0
	uint8_t  LS_TxSlewSE1;	// Byte offset 0xf6, CSR Addr 0x5807b, Direction=In
				// Custom Low Speed AC TxSlew for SE1
	uint8_t  LS_TxSlewDIFF0;	// Byte offset 0xf7, CSR Addr 0x5807b, Direction=In
				// Custom Low Speed AC TxSlew for SE1
	uint16_t LS_TxImpedanceDIFF0T;	// Byte offset 0xf8, CSR Addr 0x5807c, Direction=In
				// Custom Low Speed AC TxImpedance for DIFF0T
	uint16_t LS_TxImpedanceDIFF0C;	// Byte offset 0xfa, CSR Addr 0x5807d, Direction=In
				// Custom Low Speed AC TxImpedance for DIFF0C
	uint16_t LS_TxImpedanceSE0;	// Byte offset 0xfc, CSR Addr 0x5807e, Direction=In
				// Custom Low Speed AC TxImpedance for SE0
	uint16_t LS_TxImpedanceSE1;	// Byte offset 0xfe, CSR Addr 0x5807f, Direction=In
				// Custom Low Speed AC TxImpedance for SE1
	uint8_t  VrefInc;	// Byte offset 0x100, CSR Addr 0x58080, Direction=In
				// This field should be programmed to 1
				// This controls the vrefIncrement size for 2D training
	uint8_t  UpperLowerByte;	// Byte offset 0x101, CSR Addr 0x58080, Direction=In
				// UpperLowerByte[3:0] - A value of 0 means partner bytes are not swapped. A value of 1 means partner bytes are swapped.
				// [0] : Channel A Rank 0
				// [1] : Channel A Rank 1
				// [2] : Channel B Rank 0
				// [3] : Channel B Rank1
	uint8_t  DisableTrainingLoop;	// Byte offset 0x102, CSR Addr 0x58081, Direction=In
				// Set these fields to skip training steps that are looped
				//   [0] = 1 disable all retraining stages in loop 1: Write leveling, RxEn, Read Dq Cal
				//   [1] = 1 disable all retraining stages in loop 2: Read DQ Cal
				//   [2] = 1 disable all retraining stages in loop 3: Write Training
				//   [3] =  1 disable Write leveling training step in loop 1
				//   [4] =  1 disable RxEn training step in loop 1
				//   [5] =  1 disable Read Dq Cal training step in loop 1
	uint8_t  ALT_RL;	// Byte offset 0x103, CSR Addr 0x58081, Direction=In
				// This is the alternate Read Latency for DBI off
	uint8_t  MAIN_RL;	// Byte offset 0x104, CSR Addr 0x58082, Direction=In
				// This is the main RL calculated by phyinit
	uint8_t  CSBACKOFF;	// Byte offset 0x105, CSR Addr 0x58082, Direction=In
				// Programmable CS delay adjustment
				// CSBACKOFF = 1 : -0.125tCK
				// CSBACKOFF = 2 : -0.25tCK
				// CSBACKOFF = 3 : -0.375tCK
				// CSBACKOFF = default: -0.5tCK
	uint8_t  WrLvlTrainOpt;	// Byte offset 0x106, CSR Addr 0x58083, Direction=In
				// Write leveling training options
				// [0] = When set, coarse wck2ck leveling training is skipped
	uint8_t  Reserved107;	// Byte offset 0x107, CSR Addr 0x58083, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint16_t FCDCCMStartCSR;	// Byte offset 0x108, CSR Addr 0x58084, Direction=out
				// Start Address in DCCM for CSRs to be copied to DMA
	uint16_t FCDCCMLenCSR;	// Byte offset 0x10a, CSR Addr 0x58085, Direction=out
				// number of entries written into DCCM for CSRs
	uint16_t FCDCCMStartMR;	// Byte offset 0x10c, CSR Addr 0x58086, Direction=out
				// Start Address in DCCM for Mrs to be copied to DMA
	uint16_t  FCDCCMLenMR;	// Byte offset 0x10e, CSR Addr 0x58087, Direction=out
				// number of entries written into DCCM for Mrs
	uint8_t  MRLCalcAdj;	// Byte offset 0x110, CSR Addr 0x58088, Direction=In
				// This field is treated as an int_8 and is added to the intermediate MRL values used in training.
	uint8_t  PPT2OffsetMargin;	// Byte offset 0x111, CSR Addr 0x58088, Direction=In
				// When set to 0 disabled, non zero values add that much margin to left and right eye offsets to prevent underflow or overflow.
	uint8_t  Reserved112;	// Byte offset 0x112, CSR Addr 0x58089, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved113;	// Byte offset 0x113, CSR Addr 0x58089, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved114;	// Byte offset 0x114, CSR Addr 0x5808a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved115;	// Byte offset 0x115, CSR Addr 0x5808a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved116;	// Byte offset 0x116, CSR Addr 0x5808b, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved117;	// Byte offset 0x117, CSR Addr 0x5808b, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  RxVrefStartPatDfe0;	// Byte offset 0x118, CSR Addr 0x5808c, Direction=In
				// Starting VREF Value for Rx Training for DFE0 for Pattern Mode
	uint8_t  RxVrefStartPatDfe1;	// Byte offset 0x119, CSR Addr 0x5808c, Direction=In
				// Starting VREF Value for Rx Training for DFE1 for Pattern Mode
	uint8_t  RxVrefStartPrbsDfe0;	// Byte offset 0x11a, CSR Addr 0x5808d, Direction=In
				// Train2Dmisc[13]= 0, Starting VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1, number of points to scan before the si friendly trained vref
	uint8_t  RxVrefStartPrbsDfe1;	// Byte offset 0x11b, CSR Addr 0x5808d, Direction=In
				// Train2Dmisc[13]= 0, Starting VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1, number of points to scan before the si friendly trained vref
	uint8_t  TxVrefStart;	// Byte offset 0x11c, CSR Addr 0x5808e, Direction=In
				// Starting VREF Value for Tx Training for Prbs Mode
	uint8_t  RxVrefEndPatDfe0;	// Byte offset 0x11d, CSR Addr 0x5808e, Direction=In
				// Ending VREF Value for Rx Training for DFE0 for Pattern Mode
	uint8_t  RxVrefEndPatDfe1;	// Byte offset 0x11e, CSR Addr 0x5808f, Direction=In
				// Ending VREF Value for Rx Training for DFE1 for Pattern Mode
	uint8_t  RxVrefEndPrbsDfe0;	// Byte offset 0x11f, CSR Addr 0x5808f, Direction=In
				// Train2Dmisc[13]= 0,Ending VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1,Number of points to scan after the si friendly trained vref
	uint8_t  RxVrefEndPrbsDfe1;	// Byte offset 0x120, CSR Addr 0x58090, Direction=In
				// Train2Dmisc[13]= 0,Ending VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1,Number of points to scan after the si friendly trained vref
	uint8_t  TxVrefEnd;	// Byte offset 0x121, CSR Addr 0x58090, Direction=In
				// Ending VREF Value for Tx Training for Prbs Mode
	uint8_t  RxVrefStepPatDfe0;	// Byte offset 0x122, CSR Addr 0x58091, Direction=In
				// VREF Step Value for Rx Training for DFE0 for Pattern Mode
	uint8_t  RxVrefStepPatDfe1;	// Byte offset 0x123, CSR Addr 0x58091, Direction=In
				// VREF Step Value for Rx Training for DFE1 for Pattern Mode
	uint8_t  RxVrefStepPrbsDfe0;	// Byte offset 0x124, CSR Addr 0x58092, Direction=In
				// VREF Step Value for Rx Training for DFE0 for Prbs Mode
	uint8_t  RxVrefStepPrbsDfe1;	// Byte offset 0x125, CSR Addr 0x58092, Direction=In
				// VREF Step Value for Rx Training for DFE0 for Prbs Mode
	uint8_t  TxVrefStep;	// Byte offset 0x126, CSR Addr 0x58093, Direction=In
				// VREF Step Value for Tx Training for Prbs Mode
	uint8_t  RxDlyScanShiftRank0Byte0;	// Byte offset 0x127, CSR Addr 0x58093, Direction=In
				// Rx delay scan shift for Rank0 Dbyte0.  Examples:
				//   0x1 - start scan 1 step earlier
				//   0xf - start scan 15 steps earlier
				//   0xff - start scan 1 step later
				//   0xfe - start scan 2 steps later
	uint8_t  RxDlyScanShiftRank0Byte1;	// Byte offset 0x128, CSR Addr 0x58094, Direction=In
				// Rx delay scan shift for Rank0 Dbyte1
	uint8_t  RxDlyScanShiftRank0Byte2;	// Byte offset 0x129, CSR Addr 0x58094, Direction=In
				// Rx delay scan shift for Rank0 Dbyte2
	uint8_t  RxDlyScanShiftRank0Byte3;	// Byte offset 0x12a, CSR Addr 0x58095, Direction=In
				// Rx delay scan shift for Rank0 Dbyte3
	uint8_t  RxDlyScanShiftRank1Byte0;	// Byte offset 0x12b, CSR Addr 0x58095, Direction=In
				// Rx delay scan shift for Rank1 Dbyte0
	uint8_t  RxDlyScanShiftRank1Byte1;	// Byte offset 0x12c, CSR Addr 0x58096, Direction=In
				// Rx delay scan shift for Rank1 Dbyte1
	uint8_t  RxDlyScanShiftRank1Byte2;	// Byte offset 0x12d, CSR Addr 0x58096, Direction=In
				// Rx delay scan shift for Rank1 Dbyte2
	uint8_t  RxDlyScanShiftRank1Byte3;	// Byte offset 0x12e, CSR Addr 0x58097, Direction=In
				// Rx delay scan shift for Rank1 Dbyte3
	uint8_t  Reserved12F;	// Byte offset 0x12f, CSR Addr 0x58097, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint16_t QBPllUPllProg0;	// Byte offset 0x130, CSR Addr 0x58098, Direction=out
				// CSR PLLIPProg0 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllUPllProg1;	// Byte offset 0x132, CSR Addr 0x58099, Direction=out
				// CSR PLLIPProg1 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllUPllProg2;	// Byte offset 0x134, CSR Addr 0x5809a, Direction=out
				// CSR PLLIPProg2 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllUPllProg3;	// Byte offset 0x136, CSR Addr 0x5809b, Direction=out
				// CSR PLLIPProg3 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllCtrl1;	// Byte offset 0x138, CSR Addr 0x5809c, Direction=out
				// CSR PllCtrl1 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllCtrl4;	// Byte offset 0x13a, CSR Addr 0x5809d, Direction=out
				// CSR PllCtrl4 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllCtrl5;	// Byte offset 0x13c, CSR Addr 0x5809e, Direction=out
				// CSR PllCtrl5 value for normal mode, reserved for QuickBoot firmware.
	uint8_t  Reserved13E;	// Byte offset 0x13e, CSR Addr 0x5809f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved13F;	// Byte offset 0x13f, CSR Addr 0x5809f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
} __attribute__ ((packed)) PMU_SMB_LPDDR5_1D_t;

typedef struct _PMU_SMB_LPDDR4X_1D_t {
	uint8_t  Reserved00;	// Byte offset 0x00, CSR Addr 0x58000, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  MsgMisc;	// Byte offset 0x01, CSR Addr 0x58000, Direction=In
				// Contains various global options for training.
				//
				// Bit fields:
				//
				// MsgMisc[0] MTESTEnable
				//      0x1 = Pulse primary digital test output bump at the end of each major training stage. This enables observation of training stage completion by observing the digital test output.
				//      0x0 = Do not pulse primary digital test output bump
				//
				// MsgMisc[1] SimulationOnlyReset
				//      0x1 = Verilog only simulation option to shorten duration of DRAM reset pulse length to 1ns.
				//                Must never be set to 1 in silicon.
				//      0x0 = Use reset pulse length specifed by JEDEC standard
				//
				// MsgMisc[2] SimulationOnlyTraining
				//      0x1 = Verilog only simulation option to shorten the duration of the training steps by performing fewer iterations.
				//                Must never be set to 1 in silicon.
				//      0x0 = Use standard training duration.
				//
				// MsgMisc[3] Disable Boot Clock
				//      0x1 = Disable boot frequency clock when initializing DRAM. (not recommended)
				//      0x0 = Use Boot Frequency Clock
				//
				// MsgMisc[4] Suppress streaming messages, including assertions, regardless of HdtCtrl setting.
				//            Stage Completion messages, as well as training completion and error messages are
				//            Still sent depending on HdtCtrl setting.
				//
				// MsgMisc[5] RFU, must be zero
				//
				// MsgMisc[6] Average out WrLvl delay values
				//      0x1 = FW calculates average of training results across both the ranks and loads this new value to delay CSRs for both ranks, i.e.
				//            TxDqsDly[db0] = (TxDqsDlyTg0[db0] + TxDqsDlyTg1[db0]) /2 ;
				//            TxDqsDly[db1] = (TxDqsDlyTg0[db1] + TxDqsDlyTg1[db1]) /2 ;
				//      0x0 = TxDqsDly delay CSR for each rank has independent value which is based on its training result (default mode)
				//
				// MsgMisc[7] RFU, must be zero
				// Notes:
				//
				// - SimulationOnlyReset and SimulationOnlyTraining can be used to speed up simulation run times, and must never be used in real silicon. Some VIPs may have checks on DRAM reset parameters that may need to be disabled when using SimulationOnlyReset.
	uint16_t PmuRevision;	// Byte offset 0x02, CSR Addr 0x58001, Direction=Out
				// PMU firmware revision ID
				// After training is run, this address will contain the revision ID of the firmware
	uint8_t  Pstate;	// Byte offset 0x04, CSR Addr 0x58002, Direction=In
				// Must be set to the target Pstate to be trained 0 -15
				//  Pstate [7] - when set will use 15 Pstate Mode DMA transfer
	uint8_t  PllBypassEn;	// Byte offset 0x05, CSR Addr 0x58002, Direction=In
				// Set according to whether target Pstate uses PHY PLL bypass
				//    0x0 = PHY PLL is enabled for target Pstate
				//    0x1 = PHY PLL is bypassed for target Pstate
	uint16_t DRAMFreq;	// Byte offset 0x06, CSR Addr 0x58003, Direction=In
				// DDR data rate for the target Pstate in units of MT/s.
				// For example enter 0x0640 for DDR1600.
	uint8_t  DfiFreqRatio;	// Byte offset 0x08, CSR Addr 0x58004, Direction=In
				// Frequency ratio betwen DfiCtlClk and SDRAM memclk.
				//    0X2 = 1:2
				//    0x4 = 1:4
	uint8_t  BitTimeControl;	// Byte offset 0x09, CSR Addr 0x58004, Direction=In
				// BitTimeControl[0-2]:
				// Input for the amount of data bits 1D/2D WFF/RFF per DQ before deciding if any specific voltage and delay setting passes or fails. Every time this input increases by 1, the number of 1D/2D data comparisons is doubled. The 1D/2D run time will increase proportionally to the number of bit times requested per point.
				// 0 = 2^0 times of basic amount (default behavior)
				// 1 = 2^1 times of basic amount
				// 2 = 2^2 times of basic amount
				//  . . .
				// 7 = 2^7 times of basic amount
				//
				// [3-7]: RFU, must be zero
	uint16_t Train2DMisc;	// Byte offset 0x0a, CSR Addr 0x58005, Direction=In
				// 2D Training Miscellaneous Control
				//
				// Bit fields:
				// Train2DMisc[0]: Print Verbose 2D Eye Contour
				//   0 = Do Not Print Verbose Eye Contour  (default behavior)
				//   1 = Print Verbose Eye Contour
				//
				// Train2DMisc[1]: Print Verbose Eye Optimization Output
				//   0 = Do Not Print Verbose Eye Optimization Output  (default behavior)
				//   1 = Print Verbose Eye Optimization Output
				//
				// Train2DMisc[5:2]: Iteration Count for Optimization Algorithm
				// Iteration count = Train2DMisc[5:2] << 1
				// Iteration count == 0 is default count = 16
				// iteration count == 2 early termination
				//
				// Train2DMisc[7:6]: Number of Seeds for Optimization Algorithm
				// 0 = 2 seeds, left and right of center, default behavior
				// 1 = 1 seed, center seed
				// 2 = 2 seeds, left and right of center
				// 3 = 3 seeds, left, center and right
				//
				// Train2DMisc[8]: Print Eye Contours prior to optimization
				// 0 = Do Not Print Eye Contours prior to optimization (default behavior)
				// 1 = Print Eye Contours prior to optimization
				//
				// Train2DMisc[9]: Print full eye contours (instead of half)
				// 0 = Print Half Eye Contours (default behavior)
				// 1 = Print Full Eye Contours
				//
				// Train2DMisc[10]: Use weighted mean algorithm for optimization of RX compounded eyes with DFE
				// 0 = Use largest empty circle hill climb (default behavior)
				// 1 = Use weighted mean
				//
				// Train2DMisc[12:11]: Weighted mean algorithm bias function.
				// 0 = Use regular weighted mean
				// 1 = Use weighted mean with voltage squared
				// 2 = Use weighted mean with log2 voltage
				//
				// Train2DMisc[13]: Override RxVref runtime improvement scheme
				// 0 = runtime scheme with RxVref range set by VrefStart and Vref End
				// 1 = runtime speed scheme with range set by number of points before and after Si Friendly trained point
	uint8_t  reserved;	// Byte offset 0x0c, CSR Addr 0x58006, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Misc;	// Byte offset 0x0d, CSR Addr 0x58006, Direction=In
				// Lp4/5 specific options for training.
				//
				// Bit fields:
				//
				// Misc[0] Enable dfi_reset_n
				//
				// 0x0 = (Recommended) PHY internal registers control BP_MEMRESET_L pin until end of training.
				//  See PUB databook for requirement of dfi_reset_n control by MC before 1st dfi_init_start sequence.
				//
				// 0x1 = Enables dfi_reset_n to control BP_MEMRESET_L pin during training.
				//  To ensure that no glitches occur on BP_MEMRESET at the end of training,
				//  The MC must drive dfi_reset_n=1'b1 prior to starting training and keep its value until the end of training.
				//
				// Misc[1] unused
				// Misc[2] unused
				// Misc[3] Enable 4UI Si Friendly Scan
				// 0: 4UI scan
				// 1: 2UI scan
				// Misc[4] PRBS Read training seeding
				// 0: Use si friendly trained result
				// 1: Use RxReplica Estimate
				//  Misc[5] Pre Compute RxClk Coarse bit
				//  0: compute RxClk coarse bit after generating both sets of eyes
				//  1: estimate RxClk Coarse bit before RxClk training
				//  Misc[6] Single RxClk scan in SI Friendly Read
				//  0: Run both RxClkT and RxClkC scan
				//  1: Run only RxClkT scan
				//  Please note that Misc[6] should be set to 0 for datarate lower than 3200Mbps
				// Misc[7] RFU, must be zero
	int8_t   SIFriendlyDlyOffset;	// Byte offset 0x0e, CSR Addr 0x58007, Direction=In
				// SI Friendly Delay Offset
				// SIFriendlyDlyOffset[7:1]
				// This field can be used to modify the trained delay of an eye to be equal to an offset from the edge of that eye for the trained value of the voltage. This can be useful when performing SI friendly 2D training and encountering eye collapse in later training.
				// SIFriendlyDlyOffset[7:1] = 0  Disable this mechanism
				// SIFriendlyDlyOffset[7:1] > 0  Add offset to delay left edge of eye
				// SIFriendlyDlyOffset[7:1] < 0 Subtract offset from delay right edge of eye
				//
				// TruncV
				// SIFriendlyDlyOffset[0]
				//    0 = 2D Normal optimization. Treat any point outside of tested eye rectangle as failing.
				//    1 = If eye is truncated at low voltages treat points at voltages lower than the minimum tested voltage as passing. The trained point will always be at a voltage above the minimum tested voltage.
	uint8_t  CsTestFail;	// Byte offset 0x0f, CSR Addr 0x58007, Direction=Out
				// This field will be set if training fails on any rank.
				//    0x0 = No failures
				//    non-zero = one or more ranks failed training
	uint16_t SequenceCtrl;	// Byte offset 0x10, CSR Addr 0x58008, Direction=In
				// Controls the training steps to be run. Each bit corresponds to a training step.
				//
				// If the bit is set to 1, the training step will run.
				// If the bit is set to 0, the training step will be skipped.
				//
				// Training step to bit mapping:
				//    SequenceCtrl[0] = Run DevInit - Device/phy initialization. Should always be set.
				//    SequenceCtrl[1] = Run WrLvl - Write leveling
				//    SequenceCtrl[2] = Run RxEn - Read gate training
				//    SequenceCtrl[3] = Run RdDQS - read dqs training
				//    SequenceCtrl[4] = Run WrDq - write dq training
				//    SequenceCtrl[8-5] = RFU, must be zero
				//    SequenceCtrl[9] = Run MxRdLat - Max read latency training
				//    SequenceCtrl[11-10] = RFU, must be zero
				//    SequenceCtrl[12] = Run LPCA - CA Training
				//    SequenceCtrl[15-13] = RFU, must be zero
	uint8_t  HdtCtrl;	// Byte offset 0x12, CSR Addr 0x58009, Direction=In
				// To control the total number of debug messages, a verbosity subfield (HdtCtrl, Hardware Debug Trace Control) exists in the message block. Every message has a verbosity level associated with it, and as the HdtCtrl value is increased, less important s messages stop being sent through the mailboxes. The meanings of several major HdtCtrl thresholds are explained below:
				//
				//    0x05 = Detailed debug messages (e.g. Eye delays)
				//    0x0A = Coarse debug messages (e.g. rank information)
				//    0xC8 = Stage completion
				//    0xC9 = Assertion messages
				//    0xFF = Firmware completion messages only
				//
				// See Training App Note for more detailed information on what messages are included for each threshold.
				//
	uint8_t  Reserved13;	// Byte offset 0x13, CSR Addr 0x58009, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint16_t InternalStatus;	// Byte offset 0x14, CSR Addr 0x5800a, Direction=Out
				// RFU
	uint8_t  DFIMRLMargin;	// Byte offset 0x16, CSR Addr 0x5800b, Direction=In
				// Margin added to smallest passing trained DFI Max Read Latency value, in units of DFI clocks. Recommended to be >= 1. See the Training App Note for more details on the training process and the use of this value.
				//
				// This margin must include the maximum positive drift expected in tDQSCK over the target temperature and voltage range of the users system.
	uint8_t  TX2D_Delay_Weight;	// Byte offset 0x17, CSR Addr 0x5800b, Direction=In
				// [0-4] 0 ... 31
				// During TX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * TX_Delay_Weight2D + voltage margin * TX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  TX2D_Voltage_Weight;	// Byte offset 0x18, CSR Addr 0x5800c, Direction=In
				// [0-4] 0 ... 31
				// During TX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * TX_Delay_Weight2D + voltage margin * TX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  Quickboot;	// Byte offset 0x19, CSR Addr 0x5800c, Direction=In
				// Reserved
	uint8_t  Reserved1A;	// Byte offset 0x1a, CSR Addr 0x5800d, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  CATrainOpt;	// Byte offset 0x1b, CSR Addr 0x5800d, Direction=In
				// CA training option bit field
				// [0] CA VREF Training
				//        1 = Enable CA VREF Training
				//        0 = Disable CA VREF Training
				// [1-2] RFU must be zero
				// [3] Delayed clock feature
				//        0 = Use delayed clock
				//        1 = Use normal clock
				//  [4-7] Value by which ACTxDly is to be incremented during CA/CS training:
				//       If bit 7 is set, delay is incremented by 8,
				//       If bit 6 is set, delay is incremented by 4,
				//       if bit 5 is set, delay is incremented by 2
				//       else delay is incremented by 1
				//       This helps in reducing test run time during simulations. For silicon, it is recommended to increment delay by steps of 1 only
	uint8_t  X8Mode;	// Byte offset 0x1c, CSR Addr 0x5800e, Direction=In
				// X8Mode is encoded as a bit field for channel and rank.
				// Bit = 0 means x16 devices are connected.
				// Bit = 1 means 2 x8 devices are connected.
				// This field should be treated as if you have a 2 channel system. Valid Settings are 0xf/0xa/0x5/0x0.
				// X8Mode [0] - Channel A Rank 0
				// X8Mode [1] - Channel A Rank 1
				// X8Mode [2] - Channel B Rank 0
				// X8Mode [3] - Channel B Rank 1
				//
	uint8_t  RX2D_TrainOpt;	// Byte offset 0x1d, CSR Addr 0x5800e, Direction=In
				// RxClk Training Option
				// [0] Scan RxClkCDly during RxClkT training
				//        1 = Do not scan RxClkCDly during RxClkT training
				//        0 = Scan RxClkCDly during RxClkT training(Default)
				// [1-7] RFU must be zero
				// It is recommended to set RX2D_TrainOpt to 0
	uint8_t  TX2D_TrainOpt;	// Byte offset 0x1e, CSR Addr 0x5800f, Direction=In
				// RFU
	uint8_t  Reserved1F;	// Byte offset 0x1f, CSR Addr 0x5800f, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  RX2D_Delay_Weight;	// Byte offset 0x20, CSR Addr 0x58010, Direction=In
				// [0-4] 0 ... 31
				// During RX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * RX_Delay_Weight2D + voltage margin * RX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  RX2D_Voltage_Weight;	// Byte offset 0x21, CSR Addr 0x58010, Direction=In
				// [0-4] 0 ... 31
				// During RX 2D training when finding an eye center the delay and voltage components are weighed such that the combined margin is delay margin * RX_Delay_Weight2D + voltage margin * RX_Voltage_Weight2D. Either weight may be zero but if both are zero each weight is taken to have a value of one.
	uint8_t  Reserved22;	// Byte offset 0x22, CSR Addr 0x58011, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved23;	// Byte offset 0x23, CSR Addr 0x58011, Direction=
				// This field is reserved and must be programmed to 0x00.
	uint8_t  EnabledDQsChA;	// Byte offset 0x24, CSR Addr 0x58012, Direction=In
				// Total number of DQ bits enabled in PHY Channel A
	uint8_t  CsPresentChA;	// Byte offset 0x25, CSR Addr 0x58012, Direction=In
				// Indicates presence of DRAM at each chip select for PHY channel A.
				//
				//  0x1 = CS0 is populated with DRAM
				//  0x3 = CS0 and CS1 are populated with DRAM
				//
				// All other encodings are illegal
				//
	int8_t   CDD_ChA_RR_1_0;	// Byte offset 0x26, CSR Addr 0x58013, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RR_0_1;	// Byte offset 0x27, CSR Addr 0x58013, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_1_1;	// Byte offset 0x28, CSR Addr 0x58014, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_1_0;	// Byte offset 0x29, CSR Addr 0x58014, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_0_1;	// Byte offset 0x2a, CSR Addr 0x58015, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_RW_0_0;	// Byte offset 0x2b, CSR Addr 0x58015, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs0 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_1_1;	// Byte offset 0x2c, CSR Addr 0x58016, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_1_0;	// Byte offset 0x2d, CSR Addr 0x58016, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_0_1;	// Byte offset 0x2e, CSR Addr 0x58017, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WR_0_0;	// Byte offset 0x2f, CSR Addr 0x58017, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WW_1_0;	// Byte offset 0x30, CSR Addr 0x58018, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 1 to cs 0 on Channel A
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChA_WW_0_1;	// Byte offset 0x31, CSR Addr 0x58018, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 0 to cs 1 on Channel A
				// See PUB Databook for details on use of CDD values.
	uint8_t  MR1_A0;	// Byte offset 0x32, CSR Addr 0x58019, Direction=In
				// Value to be programmed in DRAM Mode Register 1 {Channel A, Rank 0}
	uint8_t  MR2_A0;	// Byte offset 0x33, CSR Addr 0x58019, Direction=In
				// Value to be programmed in DRAM Mode Register 2 {Channel A, Rank 0}
	uint8_t  MR3_A0;	// Byte offset 0x34, CSR Addr 0x5801a, Direction=In
				// Value to be programmed in DRAM Mode Register 3 {Channel A, Rank 0}
	uint8_t  MR4_A0;	// Byte offset 0x35, CSR Addr 0x5801a, Direction=In
				// Value to be programmed in DRAM Mode Register 4 {Channel A, Rank 0}
	uint8_t  MR11_A0;	// Byte offset 0x36, CSR Addr 0x5801b, Direction=In
				// Value to be programmed in DRAM Mode Register 11 {Channel A, Rank 0}
	uint8_t  MR12_A0;	// Byte offset 0x37, CSR Addr 0x5801b, Direction=In
				// Value to be programmed in DRAM Mode Register 12 {Channel A, Rank 0}
	uint8_t  MR13_A0;	// Byte offset 0x38, CSR Addr 0x5801c, Direction=In
				// Value to be programmed in DRAM Mode Register 13 {Channel A, Rank 0}
	uint8_t  MR14_A0;	// Byte offset 0x39, CSR Addr 0x5801c, Direction=In
				// Value to be programmed in DRAM Mode Register 14 {Channel A, Rank 0}
	uint8_t  MR16_A0;	// Byte offset 0x3a, CSR Addr 0x5801d, Direction=In
				// Value to be programmed in DRAM Mode Register 16 {Channel A, Rank 0}
	uint8_t  MR17_A0;	// Byte offset 0x3b, CSR Addr 0x5801d, Direction=In
				// Value to be programmed in DRAM Mode Register 17 {Channel A, Rank 0}
	uint8_t  MR22_A0;	// Byte offset 0x3c, CSR Addr 0x5801e, Direction=In
				// Value to be programmed in DRAM Mode Register 22 {Channel A, Rank 0}
	uint8_t  MR24_A0;	// Byte offset 0x3d, CSR Addr 0x5801e, Direction=In
				// Value to be programmed in DRAM Mode Register 24 {Channel A, Rank 0}
	uint8_t  MR1_A1;	// Byte offset 0x3e, CSR Addr 0x5801f, Direction=In
				// Value to be programmed in DRAM Mode Register 1 {Channel A, Rank 1}
	uint8_t  MR2_A1;	// Byte offset 0x3f, CSR Addr 0x5801f, Direction=In
				// Value to be programmed in DRAM Mode Register 2 {Channel A, Rank 1}
	uint8_t  MR3_A1;	// Byte offset 0x40, CSR Addr 0x58020, Direction=In
				// Value to be programmed in DRAM Mode Register 3 {Channel A, Rank 1}
	uint8_t  MR4_A1;	// Byte offset 0x41, CSR Addr 0x58020, Direction=In
				// Value to be programmed in DRAM Mode Register 4 {Channel A, Rank 1}
	uint8_t  MR11_A1;	// Byte offset 0x42, CSR Addr 0x58021, Direction=In
				// Value to be programmed in DRAM Mode Register 11 {Channel A, Rank 1}
	uint8_t  MR12_A1;	// Byte offset 0x43, CSR Addr 0x58021, Direction=In
				// Value to be programmed in DRAM Mode Register 12 {Channel A, Rank 1}
	uint8_t  MR13_A1;	// Byte offset 0x44, CSR Addr 0x58022, Direction=In
				// Value to be programmed in DRAM Mode Register 13 {Channel A, Rank 1}
	uint8_t  MR14_A1;	// Byte offset 0x45, CSR Addr 0x58022, Direction=In
				// Value to be programmed in DRAM Mode Register 14 {Channel A, Rank 1}
	uint8_t  MR16_A1;	// Byte offset 0x46, CSR Addr 0x58023, Direction=In
				// Value to be programmed in DRAM Mode Register 16 {Channel A, Rank 1}
	uint8_t  MR17_A1;	// Byte offset 0x47, CSR Addr 0x58023, Direction=In
				// Value to be programmed in DRAM Mode Register 17 {Channel A, Rank 1}
	uint8_t  MR22_A1;	// Byte offset 0x48, CSR Addr 0x58024, Direction=In
				// Value to be programmed in DRAM Mode Register 22 {Channel A, Rank 1}
	uint8_t  MR24_A1;	// Byte offset 0x49, CSR Addr 0x58024, Direction=In
				// Value to be programmed in DRAM Mode Register 24 {Channel A, Rank 1}
	uint8_t  CATerminatingRankChA;	// Byte offset 0x4a, CSR Addr 0x58025, Direction=In
				// Terminating Rank for CA bus on Channel A
				//    0x0 = Rank 0 is terminating rank
				//    0x1 = Rank 1 is terminating rank
	uint8_t  TrainedVREFCA_A0;	// Byte offset 0x4b, CSR Addr 0x58025, Direction=Out
				// Trained CA Vref setting for Ch A Rank 0
	uint8_t  TrainedVREFCA_A1;	// Byte offset 0x4c, CSR Addr 0x58026, Direction=Out
				// Trained CA Vref setting for Ch A Rank 1
	uint8_t  TrainedVREFDQ_A0;	// Byte offset 0x4d, CSR Addr 0x58026, Direction=Out
				// Trained DQ Vref setting for Ch A Rank 0
	uint8_t  TrainedVREFDQ_A1;	// Byte offset 0x4e, CSR Addr 0x58027, Direction=Out
				// Trained DQ Vref setting for Ch A Rank 1
	uint8_t  RxClkDly_Margin_A0;	// Byte offset 0x4f, CSR Addr 0x58027, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_A0;	// Byte offset 0x50, CSR Addr 0x58028, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_A0;	// Byte offset 0x51, CSR Addr 0x58028, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_A0;	// Byte offset 0x52, CSR Addr 0x58029, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  RxClkDly_Margin_A1;	// Byte offset 0x53, CSR Addr 0x58029, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_A1;	// Byte offset 0x54, CSR Addr 0x5802a, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_A1;	// Byte offset 0x55, CSR Addr 0x5802a, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_A1;	// Byte offset 0x56, CSR Addr 0x5802b, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  EnabledDQsChB;	// Byte offset 0x57, CSR Addr 0x5802b, Direction=In
				// Total number of DQ bits enabled in PHY Channel B
	uint8_t  CsPresentChB;	// Byte offset 0x58, CSR Addr 0x5802c, Direction=In
				// Indicates presence of DRAM at each chip select for PHY channel B.
				//
				//    0x0 = No chip selects are populated with DRAM
				//    0x1 = CS0 is populated with DRAM
				//    0x3 = CS0 and CS1 are populated with DRAM
				//
				// All other encodings are illegal
				//
	int8_t   CDD_ChB_RR_1_0;	// Byte offset 0x59, CSR Addr 0x5802c, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RR_0_1;	// Byte offset 0x5a, CSR Addr 0x5802d, Direction=Out
				// This is a signed integer value.
				// Read to read critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_1_1;	// Byte offset 0x5b, CSR Addr 0x5802d, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_1_0;	// Byte offset 0x5c, CSR Addr 0x5802e, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_0_1;	// Byte offset 0x5d, CSR Addr 0x5802e, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_RW_0_0;	// Byte offset 0x5e, CSR Addr 0x5802f, Direction=Out
				// This is a signed integer value.
				// Read to write critical delay difference from cs01 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_1_1;	// Byte offset 0x5f, CSR Addr 0x5802f, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_1_0;	// Byte offset 0x60, CSR Addr 0x58030, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_0_1;	// Byte offset 0x61, CSR Addr 0x58030, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WR_0_0;	// Byte offset 0x62, CSR Addr 0x58031, Direction=Out
				// This is a signed integer value.
				// Write  to read critical delay difference from cs 0 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WW_1_0;	// Byte offset 0x63, CSR Addr 0x58031, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 1 to cs 0 on Channel B
				// See PUB Databook for details on use of CDD values.
	int8_t   CDD_ChB_WW_0_1;	// Byte offset 0x64, CSR Addr 0x58032, Direction=Out
				// This is a signed integer value.
				// Write  to write critical delay difference from cs 0 to cs 1 on Channel B
				// See PUB Databook for details on use of CDD values.
	uint8_t  MR1_B0;	// Byte offset 0x65, CSR Addr 0x58032, Direction=In
				// Value to be programmed in DRAM Mode Register 1 {Channel B, Rank 0}
	uint8_t  MR2_B0;	// Byte offset 0x66, CSR Addr 0x58033, Direction=In
				// Value to be programmed in DRAM Mode Register 2 {Channel B, Rank 0}
	uint8_t  MR3_B0;	// Byte offset 0x67, CSR Addr 0x58033, Direction=In
				// Value to be programmed in DRAM Mode Register 3 {Channel B, Rank 0}
	uint8_t  MR4_B0;	// Byte offset 0x68, CSR Addr 0x58034, Direction=In
				// Value to be programmed in DRAM Mode Register 4 {Channel B, Rank 0}
	uint8_t  MR11_B0;	// Byte offset 0x69, CSR Addr 0x58034, Direction=In
				// Value to be programmed in DRAM Mode Register 11 {Channel B, Rank 0}
	uint8_t  MR12_B0;	// Byte offset 0x6a, CSR Addr 0x58035, Direction=In
				// Value to be programmed in DRAM Mode Register 12 {Channel B, Rank 0}
	uint8_t  MR13_B0;	// Byte offset 0x6b, CSR Addr 0x58035, Direction=In
				// Value to be programmed in DRAM Mode Register 13 {Channel B, Rank 0}
	uint8_t  MR14_B0;	// Byte offset 0x6c, CSR Addr 0x58036, Direction=In
				// Value to be programmed in DRAM Mode Register 14 {Channel B, Rank 0}
	uint8_t  MR16_B0;	// Byte offset 0x6d, CSR Addr 0x58036, Direction=In
				// Value to be programmed in DRAM Mode Register 16 {Channel B, Rank 0}
	uint8_t  MR17_B0;	// Byte offset 0x6e, CSR Addr 0x58037, Direction=In
				// Value to be programmed in DRAM Mode Register 17 {Channel B, Rank 0}
	uint8_t  MR22_B0;	// Byte offset 0x6f, CSR Addr 0x58037, Direction=In
				// Value to be programmed in DRAM Mode Register 22 {Channel B, Rank 0}
	uint8_t  MR24_B0;	// Byte offset 0x70, CSR Addr 0x58038, Direction=In
				// Value to be programmed in DRAM Mode Register 24 {Channel B, Rank 0}
	uint8_t  MR1_B1;	// Byte offset 0x71, CSR Addr 0x58038, Direction=In
				// Value to be programmed in DRAM Mode Register 1 {Channel B, Rank 1}
	uint8_t  MR2_B1;	// Byte offset 0x72, CSR Addr 0x58039, Direction=In
				// Value to be programmed in DRAM Mode Register 2 {Channel B, Rank 1}
	uint8_t  MR3_B1;	// Byte offset 0x73, CSR Addr 0x58039, Direction=In
				// Value to be programmed in DRAM Mode Register 3 {Channel B, Rank 1}
	uint8_t  MR4_B1;	// Byte offset 0x74, CSR Addr 0x5803a, Direction=In
				// Value to be programmed in DRAM Mode Register 4 {Channel B, Rank 1}
	uint8_t  MR11_B1;	// Byte offset 0x75, CSR Addr 0x5803a, Direction=In
				// Value to be programmed in DRAM Mode Register 11 {Channel B, Rank 1}
	uint8_t  MR12_B1;	// Byte offset 0x76, CSR Addr 0x5803b, Direction=In
				// Value to be programmed in DRAM Mode Register 12 {Channel B, Rank 1}
	uint8_t  MR13_B1;	// Byte offset 0x77, CSR Addr 0x5803b, Direction=In
				// Value to be programmed in DRAM Mode Register 13 {Channel B, Rank 1}
	uint8_t  MR14_B1;	// Byte offset 0x78, CSR Addr 0x5803c, Direction=In
				// Value to be programmed in DRAM Mode Register 14 {Channel B, Rank 1}
	uint8_t  MR16_B1;	// Byte offset 0x79, CSR Addr 0x5803c, Direction=In
				// Value to be programmed in DRAM Mode Register 16 {Channel B, Rank 1}
	uint8_t  MR17_B1;	// Byte offset 0x7a, CSR Addr 0x5803d, Direction=In
				// Value to be programmed in DRAM Mode Register 17 {Channel B, Rank 1}
	uint8_t  MR22_B1;	// Byte offset 0x7b, CSR Addr 0x5803d, Direction=In
				// Value to be programmed in DRAM Mode Register 22 {Channel B, Rank 1}
	uint8_t  MR24_B1;	// Byte offset 0x7c, CSR Addr 0x5803e, Direction=In
				// Value to be programmed in DRAM Mode Register 24 {Channel B, Rank 1}
	uint8_t  CATerminatingRankChB;	// Byte offset 0x7d, CSR Addr 0x5803e, Direction=In
				// Terminating Rank for CA bus on Channel B
				//    0x0 = Rank 0 is terminating rank
				//    0x1 = Rank 1 is terminating rank
	uint8_t  TrainedVREFCA_B0;	// Byte offset 0x7e, CSR Addr 0x5803f, Direction=Out
				// Trained CA Vref setting for Ch B Rank 0
	uint8_t  TrainedVREFCA_B1;	// Byte offset 0x7f, CSR Addr 0x5803f, Direction=Out
				// Trained CA Vref setting for Ch B Rank 1
	uint8_t  TrainedVREFDQ_B0;	// Byte offset 0x80, CSR Addr 0x58040, Direction=Out
				// Trained DQ Vref setting for Ch B Rank 0
	uint8_t  TrainedVREFDQ_B1;	// Byte offset 0x81, CSR Addr 0x58040, Direction=Out
				// Trained DQ Vref setting for Ch B Rank 1
	uint8_t  RxClkDly_Margin_B0;	// Byte offset 0x82, CSR Addr 0x58041, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_B0;	// Byte offset 0x83, CSR Addr 0x58041, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_B0;	// Byte offset 0x84, CSR Addr 0x58042, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_B0;	// Byte offset 0x85, CSR Addr 0x58042, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  RxClkDly_Margin_B1;	// Byte offset 0x86, CSR Addr 0x58043, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  VrefDac_Margin_B1;	// Byte offset 0x87, CSR Addr 0x58043, Direction=Out
				// Distance from the trained center to the closest failing region in phy DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  TxDqDly_Margin_B1;	// Byte offset 0x88, CSR Addr 0x58044, Direction=Out
				// Distance from the trained center to the closest failing region in DLL steps. This value is the minimum of all eyes in this timing group.
	uint8_t  DeviceVref_Margin_B1;	// Byte offset 0x89, CSR Addr 0x58044, Direction=Out
				// Distance from the trained center to the closest failing region in device DAC steps. This value is the minimum of all eyes in this timing group.
	uint8_t  MR21_A0;	// Byte offset 0x8a, CSR Addr 0x58045, Direction=In
				// Value to be programmed in DRAM Mode Register 21 {Channel A, Rank 0}
	uint8_t  MR51_A0;	// Byte offset 0x8b, CSR Addr 0x58045, Direction=In
				// Value to be programmed in DRAM Mode Register 51 {Channel A, Rank 0}
	uint8_t  MR21_A1;	// Byte offset 0x8c, CSR Addr 0x58046, Direction=In
				// Value to be programmed in DRAM Mode Register 21 {Channel A, Rank 1}
	uint8_t  MR51_A1;	// Byte offset 0x8d, CSR Addr 0x58046, Direction=In
				// Value to be programmed in DRAM Mode Register 51 {Channel A, Rank 1}
	uint8_t  MR21_B0;	// Byte offset 0x8e, CSR Addr 0x58047, Direction=In
				// Value to be programmed in DRAM Mode Register 21 {Channel B, Rank 0}
	uint8_t  MR51_B0;	// Byte offset 0x8f, CSR Addr 0x58047, Direction=In
				// Value to be programmed in DRAM Mode Register 51 {Channel B, Rank 0}
	uint8_t  MR21_B1;	// Byte offset 0x90, CSR Addr 0x58048, Direction=In
				// Value to be programmed in DRAM Mode Register 21 {Channel B, Rank 1}
	uint8_t  MR51_B1;	// Byte offset 0x91, CSR Addr 0x58048, Direction=In
				// Value to be programmed in DRAM Mode Register 51 {Channel B. Rank 1}
	uint8_t  LP4XMode;	// Byte offset 0x92, CSR Addr 0x58049, Direction=In
				// Must be Set if DRAM supports LP4X
	uint8_t  Disable2D;	// Byte offset 0x93, CSR Addr 0x58049, Direction=In
				// Set to disable 2D training
				// When this field is set to 1, it is not recommended to set RxDfeMode to DFE enabled with 1 previous bit lookup
	uint8_t  VrefSamples;	// Byte offset 0x94, CSR Addr 0x5804a, Direction=In
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ALT_RL;	// Byte offset 0x95, CSR Addr 0x5804a, Direction=In
				// This is the alternate Read Latency for DBI off
	uint8_t  MAIN_RL;	// Byte offset 0x96, CSR Addr 0x5804b, Direction=In
				// This is the main RL calculated by phyinit
	uint8_t  RdWrPatternA;	// Byte offset 0x97, CSR Addr 0x5804b, Direction=In
				// Lower-byte read pattern for training
				// When RdWrPatternA, PdWrPatternB and RdWrInvert are all set to 0 uses default patterns
				// When set RxDfeMode to 0x4 and set Disable2D to 0, it is recommended to set this filed to 0
	uint8_t  RdWrPatternB;	// Byte offset 0x98, CSR Addr 0x5804c, Direction=In
				// Upper-byte read pattern for training
				// When RdWrPatternA, PdWrPatternB and RdWrInvert are all set to 0 uses default patterns
				// When set RxDfeMode to 0x4 and set Disable2D to 0, it is recommended to set this filed to 0
	uint8_t  RdWrInvert;	// Byte offset 0x99, CSR Addr 0x5804c, Direction=In
				// Per-byte per bit  invert for read pattern for training
				// Must be used together with RdWrPatternA/RdWrPatternB
				// When set RxDfeMode to 0x4 and set Disable2D to 0, it is recommended to set this filed to 0
	uint8_t  LdffMode;	// Byte offset 0x9a, CSR Addr 0x5804d, Direction=In
				// In LDFF mode raw PATN/PRBS sequences driven on DBI & EDC  lanes. If this is set to 0 pattern follows MR settings
				// [0] = 1 Force DBI like patterns on all lanes
				// [1] = 1 Force non DBI patterns on all lanes
	uint8_t  Reserved9B;	// Byte offset 0x9b, CSR Addr 0x5804d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint16_t FCDfi0AcsmStart;	// Byte offset 0x9c, CSR Addr 0x5804e, Direction=In
				// Start Address for MRW commands for DFI0
	uint16_t FCDfi1AcsmStart;	// Byte offset 0x9e, CSR Addr 0x5804f, Direction=In
				// Start Address for MRW commands for DFI1
	uint16_t FCDfi0AcsmStartPSY;	// Byte offset 0xa0, CSR Addr 0x58050, Direction=In
				// Start Address for MRW commands for DFI0 for the previous PState
	uint16_t FCDfi1AcsmStartPSY;	// Byte offset 0xa2, CSR Addr 0x58051, Direction=In
				// Start Address for MRW commands for DFI1 for the previous PState
	uint16_t FCDMAStartMR;	// Byte offset 0xa4, CSR Addr 0x58052, Direction=In
				// Start DMA Address for FCDfi0AcsmStart
	uint16_t FCDMAStartCsr;	// Byte offset 0xa6, CSR Addr 0x58053, Direction=In
				// Start DMA Address for Starting CSR
	uint8_t  EnCustomSettings;	// Byte offset 0xa8, CSR Addr 0x58054, Direction=In
				// Enable Custome TxSlew and TxImpedance Settings
				//
				// When this field is set to 1, the following LS_ values shall be used in the corresponding AC CSRs during low speed operations.
				// The values are programmed as it is in the CSRs by the firmware, so these should be set very carefully
				//
	uint8_t  LS_TxSlewSE0;	// Byte offset 0xa9, CSR Addr 0x58054, Direction=In
				// Custom Low Speed AC TxSlew for SE0
	uint8_t  LS_TxSlewSE1;	// Byte offset 0xaa, CSR Addr 0x58055, Direction=In
				// Custom Low Speed AC TxSlew for SE1
	uint8_t  LS_TxSlewDIFF0;	// Byte offset 0xab, CSR Addr 0x58055, Direction=In
				// Custom Low Speed AC TxSlew for SE1
	uint16_t LS_TxImpedanceDIFF0T;	// Byte offset 0xac, CSR Addr 0x58056, Direction=In
				// Custom Low Speed AC TxImpedance for DIFF0T
	uint16_t LS_TxImpedanceDIFF0C;	// Byte offset 0xae, CSR Addr 0x58057, Direction=In
				// Custom Low Speed AC TxImpedance for DIFF0C
	uint16_t LS_TxImpedanceSE0;	// Byte offset 0xb0, CSR Addr 0x58058, Direction=In
				// Custom Low Speed AC TxImpedance for SE0
	uint16_t LS_TxImpedanceSE1;	// Byte offset 0xb2, CSR Addr 0x58059, Direction=In
				// Custom Low Speed AC TxImpedance for SE1
	uint8_t  VrefInc;	// Byte offset 0xb4, CSR Addr 0x5805a, Direction=In
				// This field should be programmed to 1
				// This controls the vrefIncrement size for 2D training
	uint8_t  WrLvlTrainOpt;	// Byte offset 0xb5, CSR Addr 0x5805a, Direction=In
				// LP4 Write leveling training options. Currently not in use
	uint16_t FCDCCMStartCSR;	// Byte offset 0xb6, CSR Addr 0x5805b, Direction=out
				// Start Address in DCCM for CSRs to be copied to DMA
	uint16_t FCDCCMLenCSR;	// Byte offset 0xb8, CSR Addr 0x5805c, Direction=out
				// number of entries written into DCCM for CSRs
	uint16_t FCDCCMStartMR;	// Byte offset 0xba, CSR Addr 0x5805d, Direction=out
				// Start Address in DCCM for Mrs to be copied to DMA
	uint16_t  FCDCCMLenMR;	// Byte offset 0xbc, CSR Addr 0x5805e, Direction=out
				// number of entries written into DCCM for Mrs
	uint8_t  ReservedBE;	// Byte offset 0xbe, CSR Addr 0x5805f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedBF;	// Byte offset 0xbf, CSR Addr 0x5805f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC0;	// Byte offset 0xc0, CSR Addr 0x58060, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC1;	// Byte offset 0xc1, CSR Addr 0x58060, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC2;	// Byte offset 0xc2, CSR Addr 0x58061, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC3;	// Byte offset 0xc3, CSR Addr 0x58061, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC4;	// Byte offset 0xc4, CSR Addr 0x58062, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC5;	// Byte offset 0xc5, CSR Addr 0x58062, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC6;	// Byte offset 0xc6, CSR Addr 0x58063, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedC7;	// Byte offset 0xc7, CSR Addr 0x58063, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  RxVrefStartPatDfe0;	// Byte offset 0xc8, CSR Addr 0x58064, Direction=In
				// Starting VREF Value for Rx Training for DFE0 for Pattern Mode
	uint8_t  RxVrefStartPatDfe1;	// Byte offset 0xc9, CSR Addr 0x58064, Direction=In
				// Starting VREF Value for Rx Training for DFE1 for Pattern Mode
	uint8_t  RxVrefStartPrbsDfe0;	// Byte offset 0xca, CSR Addr 0x58065, Direction=In
				// Train2Dmisc[13]= 0, Starting VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1, number of points to scan before the si friendly trained vref
	uint8_t  RxVrefStartPrbsDfe1;	// Byte offset 0xcb, CSR Addr 0x58065, Direction=In
				// Train2Dmisc[13]= 0, Starting VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1, number of points to scan before the si friendly trained vref
	uint8_t  TxVrefStart;	// Byte offset 0xcc, CSR Addr 0x58066, Direction=In
				// Starting VREF Value for Tx Training for Prbs Mode
	uint8_t  RxVrefEndPatDfe0;	// Byte offset 0xcd, CSR Addr 0x58066, Direction=In
				// Ending VREF Value for Rx Training for DFE0 for Pattern Mode
	uint8_t  RxVrefEndPatDfe1;	// Byte offset 0xce, CSR Addr 0x58067, Direction=In
				// Ending VREF Value for Rx Training for DFE1 for Pattern Mode
	uint8_t  RxVrefEndPrbsDfe0;	// Byte offset 0xcf, CSR Addr 0x58067, Direction=In
				// Train2Dmisc[13]= 0,Ending VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1,Number of points to scan after the si friendly trained vref
	uint8_t  RxVrefEndPrbsDfe1;	// Byte offset 0xd0, CSR Addr 0x58068, Direction=In
				// Train2Dmisc[13]= 0,Ending VREF Value for Rx Training for DFE0 for Prbs Mode
				// Train2Dmisc[13]= 1,Number of points to scan after the si friendly trained vref
	uint8_t  TxVrefEnd;	// Byte offset 0xd1, CSR Addr 0x58068, Direction=In
				// Ending VREF Value for Tx Training for Prbs Mode
	uint8_t  RxVrefStepPatDfe0;	// Byte offset 0xd2, CSR Addr 0x58069, Direction=In
				// VREF Step Value for Rx Training for DFE0 for Pattern Mode
	uint8_t  RxVrefStepPatDfe1;	// Byte offset 0xd3, CSR Addr 0x58069, Direction=In
				// VREF Step Value for Rx Training for DFE1 for Pattern Mode
	uint8_t  RxVrefStepPrbsDfe0;	// Byte offset 0xd4, CSR Addr 0x5806a, Direction=In
				// VREF Step Value for Rx Training for DFE0 for Prbs Mode
	uint8_t  RxVrefStepPrbsDfe1;	// Byte offset 0xd5, CSR Addr 0x5806a, Direction=In
				// VREF Step Value for Rx Training for DFE0 for Prbs Mode
	uint8_t  TxVrefStep;	// Byte offset 0xd6, CSR Addr 0x5806b, Direction=In
				// VREF Step Value for Tx Training for Prbs Mode
	uint8_t  UpperLowerByte;	// Byte offset 0xd7, CSR Addr 0x5806b, Direction=In
				// UpperLowerByte[3:0] - A value of 0 means partner bytes are not swapped. A value of 1 means partner bytes are swapped.
				// [0] : Channel A Rank 0
				// [1] : Channel A Rank 1
				// [2] : Channel B Rank 0
				// [3] : Channel B Rank1
	uint8_t  MRLCalcAdj;	// Byte offset 0xd8, CSR Addr 0x5806c, Direction=In
				// This field is treated as an int_8 and is added to the intermediate MRL values used in training.
	uint8_t  PPT2OffsetMargin;	// Byte offset 0xd9, CSR Addr 0x5806c, Direction=In
				// When set to 0 disabled, non zero values add that much margin to left and right eye offsets to prevent underflow or overflow.
	uint8_t  ReservedDA;	// Byte offset 0xda, CSR Addr 0x5806d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedDB;	// Byte offset 0xdb, CSR Addr 0x5806d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedDC;	// Byte offset 0xdc, CSR Addr 0x5806e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedDD;	// Byte offset 0xdd, CSR Addr 0x5806e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedDE;	// Byte offset 0xde, CSR Addr 0x5806f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedDF;	// Byte offset 0xdf, CSR Addr 0x5806f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE0;	// Byte offset 0xe0, CSR Addr 0x58070, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE1;	// Byte offset 0xe1, CSR Addr 0x58070, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE2;	// Byte offset 0xe2, CSR Addr 0x58071, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE3;	// Byte offset 0xe3, CSR Addr 0x58071, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE4;	// Byte offset 0xe4, CSR Addr 0x58072, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE5;	// Byte offset 0xe5, CSR Addr 0x58072, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE6;	// Byte offset 0xe6, CSR Addr 0x58073, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE7;	// Byte offset 0xe7, CSR Addr 0x58073, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE8;	// Byte offset 0xe8, CSR Addr 0x58074, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedE9;	// Byte offset 0xe9, CSR Addr 0x58074, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedEA;	// Byte offset 0xea, CSR Addr 0x58075, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedEB;	// Byte offset 0xeb, CSR Addr 0x58075, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedEC;	// Byte offset 0xec, CSR Addr 0x58076, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedED;	// Byte offset 0xed, CSR Addr 0x58076, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedEE;	// Byte offset 0xee, CSR Addr 0x58077, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedEF;	// Byte offset 0xef, CSR Addr 0x58077, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF0;	// Byte offset 0xf0, CSR Addr 0x58078, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF1;	// Byte offset 0xf1, CSR Addr 0x58078, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF2;	// Byte offset 0xf2, CSR Addr 0x58079, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF3;	// Byte offset 0xf3, CSR Addr 0x58079, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF4;	// Byte offset 0xf4, CSR Addr 0x5807a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF5;	// Byte offset 0xf5, CSR Addr 0x5807a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF6;	// Byte offset 0xf6, CSR Addr 0x5807b, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF7;	// Byte offset 0xf7, CSR Addr 0x5807b, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF8;	// Byte offset 0xf8, CSR Addr 0x5807c, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedF9;	// Byte offset 0xf9, CSR Addr 0x5807c, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedFA;	// Byte offset 0xfa, CSR Addr 0x5807d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedFB;	// Byte offset 0xfb, CSR Addr 0x5807d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedFC;	// Byte offset 0xfc, CSR Addr 0x5807e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedFD;	// Byte offset 0xfd, CSR Addr 0x5807e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedFE;	// Byte offset 0xfe, CSR Addr 0x5807f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  ReservedFF;	// Byte offset 0xff, CSR Addr 0x5807f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved100;	// Byte offset 0x100, CSR Addr 0x58080, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved101;	// Byte offset 0x101, CSR Addr 0x58080, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved102;	// Byte offset 0x102, CSR Addr 0x58081, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved103;	// Byte offset 0x103, CSR Addr 0x58081, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved104;	// Byte offset 0x104, CSR Addr 0x58082, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved105;	// Byte offset 0x105, CSR Addr 0x58082, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved106;	// Byte offset 0x106, CSR Addr 0x58083, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved107;	// Byte offset 0x107, CSR Addr 0x58083, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved108;	// Byte offset 0x108, CSR Addr 0x58084, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved109;	// Byte offset 0x109, CSR Addr 0x58084, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved10A;	// Byte offset 0x10a, CSR Addr 0x58085, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved10B;	// Byte offset 0x10b, CSR Addr 0x58085, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved10C;	// Byte offset 0x10c, CSR Addr 0x58086, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved10D;	// Byte offset 0x10d, CSR Addr 0x58086, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved10E;	// Byte offset 0x10e, CSR Addr 0x58087, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved10F;	// Byte offset 0x10f, CSR Addr 0x58087, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved110;	// Byte offset 0x110, CSR Addr 0x58088, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved111;	// Byte offset 0x111, CSR Addr 0x58088, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved112;	// Byte offset 0x112, CSR Addr 0x58089, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved113;	// Byte offset 0x113, CSR Addr 0x58089, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved114;	// Byte offset 0x114, CSR Addr 0x5808a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved115;	// Byte offset 0x115, CSR Addr 0x5808a, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved116;	// Byte offset 0x116, CSR Addr 0x5808b, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved117;	// Byte offset 0x117, CSR Addr 0x5808b, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved118;	// Byte offset 0x118, CSR Addr 0x5808c, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved119;	// Byte offset 0x119, CSR Addr 0x5808c, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved11A;	// Byte offset 0x11a, CSR Addr 0x5808d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved11B;	// Byte offset 0x11b, CSR Addr 0x5808d, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved11C;	// Byte offset 0x11c, CSR Addr 0x5808e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved11D;	// Byte offset 0x11d, CSR Addr 0x5808e, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved11E;	// Byte offset 0x11e, CSR Addr 0x5808f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved11F;	// Byte offset 0x11f, CSR Addr 0x5808f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved120;	// Byte offset 0x120, CSR Addr 0x58090, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved121;	// Byte offset 0x121, CSR Addr 0x58090, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved122;	// Byte offset 0x122, CSR Addr 0x58091, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved123;	// Byte offset 0x123, CSR Addr 0x58091, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved124;	// Byte offset 0x124, CSR Addr 0x58092, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved125;	// Byte offset 0x125, CSR Addr 0x58092, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved126;	// Byte offset 0x126, CSR Addr 0x58093, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  RxDlyScanShiftRank0Byte0;	// Byte offset 0x127, CSR Addr 0x58093, Direction=In
				// Rx delay scan shift for Rank0 Dbyte0.  Examples:
				//   0x1 - start scan 1 step earlier
				//   0xf - start scan 15 steps earlier
				//   0xff - start scan 1 step later
				//   0xfe - start scan 2 steps later
	uint8_t  RxDlyScanShiftRank0Byte1;	// Byte offset 0x128, CSR Addr 0x58094, Direction=In
				// Rx delay scan shift for Rank0 Dbyte1
	uint8_t  RxDlyScanShiftRank0Byte2;	// Byte offset 0x129, CSR Addr 0x58094, Direction=In
				// Rx delay scan shift for Rank0 Dbyte2
	uint8_t  RxDlyScanShiftRank0Byte3;	// Byte offset 0x12a, CSR Addr 0x58095, Direction=In
				// Rx delay scan shift for Rank0 Dbyte3
	uint8_t  RxDlyScanShiftRank1Byte0;	// Byte offset 0x12b, CSR Addr 0x58095, Direction=In
				// Rx delay scan shift for Rank1 Dbyte0
	uint8_t  RxDlyScanShiftRank1Byte1;	// Byte offset 0x12c, CSR Addr 0x58096, Direction=In
				// Rx delay scan shift for Rank1 Dbyte1
	uint8_t  RxDlyScanShiftRank1Byte2;	// Byte offset 0x12d, CSR Addr 0x58096, Direction=In
				// Rx delay scan shift for Rank1 Dbyte2
	uint8_t  RxDlyScanShiftRank1Byte3;	// Byte offset 0x12e, CSR Addr 0x58097, Direction=In
				// Rx delay scan shift for Rank1 Dbyte3
	uint8_t  Reserved12F;	// Byte offset 0x12f, CSR Addr 0x58097, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint16_t QBPllUPllProg0;	// Byte offset 0x130, CSR Addr 0x58098, Direction=out
				// CSR PLLIPProg0 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllUPllProg1;	// Byte offset 0x132, CSR Addr 0x58099, Direction=out
				// CSR PLLIPProg1 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllUPllProg2;	// Byte offset 0x134, CSR Addr 0x5809a, Direction=out
				// CSR PLLIPProg2 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllUPllProg3;	// Byte offset 0x136, CSR Addr 0x5809b, Direction=out
				// CSR PLLIPProg3 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllCtrl1;	// Byte offset 0x138, CSR Addr 0x5809c, Direction=out
				// CSR PllCtrl1 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllCtrl4;	// Byte offset 0x13a, CSR Addr 0x5809d, Direction=out
				// CSR PllCtrl4 value for normal mode, reserved for QuickBoot firmware.
	uint16_t QBPllCtrl5;	// Byte offset 0x13c, CSR Addr 0x5809e, Direction=out
				// CSR PllCtrl5 value for normal mode, reserved for QuickBoot firmware.
	uint8_t  Reserved13E;	// Byte offset 0x13e, CSR Addr 0x5809f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
	uint8_t  Reserved13F;	// Byte offset 0x13f, CSR Addr 0x5809f, Direction=N/A
				// This field is reserved and must be programmed to 0x00.
} __attribute__ ((packed)) PMU_SMB_LPDDR4X_1D_t;

#endif
