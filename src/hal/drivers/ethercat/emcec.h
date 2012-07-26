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
#ifndef _EMCEC_H_
#define _EMCEC_H_

#include <linux/ctype.h>

#include <rtai_sem.h>

#include "ecrt.h"
#include "emcec_conf.h"

// list macros
#define EMCEC_LIST_APPEND(first, last, item) \
do {                             \
  (item)->prev = (last);         \
  if ((item)->prev != NULL) {    \
    (item)->prev->next = (item); \
  } else {                       \
    (first) = (item);            \
  }                              \
  (last) = (item);               \
} while (0);                     \

// pdo macros
#define EMCEC_PDO_INIT(pdo, pos, vid, pid, idx, sidx, off, bpos) \
do {                        \
  pdo->position = pos;      \
  pdo->vendor_id = vid;     \
  pdo->product_code = pid;  \
  pdo->index = idx;         \
  pdo->subindex = sidx;     \
  pdo->offset = off;        \
  pdo->bit_position = bpos; \
  pdo++;                    \
} while (0);                \

#define EMCEC_MSG_PFX "EMCEC: "

// vendor ids
#define EMCEC_BECKHOFF_VID 0x00000002
#define EMCEC_STOEBER_VID  0x000000b9

// SDO request timeout (ms)
#define EMCEC_SDO_REQ_TIMEOUT 500

struct emcec_master;
struct emcec_slave;

typedef int (*emcec_slave_init_t) (int comp_id, struct emcec_slave *slave, ec_pdo_entry_reg_t *pdo_entry_regs);
typedef void (*emcec_slave_cleanup_t) (struct emcec_slave *slave);
typedef void (*emcec_slave_rw_t) (struct emcec_slave *slave, long period);

typedef struct emcec_master_data {
  hal_u32_t *slaves_responding;
  hal_bit_t *state_init;
  hal_bit_t *state_preop;
  hal_bit_t *state_safeop;
  hal_bit_t *state_op;
  hal_bit_t *link_up;
} emcec_master_data_t;

typedef struct emcec_slave_state {
  hal_bit_t *online;
  hal_bit_t *operational;
  hal_bit_t *state_init;
  hal_bit_t *state_preop;
  hal_bit_t *state_safeop;
  hal_bit_t *state_op;
} emcec_slave_state_t;

typedef struct emcec_master {
  struct emcec_master *prev;
  struct emcec_master *next;
  int index;
  char name[EMCEC_CONF_STR_MAXLEN];
  ec_master_t *master;
  SEM semaphore;
  int pdo_entry_count;
  ec_pdo_entry_reg_t *pdo_entry_regs;
  ec_domain_t *domain;
  uint8_t *process_data;
  int process_data_len;
  struct emcec_slave *first_slave;
  struct emcec_slave *last_slave;
  emcec_master_data_t *hal_data;
  uint64_t app_time;
  uint32_t app_time_period;
  int sync_ref_cnt;
  int sync_ref_cycles;
} emcec_master_t;

typedef struct {
  uint16_t assignActivate;
  uint32_t sync0Cycle;
  uint32_t sync0Shift;
  uint32_t sync1Cycle;
  uint32_t sync1Shift;
} emcec_slave_dc_t;

typedef struct {
  uint16_t divider;
  uint16_t intervals;
} emcec_slave_watchdog_t;

typedef struct emcec_slave {
  struct emcec_slave *prev;
  struct emcec_slave *next;
  struct emcec_master *master;
  int index;
  char name[EMCEC_CONF_STR_MAXLEN];
  uint32_t vid;
  uint32_t pid;
  int pdo_entry_count;
  ec_sync_info_t *sync_info;
  ec_slave_config_t *config;
  ec_slave_config_state_t state;
  emcec_slave_dc_t *dc_conf;
  emcec_slave_watchdog_t *wd_conf;
  emcec_slave_init_t proc_init;
  emcec_slave_cleanup_t proc_cleanup;
  emcec_slave_rw_t proc_read;
  emcec_slave_rw_t proc_write;
  emcec_slave_state_t *hal_state_data;
  void *hal_data;
} emcec_slave_t;

ec_sdo_request_t *emcec_read_sdo(struct emcec_slave *slave, uint16_t index, uint8_t subindex, size_t size);

#endif

