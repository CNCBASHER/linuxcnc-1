//
//    Copyright (C) 2011 Sascha Ittner <sascha.ittner@modusoft.de>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//
#ifndef _EMCEC_CONF_H_
#define _EMCEC_H_

#define EMCEC_MODULE_NAME "emcec"

#define EMCEC_CONF_SHMEM_KEY   0xACB572C7
#define EMCEC_CONF_SHMEM_MAGIC 0x036ED5A3

#define EMCEC_CONF_STR_MAXLEN 32

typedef enum {
  emcecConfTypeNone,
  emcecConfTypeMaster,
  emcecConfTypeSlave,
  emcecConfTypeDcConf,
  emcecConfTypeWatchdog
} EMCEC_CONF_TYPE_T;

typedef enum {
  emcecSlaveTypeInvalid,
  emcecSlaveTypeEK1100,
  emcecSlaveTypeEL1002,
  emcecSlaveTypeEL1004,
  emcecSlaveTypeEL1008,
  emcecSlaveTypeEL1012,
  emcecSlaveTypeEL1014,
  emcecSlaveTypeEL1018,
  emcecSlaveTypeEL1024,
  emcecSlaveTypeEL1034,
  emcecSlaveTypeEL1084,
  emcecSlaveTypeEL1088,
  emcecSlaveTypeEL1094,
  emcecSlaveTypeEL1098,
  emcecSlaveTypeEL1104,
  emcecSlaveTypeEL1114,
  emcecSlaveTypeEL1124,
  emcecSlaveTypeEL1134,
  emcecSlaveTypeEL1144,
  emcecSlaveTypeEL1808,
  emcecSlaveTypeEL2002,
  emcecSlaveTypeEL2004,
  emcecSlaveTypeEL2008,
  emcecSlaveTypeEL2022,
  emcecSlaveTypeEL2024,
  emcecSlaveTypeEL2032,
  emcecSlaveTypeEL2034,
  emcecSlaveTypeEL2042,
  emcecSlaveTypeEL2084,
  emcecSlaveTypeEL2088,
  emcecSlaveTypeEL2124,
  emcecSlaveTypeEL2808,
  emcecSlaveTypeEL3102,
  emcecSlaveTypeEL3112,
  emcecSlaveTypeEL3122,
  emcecSlaveTypeEL3142,
  emcecSlaveTypeEL3152,
  emcecSlaveTypeEL3162,
  emcecSlaveTypeEL4102,
  emcecSlaveTypeEL4112,
  emcecSlaveTypeEL4122,
  emcecSlaveTypeEL4132,
  emcecSlaveTypeEL5152,
  emcecSlaveTypeEL2521,
  emcecSlaveTypeStMDS5k
} EMCEC_SLAVE_TYPE_T;

typedef struct {
  uint32_t magic;
  size_t length;
} EMCEC_CONF_HEADER_T;

typedef struct {
  EMCEC_CONF_TYPE_T confType;
  int index;
  uint32_t appTimePeriod;
  int refClockSyncCycles;
  char name[EMCEC_CONF_STR_MAXLEN];
} EMCEC_CONF_MASTER_T;

typedef struct {
  EMCEC_CONF_TYPE_T confType;
  int index;
  EMCEC_SLAVE_TYPE_T type;
  char name[EMCEC_CONF_STR_MAXLEN];
} EMCEC_CONF_SLAVE_T;

typedef struct {
  EMCEC_CONF_TYPE_T confType;
  uint16_t assignActivate;
  uint32_t sync0Cycle;
  uint32_t sync0Shift;
  uint32_t sync1Cycle;
  uint32_t sync1Shift;
} EMCEC_CONF_DC_T;

typedef struct {
  EMCEC_CONF_TYPE_T confType;
  uint16_t divider;
  uint16_t intervals;
} EMCEC_CONF_WATCHDOG_T;

typedef struct {
  EMCEC_CONF_TYPE_T confType;
} EMCEC_CONF_NULL_T;

#endif
