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



#include "rtapi.h"
#include "rtapi_ctype.h"
#include "rtapi_app.h"
#include "rtapi_string.h"

#include "hal.h"

#include "emcec.h"
#include "emcec_ek1100.h"
#include "emcec_el1xxx.h"
#include "emcec_el2xxx.h"
#include "emcec_el31x2.h"
#include "emcec_el41x2.h"
#include "emcec_el5152.h"
#include "emcec_el2521.h"
#include "emcec_stmds5k.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sascha Ittner <sascha.ittner@modusoft.de>");
MODULE_DESCRIPTION("Driver for EtherCAT devices");

typedef struct emcec_typelist {
  EMCEC_SLAVE_TYPE_T type;
  uint32_t vid;
  uint32_t pid;
  int pdo_entry_count;
  emcec_slave_init_t proc_init;
} emcec_typelist_t;

static const emcec_typelist_t types[] = {
  // bus coupler
  { emcecSlaveTypeEK1100, EMCEC_EK1100_VID, EMCEC_EK1100_PID, EMCEC_EK1100_PDOS, NULL},

  // digital in
  { emcecSlaveTypeEL1002, EMCEC_EL1xxx_VID, EMCEC_EL1002_PID, EMCEC_EL1002_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1004, EMCEC_EL1xxx_VID, EMCEC_EL1004_PID, EMCEC_EL1004_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1008, EMCEC_EL1xxx_VID, EMCEC_EL1008_PID, EMCEC_EL1008_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1012, EMCEC_EL1xxx_VID, EMCEC_EL1012_PID, EMCEC_EL1012_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1014, EMCEC_EL1xxx_VID, EMCEC_EL1014_PID, EMCEC_EL1014_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1018, EMCEC_EL1xxx_VID, EMCEC_EL1018_PID, EMCEC_EL1018_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1024, EMCEC_EL1xxx_VID, EMCEC_EL1024_PID, EMCEC_EL1024_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1034, EMCEC_EL1xxx_VID, EMCEC_EL1034_PID, EMCEC_EL1034_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1084, EMCEC_EL1xxx_VID, EMCEC_EL1084_PID, EMCEC_EL1084_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1088, EMCEC_EL1xxx_VID, EMCEC_EL1088_PID, EMCEC_EL1088_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1094, EMCEC_EL1xxx_VID, EMCEC_EL1094_PID, EMCEC_EL1094_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1098, EMCEC_EL1xxx_VID, EMCEC_EL1098_PID, EMCEC_EL1098_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1104, EMCEC_EL1xxx_VID, EMCEC_EL1104_PID, EMCEC_EL1104_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1114, EMCEC_EL1xxx_VID, EMCEC_EL1114_PID, EMCEC_EL1114_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1124, EMCEC_EL1xxx_VID, EMCEC_EL1124_PID, EMCEC_EL1124_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1134, EMCEC_EL1xxx_VID, EMCEC_EL1134_PID, EMCEC_EL1134_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1144, EMCEC_EL1xxx_VID, EMCEC_EL1144_PID, EMCEC_EL1144_PDOS, emcec_el1xxx_init},
  { emcecSlaveTypeEL1808, EMCEC_EL1xxx_VID, EMCEC_EL1808_PID, EMCEC_EL1808_PDOS, emcec_el1xxx_init},

  // digital out
  { emcecSlaveTypeEL2002, EMCEC_EL2xxx_VID, EMCEC_EL2002_PID, EMCEC_EL2002_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2004, EMCEC_EL2xxx_VID, EMCEC_EL2004_PID, EMCEC_EL2004_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2008, EMCEC_EL2xxx_VID, EMCEC_EL2008_PID, EMCEC_EL2008_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2022, EMCEC_EL2xxx_VID, EMCEC_EL2022_PID, EMCEC_EL2022_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2024, EMCEC_EL2xxx_VID, EMCEC_EL2024_PID, EMCEC_EL2024_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2032, EMCEC_EL2xxx_VID, EMCEC_EL2032_PID, EMCEC_EL2032_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2034, EMCEC_EL2xxx_VID, EMCEC_EL2034_PID, EMCEC_EL2034_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2042, EMCEC_EL2xxx_VID, EMCEC_EL2042_PID, EMCEC_EL2042_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2084, EMCEC_EL2xxx_VID, EMCEC_EL2084_PID, EMCEC_EL2084_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2088, EMCEC_EL2xxx_VID, EMCEC_EL2088_PID, EMCEC_EL2088_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2124, EMCEC_EL2xxx_VID, EMCEC_EL2124_PID, EMCEC_EL2124_PDOS, emcec_el2xxx_init},
  { emcecSlaveTypeEL2808, EMCEC_EL2xxx_VID, EMCEC_EL2808_PID, EMCEC_EL2808_PDOS, emcec_el2xxx_init},

  // analog in, 2ch, 16 bits
  { emcecSlaveTypeEL3102, EMCEC_EL31x2_VID, EMCEC_EL3102_PID, EMCEC_EL31x2_PDOS, emcec_el31x2_init},
  { emcecSlaveTypeEL3112, EMCEC_EL31x2_VID, EMCEC_EL3112_PID, EMCEC_EL31x2_PDOS, emcec_el31x2_init},
  { emcecSlaveTypeEL3122, EMCEC_EL31x2_VID, EMCEC_EL3122_PID, EMCEC_EL31x2_PDOS, emcec_el31x2_init},
  { emcecSlaveTypeEL3142, EMCEC_EL31x2_VID, EMCEC_EL3142_PID, EMCEC_EL31x2_PDOS, emcec_el31x2_init},
  { emcecSlaveTypeEL3152, EMCEC_EL31x2_VID, EMCEC_EL3152_PID, EMCEC_EL31x2_PDOS, emcec_el31x2_init},
  { emcecSlaveTypeEL3162, EMCEC_EL31x2_VID, EMCEC_EL3162_PID, EMCEC_EL31x2_PDOS, emcec_el31x2_init},

  // analog out, 2ch, 16 bits
  { emcecSlaveTypeEL4102, EMCEC_EL41x2_VID, EMCEC_EL4102_PID, EMCEC_EL41x2_PDOS, emcec_el41x2_init},
  { emcecSlaveTypeEL4112, EMCEC_EL41x2_VID, EMCEC_EL4112_PID, EMCEC_EL41x2_PDOS, emcec_el41x2_init},
  { emcecSlaveTypeEL4122, EMCEC_EL41x2_VID, EMCEC_EL4122_PID, EMCEC_EL41x2_PDOS, emcec_el41x2_init},
  { emcecSlaveTypeEL4132, EMCEC_EL41x2_VID, EMCEC_EL4132_PID, EMCEC_EL41x2_PDOS, emcec_el41x2_init},

  // encoder inputs
  { emcecSlaveTypeEL5152, EMCEC_EL5152_VID, EMCEC_EL5152_PID, EMCEC_EL5152_PDOS, emcec_el5152_init},

  // pulse train (stepper) output
  { emcecSlaveTypeEL2521, EMCEC_EL2521_VID, EMCEC_EL2521_PID, EMCEC_EL2521_PDOS, emcec_el2521_init},

  // stoeber MDS5000 series
  { emcecSlaveTypeStMDS5k, EMCEC_STMDS5K_VID, EMCEC_STMDS5K_PID, EMCEC_STMDS5K_PDOS, emcec_stmds5k_init},

  { emcecSlaveTypeInvalid }
};

static emcec_master_t *first_master = NULL;
static emcec_master_t *last_master = NULL;
static int comp_id = -1;

static emcec_master_data_t *global_hal_data;
static ec_master_state_t global_ms;

int emcec_parse_config(void);
void emcec_clear_config(void);

void emcec_request_lock(void *data);
void emcec_release_lock(void *data);

emcec_master_data_t *emcec_init_master_hal(const char *pfx);
emcec_slave_state_t *emcec_init_slave_state_hal(char *master_name, char *slave_name);
void emcec_update_master_hal(emcec_master_data_t *hal_data, ec_master_state_t *ms);
void emcec_update_slave_state_hal(emcec_slave_state_t *hal_data, ec_slave_config_state_t *ss);

void emcec_read_all(void *arg, long period);
void emcec_write_all(void *arg, long period);
void emcec_read_master(void *arg, long period);
void emcec_write_master(void *arg, long period);

int rtapi_app_main(void) {
  int slave_count;
  emcec_master_t *master;
  emcec_slave_t *slave;
  char name[HAL_NAME_LEN + 1];
  ec_pdo_entry_reg_t *pdo_entry_regs;

  // connect to the HAL
  if ((comp_id = hal_init (EMCEC_MODULE_NAME)) < 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "hal_init() failed\n");
    goto fail0;
  }

  // parse configuration
  if ((slave_count = emcec_parse_config()) < 0) {
    goto fail1;
  }

  // init global hal data
  if ((global_hal_data = emcec_init_master_hal(EMCEC_MODULE_NAME)) == NULL) {
    goto fail2;
  }

  // initialize masters
  for (master = first_master; master != NULL; master = master->next) {
    // request ethercat master
    if (!(master->master = ecrt_request_master(master->index))) {
      rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "requesting master %s (index %d) failed\n", master->name, master->index);
      goto fail2;
    }

    // register callbacks
    ecrt_master_callbacks(master->master, emcec_request_lock, emcec_release_lock, master);

    // create domain
    if (!(master->domain = ecrt_master_create_domain(master->master))) {
      rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "master %s domain creation failed\n", master->name);
      goto fail2;
    }

    // initialize slaves
    pdo_entry_regs = master->pdo_entry_regs;
    for (slave = master->first_slave; slave != NULL; slave = slave->next) {
      // read slave config
      if (!(slave->config = ecrt_master_slave_config(master->master, 0, slave->index, slave->vid, slave->pid))) {
        rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "fail to read slave %s.%s configuration\n", master->name, slave->name);
        goto fail2;
      }

      // setup pdos
      if (slave->proc_init != NULL) {
        if ((slave->proc_init(comp_id, slave, pdo_entry_regs)) != 0) {
          goto fail2;
        }
      }
      pdo_entry_regs += slave->pdo_entry_count;

      // configure dc for this slave
      if (slave->dc_conf != NULL) {
        ecrt_slave_config_dc(slave->config, slave->dc_conf->assignActivate,
          slave->dc_conf->sync0Cycle, slave->dc_conf->sync0Shift,
          slave->dc_conf->sync1Cycle, slave->dc_conf->sync1Shift);
      }

      // Configure the slave's watchdog times.
      if (slave->wd_conf != NULL) {
        ecrt_slave_config_watchdog(slave->config, slave->wd_conf->divider, slave->wd_conf->intervals); 
      }

      // configure slave
      if (slave->sync_info != NULL) {
        if (ecrt_slave_config_pdos(slave->config, EC_END, slave->sync_info)) {
          rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "fail to configure slave %s.%s\n", master->name, slave->name);
          goto fail2;
        }
      }

      // export state pins
      if ((slave->hal_state_data = emcec_init_slave_state_hal(master->name, slave->name)) == NULL) {
        goto fail2;
      }
    }

    // terminate POD entries
    pdo_entry_regs->index = 0;

    // register PDO entries
    if (ecrt_domain_reg_pdo_entry_list(master->domain, master->pdo_entry_regs)) {
      rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "master %s PDO entry registration failed\n", master->name);
      goto fail2;
    }

    // activating master
    if (ecrt_master_activate(master->master)) {
      rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "failed to activate master %s\n", master->name);
      goto fail2;
    }

    // Get internal process data for domain
    master->process_data = ecrt_domain_data(master->domain);
    master->process_data_len = ecrt_domain_size(master->domain);

    // init hal data
    rtapi_snprintf(name, HAL_NAME_LEN, "%s.%s", EMCEC_MODULE_NAME, master->name);
    if ((master->hal_data = emcec_init_master_hal(name)) == NULL) {
      goto fail2;
    }

    // export read function
    rtapi_snprintf(name, HAL_NAME_LEN, "%s.%s.read", EMCEC_MODULE_NAME, master->name);
    if (hal_export_funct(name, emcec_read_master, master, 0, 0, comp_id) != 0) {
      rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "master %s read funct export failed\n", master->name);
      goto fail2;
    }
    // export write function
    rtapi_snprintf(name, HAL_NAME_LEN, "%s.%s.write", EMCEC_MODULE_NAME, master->name);
    if (hal_export_funct(name, emcec_write_master, master, 0, 0, comp_id) != 0) {
      rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "master %s write funct export failed\n", master->name);
      goto fail2;
    }
  }

  // export read-all function
  rtapi_snprintf(name, HAL_NAME_LEN, "%s.read-all", EMCEC_MODULE_NAME);
  if (hal_export_funct(name, emcec_read_all, NULL, 0, 0, comp_id) != 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "read-all funct export failed\n");
    goto fail2;
  }
  // export write-all function
  rtapi_snprintf(name, HAL_NAME_LEN, "%s.write-all", EMCEC_MODULE_NAME);
  if (hal_export_funct(name, emcec_write_all, NULL, 0, 0, comp_id) != 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "write-all funct export failed\n");
    goto fail2;
  }

  rtapi_print_msg(RTAPI_MSG_INFO, EMCEC_MSG_PFX "installed driver for %d slaves\n", slave_count);
  hal_ready (comp_id);
  return 0;

fail2:
  emcec_clear_config();
fail1:
  hal_exit(comp_id);
fail0:
  return -EINVAL;
}

void rtapi_app_exit(void) {
  emcec_clear_config();
  hal_exit(comp_id);
}

int emcec_parse_config(void) {
  int shmem_id;
  void *shmem_ptr;
  EMCEC_CONF_HEADER_T *header;
  size_t length;
  void *conf;
  int slave_count;
  const emcec_typelist_t *type;
  emcec_master_t *master;
  emcec_slave_t *slave;
  emcec_slave_dc_t *dc;
  emcec_slave_watchdog_t *wd;
  ec_pdo_entry_reg_t *pdo_entry_regs;
  EMCEC_CONF_TYPE_T conf_type;
  EMCEC_CONF_MASTER_T *master_conf;
  EMCEC_CONF_SLAVE_T *slave_conf;
  EMCEC_CONF_DC_T *dc_conf;
  EMCEC_CONF_WATCHDOG_T *wd_conf;

  // initialize list
  first_master = NULL;
  last_master = NULL;

  // try to get config header
  shmem_id = rtapi_shmem_new(EMCEC_CONF_SHMEM_KEY, comp_id, sizeof(EMCEC_CONF_HEADER_T));
  if (shmem_id < 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "couldn't allocate user/RT shared memory\n");
    goto fail0;
  }
  if (rtapi_shmem_getptr(shmem_id, &shmem_ptr) < 0 ) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "couldn't map user/RT shared memory\n");
    goto fail1;
  }

  // check magic, get length and close shmem
  header = shmem_ptr;
  if (header->magic != EMCEC_CONF_SHMEM_MAGIC) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "emcec_conf is not loaded\n");
    goto fail1;
  }
  length = header->length;
  rtapi_shmem_delete(shmem_id, comp_id);

  // reopen shmem with proper size 
  shmem_id = rtapi_shmem_new(EMCEC_CONF_SHMEM_KEY, comp_id, sizeof(EMCEC_CONF_HEADER_T) + length);
  if (shmem_id < 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "couldn't allocate user/RT shared memory\n");
    goto fail0;
  }
  if (rtapi_shmem_getptr(shmem_id, &shmem_ptr) < 0 ) {
    rtapi_print_msg (RTAPI_MSG_ERR, EMCEC_MSG_PFX "couldn't map user/RT shared memory\n");
    goto fail1;
  }

  // get pointer to config
  conf = shmem_ptr + sizeof(EMCEC_CONF_HEADER_T);

  // process config items
  slave_count = 0;
  master = NULL;
  slave = NULL;
  while((conf_type = ((EMCEC_CONF_NULL_T *)conf)->confType) != emcecConfTypeNone) {
    // get type
    switch (conf_type) {
      case emcecConfTypeMaster:
        // get config token
        master_conf = (EMCEC_CONF_MASTER_T *)conf;
        conf += sizeof(EMCEC_CONF_MASTER_T);

        // alloc master memory
        master = kzalloc(sizeof(emcec_master_t), GFP_KERNEL);
        if (master == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Unable to allocate master %d structure memory\n", master_conf->index);
          goto fail2;
        }      

        // initialize master
        master->index = master_conf->index;
        strncpy(master->name, master_conf->name, EMCEC_CONF_STR_MAXLEN);
        master->name[EMCEC_CONF_STR_MAXLEN - 1] = 0;
        rt_sem_init(&master->semaphore, 1);
        master->app_time = 0;
        master->app_time_period = master_conf->appTimePeriod;
        master->sync_ref_cnt = 0;
        master->sync_ref_cycles = master_conf->refClockSyncCycles;

        // add master to list
        EMCEC_LIST_APPEND(first_master, last_master, master);
        break;

      case emcecConfTypeSlave:
        // get config token
        slave_conf = (EMCEC_CONF_SLAVE_T *)conf;
        conf += sizeof(EMCEC_CONF_SLAVE_T);

        // check for master
        if (master == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Master node for slave missing\n");
          goto fail2;
        }

        // check for valid slave type
        for (type = types; type->type != emcecSlaveTypeInvalid; type++) {
          if (type->type == slave_conf->type) {
            break;
          }
        }
        if (type->type == emcecSlaveTypeInvalid) {
          rtapi_print_msg(RTAPI_MSG_WARN, EMCEC_MSG_PFX "Invalid slave type %d\n", slave_conf->type);
          continue;
        }

        // create new slave
        slave = kzalloc(sizeof(emcec_slave_t), GFP_KERNEL);
        if (slave == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Unable to allocate slave %s.%s structure memory\n", master->name, slave_conf->name);
          goto fail2;
        }      

        // initialize slave
        slave->index = slave_conf->index;
        strncpy(slave->name, slave_conf->name, EMCEC_CONF_STR_MAXLEN);
        slave->name[EMCEC_CONF_STR_MAXLEN - 1] = 0;
        slave->master = master;
        slave->vid = type->vid;
        slave->pid = type->pid;
        slave->pdo_entry_count = type->pdo_entry_count;
        slave->proc_init = type->proc_init;
        slave->dc_conf = NULL;
        slave->wd_conf = NULL;

        // add slave to list
        EMCEC_LIST_APPEND(master->first_slave, master->last_slave, slave);

        // update master's POD entry count
        master->pdo_entry_count += type->pdo_entry_count;

        // update slave count
        slave_count++;
        break;

      case emcecConfTypeDcConf:
        // get config token
        dc_conf = (EMCEC_CONF_DC_T *)conf;
        conf += sizeof(EMCEC_CONF_DC_T);
        break;

        // check for slave
        if (slave == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Slave node for dc config missing\n");
          goto fail2;
        }

        // check for double dc config
        if (slave->dc_conf != NULL) {
          rtapi_print_msg(RTAPI_MSG_WARN, EMCEC_MSG_PFX "Double dc config for slave %s.%s\n", master->name, slave->name);
          continue;
        }

        // create new dc config
        dc = kzalloc(sizeof(emcec_slave_dc_t), GFP_KERNEL);
        if (dc == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Unable to allocate slave %s.%s dc config memory\n", master->name, slave->name);
          goto fail2;
        }      

        // initialize dc conf
        dc->assignActivate = dc_conf->assignActivate;
        dc->sync0Cycle = dc_conf->sync0Cycle;
        dc->sync0Shift = dc_conf->sync0Shift;
        dc->sync1Cycle = dc_conf->sync1Cycle;
        dc->sync1Shift = dc_conf->sync1Shift;

        // add to slave
        slave->dc_conf = dc;

      case emcecConfTypeWatchdog:
        // get config token
        wd_conf = (EMCEC_CONF_WATCHDOG_T *)conf;
        conf += sizeof(EMCEC_CONF_WATCHDOG_T);
        break;

        // check for slave
        if (slave == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Slave node for watchdog config missing\n");
          goto fail2;
        }

        // check for double wd config
        if (slave->wd_conf != NULL) {
          rtapi_print_msg(RTAPI_MSG_WARN, EMCEC_MSG_PFX "Double watchdog config for slave %s.%s\n", master->name, slave->name);
          continue;
        }

        // create new wd config
        wd = kzalloc(sizeof(emcec_slave_watchdog_t), GFP_KERNEL);
        if (wd == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Unable to allocate slave %s.%s watchdog config memory\n", master->name, slave->name);
          goto fail2;
        }      

        // initialize wd conf
        wd->divider = wd_conf->divider;
        wd->intervals = wd_conf->intervals;

        // add to slave
        slave->wd_conf = wd;

      default:
        rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Unknown config item type\n");
        goto fail2;
    }
  }

  // close shmem
  rtapi_shmem_delete(shmem_id, comp_id);

  // allocate PDO etity memory
  for (master = first_master; master != NULL; master = master->next) {
    pdo_entry_regs = kzalloc(sizeof(ec_pdo_entry_reg_t) * (master->pdo_entry_count + 1), GFP_KERNEL);
    if (pdo_entry_regs == NULL) {
      rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "Unable to allocate master %s PDO entry memory\n", master->name);
      goto fail2;
    }
    master->pdo_entry_regs = pdo_entry_regs;      
  }

  return slave_count;

fail2:
  emcec_clear_config();
fail1:
  rtapi_shmem_delete(shmem_id, comp_id);
fail0:
  return -1;
}

void emcec_clear_config(void) {
  emcec_master_t *master, *prev_master;
  emcec_slave_t *slave, *prev_slave;

  // iterate all masters
  master = last_master;
  while (master != NULL) {
    prev_master = master->prev;

    // iterate all masters
    slave = master->last_slave;
    while (slave != NULL) {
      prev_slave = slave->prev;

      // cleanup slave
      if (slave->proc_cleanup != NULL) {
        slave->proc_cleanup(slave);
      }

      // free slave
      if (slave->dc_conf != NULL) {
        kfree(slave->dc_conf);
      }
      if (slave->wd_conf != NULL) {
        kfree(slave->wd_conf);
      }
      kfree(slave);
      slave = prev_slave;
    }

    // release master
    if (master->master) {
      ecrt_release_master(master->master);
    }

    // free PDO entry memory
    if (master->pdo_entry_regs != NULL) {
      kfree(master->pdo_entry_regs);
    }

    // delete semaphore
    rt_sem_delete(&master->semaphore);

    // free master
    kfree(master);
    master = prev_master;
  }
}

void emcec_request_lock(void *data) {
  emcec_master_t *master = (emcec_master_t *) data;
  rt_sem_wait(&master->semaphore);
}

void emcec_release_lock(void *data) {
  emcec_master_t *master = (emcec_master_t *) data;
  rt_sem_signal(&master->semaphore);
}

emcec_master_data_t *emcec_init_master_hal(const char *pfx) {
  emcec_master_data_t *hal_data;

  // alloc hal data
  if ((hal_data = hal_malloc(sizeof(emcec_master_data_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "hal_malloc() for %s failed\n", pfx);
    return NULL;
  }
  memset(hal_data, 0, sizeof(emcec_master_data_t));

  // export pins
  if (hal_pin_u32_newf(HAL_OUT, &(hal_data->slaves_responding), comp_id, "%s.slaves-responding", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.slaves-responding failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_init), comp_id, "%s.state-init", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.state-init failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_preop), comp_id, "%s.state-preop", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.state-preop failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_safeop), comp_id, "%s.state-safeop", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.state-safeop failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_op), comp_id, "%s.state-op", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.state-op failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->link_up), comp_id, "%s.link-up", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.link-up failed\n", pfx);
    return NULL;
  }

  // initialize pins
  *(hal_data->slaves_responding) = 0;
  *(hal_data->state_init) = 0;
  *(hal_data->state_preop) = 0;
  *(hal_data->state_safeop) = 0;
  *(hal_data->state_op) = 0;
  *(hal_data->link_up) = 0;

  return hal_data;
}

emcec_slave_state_t *emcec_init_slave_state_hal(char *master_name, char *slave_name) {
  emcec_slave_state_t *hal_data;

  // alloc hal data
  if ((hal_data = hal_malloc(sizeof(emcec_slave_state_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "hal_malloc() for %s.%s.%s failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  memset(hal_data, 0, sizeof(emcec_master_data_t));

  // export pins
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->online), comp_id, "%s.%s.%s.slave-online", EMCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.slaves-online failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->operational), comp_id, "%s.%s.%s.slave-oper", EMCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.slaves-oper failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_init), comp_id, "%s.%s.%s.slave-state-init", EMCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-init failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_preop), comp_id, "%s.%s.%s.slave-state-preop", EMCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-preop failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_safeop), comp_id, "%s.%s.%s.slave-state-safeop", EMCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-safeop failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_op), comp_id, "%s.%s.%s.slave-state-op", EMCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-op failed\n", EMCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }

  // initialize pins
  *(hal_data->online) = 0;
  *(hal_data->operational) = 0;
  *(hal_data->state_init) = 0;
  *(hal_data->state_preop) = 0;
  *(hal_data->state_safeop) = 0;
  *(hal_data->state_op) = 0;

  return hal_data;
}

void emcec_update_master_hal(emcec_master_data_t *hal_data, ec_master_state_t *ms) {
  *(hal_data->slaves_responding) = ms->slaves_responding;
  *(hal_data->state_init) = (ms->al_states & 0x01) != 0;
  *(hal_data->state_preop) = (ms->al_states & 0x02) != 0;
  *(hal_data->state_safeop) = (ms->al_states & 0x04) != 0;
  *(hal_data->state_op) = (ms->al_states & 0x08) != 0;
  *(hal_data->link_up) = ms->link_up;
}

void emcec_update_slave_state_hal(emcec_slave_state_t *hal_data, ec_slave_config_state_t *ss) {
  *(hal_data->online) = ss->online;
  *(hal_data->operational) = ss->operational;
  *(hal_data->state_init) = (ss->al_state & 0x01) != 0;
  *(hal_data->state_preop) = (ss->al_state & 0x02) != 0;
  *(hal_data->state_safeop) = (ss->al_state & 0x04) != 0;
  *(hal_data->state_op) = (ss->al_state & 0x08) != 0;
}

void emcec_read_all(void *arg, long period) {
  emcec_master_t *master;

  // initialize global state
  global_ms.slaves_responding = 0;
  global_ms.al_states = 0;
  global_ms.link_up = (first_master != NULL);

  // process slaves
  for (master = first_master; master != NULL; master = master->next) {
    emcec_read_master(master, period);
  }

  // update global state pins
  emcec_update_master_hal(global_hal_data, &global_ms);
}

void emcec_write_all(void *arg, long period) {
  emcec_master_t *master;

  // process slaves
  for (master = first_master; master != NULL; master = master->next) {
    emcec_write_master(master, period);
  }
}

void emcec_read_master(void *arg, long period) {
  emcec_master_t *master = (emcec_master_t *) arg;
  emcec_slave_t *slave;
  ec_master_state_t ms;

  // receive process data & master state
  rt_sem_wait(&master->semaphore);
  ecrt_master_receive(master->master);
  ecrt_domain_process(master->domain);
  ecrt_master_state(master->master, &ms);
  rt_sem_signal(&master->semaphore);

  // update state pins
  emcec_update_master_hal(master->hal_data, &ms);

  // update global state
  global_ms.slaves_responding += ms.slaves_responding;
  global_ms.al_states |= ms.al_states;
  global_ms.link_up = global_ms.link_up && ms.link_up;

  // process slaves
  for (slave = master->first_slave; slave != NULL; slave = slave->next) {
    // get slaves state
    rt_sem_wait(&master->semaphore);
    ecrt_slave_config_state(slave->config, &slave->state);
    rt_sem_signal(&master->semaphore);
    emcec_update_slave_state_hal(slave->hal_state_data, &slave->state);

    // process read function
    if (slave->proc_read != NULL) {
      slave->proc_read(slave, period);
    }
  }
}

void emcec_write_master(void *arg, long period) {
  emcec_master_t *master = (emcec_master_t *) arg;
  emcec_slave_t *slave;

  // process slaves
  for (slave = master->first_slave; slave != NULL; slave = slave->next) {
    if (slave->proc_write != NULL) {
      slave->proc_write(slave, period);
    }
  }

  // send process data
  rt_sem_wait(&master->semaphore);

  // update application time
  master->app_time += master->app_time_period;
  ecrt_master_application_time(master->master, master->app_time);

  // sync ref clock to master            
  if (master->sync_ref_cnt == 0) {
    master->sync_ref_cnt = master->sync_ref_cycles;
    ecrt_master_sync_reference_clock(master->master);
  }
  master->sync_ref_cnt--;

  // sync slaves to ref clock
  ecrt_master_sync_slave_clocks(master->master);

  // send domain data
  ecrt_domain_queue(master->domain);
  ecrt_master_send(master->master);
  rt_sem_signal(&master->semaphore);
}

ec_sdo_request_t *emcec_read_sdo(struct emcec_slave *slave, uint16_t index, uint8_t subindex, size_t size) {
  emcec_master_t *master = slave->master;
  ec_sdo_request_t *sdo;
  ec_request_state_t sdo_state;

  // create request
  if (!(sdo = ecrt_slave_config_create_sdo_request(slave->config, index, subindex, size))) {
  rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "slave %s.%s: Failed to create SDO request (0x%04x:0x%02x)\n", master->name, slave->name, index, subindex);
      return NULL;
  }

  // set timeout
  ecrt_sdo_request_timeout(sdo, EMCEC_SDO_REQ_TIMEOUT);

  // send request
  ecrt_sdo_request_read(sdo);

  // wait for completition
  while ((sdo_state = ecrt_sdo_request_state(sdo)) == EC_REQUEST_BUSY) {
    schedule();
  }

  // check state
  if (sdo_state != EC_REQUEST_SUCCESS) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "slave %s.%s: Failed to execute SDO request (0x%04x:0x%02x)\n", master->name, slave->name, index, subindex);
    return NULL;
  }

  return sdo;
}

