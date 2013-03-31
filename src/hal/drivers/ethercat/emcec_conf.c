//
//  Copyright (C) 2012 Sascha Ittner <sascha.ittner@modusoft.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <expat.h>
#include <signal.h>
#include <sys/eventfd.h>

#include "rtapi.h"
#include "hal.h"

#include "emcec_conf.h"

#define BUFFSIZE 8192

typedef struct {
  char *name;
  EMCEC_SLAVE_TYPE_T type;
} EMCEC_CONF_TYPELIST_T;

typedef struct {
  hal_u32_t *master_count;
  hal_u32_t *slave_count;
} EMCEC_CONF_HAL_T;

static const EMCEC_CONF_TYPELIST_T slaveTypes[] = {
  // bus coupler
  { "EK1100", emcecSlaveTypeEK1100 },

  // generic device
  { "generic", emcecSlaveTypeGeneric },

  // digital in
  { "EL1002", emcecSlaveTypeEL1002 },
  { "EL1004", emcecSlaveTypeEL1004 },
  { "EL1008", emcecSlaveTypeEL1008 },
  { "EL1012", emcecSlaveTypeEL1012 },
  { "EL1014", emcecSlaveTypeEL1014 },
  { "EL1018", emcecSlaveTypeEL1018 },
  { "EL1024", emcecSlaveTypeEL1024 },
  { "EL1034", emcecSlaveTypeEL1034 },
  { "EL1084", emcecSlaveTypeEL1084 },
  { "EL1088", emcecSlaveTypeEL1088 },
  { "EL1094", emcecSlaveTypeEL1094 },
  { "EL1098", emcecSlaveTypeEL1098 },
  { "EL1104", emcecSlaveTypeEL1104 },
  { "EL1114", emcecSlaveTypeEL1114 },
  { "EL1124", emcecSlaveTypeEL1124 },
  { "EL1134", emcecSlaveTypeEL1134 },
  { "EL1144", emcecSlaveTypeEL1144 },
  { "EL1808", emcecSlaveTypeEL1808 },
  { "EL1809", emcecSlaveTypeEL1809 },

  // digital out
  { "EL2002", emcecSlaveTypeEL2002 },
  { "EL2004", emcecSlaveTypeEL2004 },
  { "EL2008", emcecSlaveTypeEL2008 },
  { "EL2022", emcecSlaveTypeEL2022 },
  { "EL2024", emcecSlaveTypeEL2024 },
  { "EL2032", emcecSlaveTypeEL2032 },
  { "EL2034", emcecSlaveTypeEL2034 },
  { "EL2042", emcecSlaveTypeEL2042 },
  { "EL2084", emcecSlaveTypeEL2084 },
  { "EL2088", emcecSlaveTypeEL2088 },
  { "EL2124", emcecSlaveTypeEL2124 },
  { "EL2808", emcecSlaveTypeEL2808 },
  { "EL2809", emcecSlaveTypeEL2809 },

  // analog in, 2ch, 16 bits
  { "EL3102", emcecSlaveTypeEL3102 },
  { "EL3112", emcecSlaveTypeEL3112 },
  { "EL3122", emcecSlaveTypeEL3122 },
  { "EL3142", emcecSlaveTypeEL3142 },
  { "EL3152", emcecSlaveTypeEL3152 },
  { "EL3162", emcecSlaveTypeEL3162 },

  // analog out, 2ch, 12 bits
  { "EL4002", emcecSlaveTypeEL4002 },
  { "EL4012", emcecSlaveTypeEL4012 },
  { "EL4022", emcecSlaveTypeEL4022 },
  { "EL4032", emcecSlaveTypeEL4032 },

  // analog out, 2ch, 16 bits
  { "EL4102", emcecSlaveTypeEL4102 },
  { "EL4112", emcecSlaveTypeEL4112 },
  { "EL4122", emcecSlaveTypeEL4122 },
  { "EL4132", emcecSlaveTypeEL4132 },

  // encoder inputs
  { "EL5151", emcecSlaveTypeEL5151 },
  { "EL5152", emcecSlaveTypeEL5152 },

  // pulse train (stepper) output
  { "EL2521", emcecSlaveTypeEL2521 },

  // dc servo
  { "EL7342", emcecSlaveTypeEL7342 },

  // power suppply
  { "EL9505", emcecSlaveTypeEL9505 },
  { "EL9508", emcecSlaveTypeEL9508 },
  { "EL9510", emcecSlaveTypeEL9510 },
  { "EL9512", emcecSlaveTypeEL9512 },
  { "EL9515", emcecSlaveTypeEL9515 },

  // stoeber MDS5000 series
  { "StMDS5k", emcecSlaveTypeStMDS5k },

  { NULL }
};

char *modname = "emcec_conf";
int hal_comp_id;
EMCEC_CONF_HAL_T *conf_hal_data;

int exitEvent;

XML_Parser parser;
int currConfType;
void *outputBuffer;
size_t outputBufferLen;
size_t outputBufferPos;

EMCEC_CONF_MASTER_T *currMaster;
EMCEC_CONF_SLAVE_T *currSlave;
EMCEC_CONF_SYNCMANAGER_T *currSyncManager;
EMCEC_CONF_PDO_T *currPdo;
EMCEC_CONF_SDOCONF_T *currSdoConf;

int shmem_id;

void xml_start_handler(void *data, const char *el, const char **attr);
void xml_end_handler(void *data, const char *el);

void *getOutputBuffer(size_t len);

int parseHexdump(const char *str, uint8_t *buf);

void parseMasterAttrs(const char **attr);
void parseSlaveAttrs(const char **attr);
void parseDcConfAttrs(const char **attr);
void parseWatchdogAttrs(const char **attr);
void parseSdoConfigAttrs(const char **attr);
void parseSdoDataRawAttrs(const char **attr);
void parseSyncManagerAttrs(const char **attr);
void parsePdoAttrs(const char **attr);
void parsePdoEntryAttrs(const char **attr);

int parseSyncCycle(const char *nptr);

static void exitHandler(int sig) {
  uint64_t u = 1;
  if (write(exitEvent, &u, sizeof(uint64_t)) < 0) {
    fprintf(stderr, "%s: ERROR: error writing exit event\n", modname);
  }
}

int main(int argc, char **argv) {
  int ret = 1;
  char *filename;
  int done;
  char buffer[BUFFSIZE];
  FILE *file;
  EMCEC_CONF_NULL_T *end;
  void *shmem_ptr;
  EMCEC_CONF_HEADER_T *header;
  uint64_t u;

  // initialize component
  hal_comp_id = hal_init(modname);
  if (hal_comp_id < 1) {
    fprintf(stderr, "%s: ERROR: hal_init failed\n", modname);
    goto fail0;
  }

  // allocate hal memory
  conf_hal_data = hal_malloc(sizeof(EMCEC_CONF_HAL_T));
  if (conf_hal_data == NULL) {
    fprintf(stderr, "%s: ERROR: unable to allocate HAL shared memory\n", modname);
    goto fail1;
  }

  // register pins
  if (hal_pin_u32_newf(HAL_OUT, &(conf_hal_data->master_count), hal_comp_id, "%s.conf.master-count", EMCEC_MODULE_NAME) != 0) {
    fprintf(stderr, "%s: ERROR: unable to register pin %s.conf.master-count\n", modname, EMCEC_MODULE_NAME);
    goto fail1;
  }
  if (hal_pin_u32_newf(HAL_OUT, &(conf_hal_data->slave_count), hal_comp_id, "%s.conf.slave-count", EMCEC_MODULE_NAME) != 0) {
    fprintf(stderr, "%s: ERROR: unable to register pin %s.conf.slave-count\n", modname, EMCEC_MODULE_NAME);
    goto fail1;
  }
  *conf_hal_data->master_count = 0;
  *conf_hal_data->slave_count = 0;

  // initialize signal handling
  exitEvent = eventfd(0, 0);
  if (exitEvent == -1) {
    fprintf(stderr, "%s: ERROR: unable to create exit event\n", modname);
    goto fail1;
  }
  signal(SIGINT, exitHandler);
  signal(SIGTERM, exitHandler);

  // get config file name
  if (argc != 2) {
    fprintf(stderr, "%s: ERROR: invalid arguments\n", modname);
    goto fail2;
  }
  filename = argv[1];

  // open file
  file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "%s: ERROR: unable to open config file %s\n", modname, filename);
    goto fail2;
  }

  // create xml parser
  parser = XML_ParserCreate(NULL);
  if (parser == NULL) {
    fprintf(stderr, "%s: ERROR: Couldn't allocate memory for parser\n", modname);
    goto fail3;
  }

  // setup handlers
  XML_SetElementHandler(parser, xml_start_handler, xml_end_handler);

  currConfType = emcecConfTypeNone;
  outputBuffer = NULL;
  outputBufferLen = 0;
  outputBufferPos = 0;
  currMaster = NULL;
  currSlave = NULL;
  currSyncManager = NULL;
  currPdo = NULL;
  currSdoConf = NULL;
  for (done=0; !done;) {
    // read block
    int len = fread(buffer, 1, BUFFSIZE, file);
    if (ferror(file)) {
      fprintf(stderr, "%s: ERROR: Couldn't read from file %s\n", modname, filename);
      goto fail4;
    }

    // check for EOF
    done = feof(file);

    // parse current block
    if (!XML_Parse(parser, buffer, len, done)) {
      fprintf(stderr, "%s: ERROR: Parse error at line %u: %s\n", modname,
        (unsigned int)XML_GetCurrentLineNumber(parser),
        XML_ErrorString(XML_GetErrorCode(parser)));
      goto fail4;
    }
  }

  // set end marker
  end = getOutputBuffer(sizeof(EMCEC_CONF_NULL_T));
  if (end == NULL) {
      goto fail4;
  }
  end->confType = emcecConfTypeNone;

  // setup shared mem for config
  shmem_id = rtapi_shmem_new(EMCEC_CONF_SHMEM_KEY, hal_comp_id, sizeof(EMCEC_CONF_HEADER_T) + outputBufferPos);
  if ( shmem_id < 0 ) {
    fprintf(stderr, "%s: ERROR: couldn't allocate user/RT shared memory\n", modname);
    goto fail4;
  }
  if (rtapi_shmem_getptr(shmem_id, &shmem_ptr) < 0) {
    fprintf(stderr, "%s: ERROR: couldn't map user/RT shared memory\n", modname);
    goto fail5;
  }
  
  // setup header
  header = shmem_ptr;
  shmem_ptr += sizeof(EMCEC_CONF_HEADER_T);
  header->magic = EMCEC_CONF_SHMEM_MAGIC;
  header->length = outputBufferPos;

  // copy data
  memcpy(shmem_ptr, outputBuffer, outputBufferPos);

  // free build buffer
  free(outputBuffer);
  outputBuffer = NULL;

  // everything is fine
  ret = 0;
  hal_ready(hal_comp_id);

  // wait for SIGTERM
  if (read(exitEvent, &u, sizeof(uint64_t)) < 0) {
    fprintf(stderr, "%s: ERROR: error reading exit event\n", modname);
  }

fail5:
  rtapi_shmem_delete(shmem_id, hal_comp_id);
fail4:
  if (outputBuffer != NULL) {
    free(outputBuffer);
  }
  XML_ParserFree(parser);
fail3:
  fclose(file);
fail2:
  close(exitEvent);
fail1:
  hal_exit(hal_comp_id);
fail0:
  return ret;
}

void xml_start_handler(void *data, const char *el, const char **attr) {
  switch (currConfType) {
    case emcecConfTypeNone:
      if (strcmp(el, "master") == 0) {
        currConfType = emcecConfTypeMaster;
        parseMasterAttrs(attr);
        return;
      }
      break;
    case emcecConfTypeMaster:
      if (strcmp(el, "slave") == 0) {
        currConfType = emcecConfTypeSlave;
        parseSlaveAttrs(attr);
        return;
      }
      break;
    case emcecConfTypeSlave:
      if (strcmp(el, "dcConf") == 0) {
        currConfType = emcecConfTypeDcConf;
        parseDcConfAttrs(attr);
        return;
      }
      if (strcmp(el, "watchdog") == 0) {
        currConfType = emcecConfTypeWatchdog;
        parseWatchdogAttrs(attr);
        return;
      }
      if (strcmp(el, "sdoConfig") == 0) {
        currConfType = emcecConfTypeSdoConfig;
        parseSdoConfigAttrs(attr);
        return;
      }
      if (currSlave->type == emcecSlaveTypeGeneric && strcmp(el, "syncManager") == 0) {
        currConfType = emcecConfTypeSyncManager;
        parseSyncManagerAttrs(attr);
        return;
      }
      break;
    case emcecConfTypeSdoConfig:
      if (strcmp(el, "sdoDataRaw") == 0) {
        currConfType = emcecConfTypeSdoDataRaw;
        parseSdoDataRawAttrs(attr);
        return;
      }
      break;
    case emcecConfTypeSyncManager:
      if (strcmp(el, "pdo") == 0) {
        currConfType = emcecConfTypePdo;
        parsePdoAttrs(attr);
        return;
      }
      break;
    case emcecConfTypePdo:
      if (strcmp(el, "pdoEntry") == 0) {
        currConfType = emcecConfTypePdoEntry;
        parsePdoEntryAttrs(attr);
        return;
      }
      break;
  }

  fprintf(stderr, "%s: ERROR: unexpected node %s found\n", modname, el);
  XML_StopParser(parser, 0);
}

void xml_end_handler(void *data, const char *el) {
  switch (currConfType) {
    case emcecConfTypeMaster:
      if (strcmp(el, "master") == 0) {
        currConfType = emcecConfTypeNone;
        return;
      }
      break;
    case emcecConfTypeSlave:
      if (strcmp(el, "slave") == 0) {
        currConfType = emcecConfTypeMaster;
        return;
      }
      break;
    case emcecConfTypeDcConf:
      if (strcmp(el, "dcConf") == 0) {
        currConfType = emcecConfTypeSlave;
        return;
      }
      break;
    case emcecConfTypeWatchdog:
      if (strcmp(el, "watchdog") == 0) {
        currConfType = emcecConfTypeSlave;
        return;
      }
      break;
    case emcecConfTypeSdoConfig:
      if (strcmp(el, "sdoConfig") == 0) {
        currConfType = emcecConfTypeSlave;
        return;
      }
      break;
    case emcecConfTypeSdoDataRaw:
      if (strcmp(el, "sdoDataRaw") == 0) {
        currConfType = emcecConfTypeSdoConfig;
        return;
      }
      break;
    case emcecConfTypeSyncManager:
      if (strcmp(el, "syncManager") == 0) {
        currConfType = emcecConfTypeSlave;
        return;
      }
      break;
    case emcecConfTypePdo:
      if (strcmp(el, "pdo") == 0) {
        currConfType = emcecConfTypeSyncManager;
        return;
      }
      break;
    case emcecConfTypePdoEntry:
      if (strcmp(el, "pdoEntry") == 0) {
        currConfType = emcecConfTypePdo;
        return;
      }
      break;
  }

  fprintf(stderr, "%s: ERROR: unexpected close tag %s found\n", modname, el);
  XML_StopParser(parser, 0);
}

void *getOutputBuffer(size_t len) {
  void *p;

  // reallocate if len do not fit into current buffer
  while ((outputBufferLen - outputBufferPos) < len) {
    size_t new = outputBufferLen + BUFFSIZE;
    outputBuffer = realloc(outputBuffer, new);
    if (outputBuffer == NULL) {
      fprintf(stderr, "%s: ERROR: Couldn't allocate memory for config token\n", modname);
      XML_StopParser(parser, 0);
      return NULL;
    }
    outputBufferLen = new;
  }

  // initialize memory
  p = outputBuffer + outputBufferPos;
  memset(p, 0, len);
  outputBufferPos += len;
  return p;
}

int parseHexdump(const char *str, uint8_t *buf) {
  char c;
  int len = 0;
  int nib = 0;
  uint8_t val = 0;

  while (1) {
    c = *(str++);

    // check for seperator or end of string
    if (c == 0 || c == ' ') {
      if (nib > 0) {
        if (buf != NULL) {
          *(buf++) = val;
        }
        len++;
        nib = 0;
        val = 0;
      }
      if (c == 0) {
        return len;
      }
      continue;
    }

    // get nibble value
    if (c >= '0' && c <= '9') {
      c = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      c = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      c = c - 'A' + 10;
    } else {
      return -1;
    }

    // allow only byte length
    if (nib >= 2) {
      return -1;
    }

    val = (val << 4) + c;
    nib++;
  }
}

void parseMasterAttrs(const char **attr) {
  EMCEC_CONF_MASTER_T *p = getOutputBuffer(sizeof(EMCEC_CONF_MASTER_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypeMaster;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse index
    if (strcmp(name, "idx") == 0) {
      p->index = atoi(val);
      continue;
    }

    // parse name
    if (strcmp(name, "name") == 0) {
      strncpy(p->name, val, EMCEC_CONF_STR_MAXLEN);
      p->name[EMCEC_CONF_STR_MAXLEN - 1] = 0;
      continue;
    }

    // parse appTimePeriod
    if (strcmp(name, "appTimePeriod") == 0) {
      p->appTimePeriod = atol(val);
      continue;
    }

    // parse refClockSyncCycles
    if (strcmp(name, "refClockSyncCycles") == 0) {
      p->refClockSyncCycles = atoll(val);
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid master attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }

  // set default name
  if (p->name[0] == 0) {
    snprintf(p->name, EMCEC_CONF_STR_MAXLEN, "%d", p->index);
  }

  (*(conf_hal_data->master_count))++;
  currMaster = p;
}

void parseSlaveAttrs(const char **attr) {
  EMCEC_CONF_SLAVE_T *p = getOutputBuffer(sizeof(EMCEC_CONF_SLAVE_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypeSlave;
  p->type = emcecSlaveTypeInvalid;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse index
    if (strcmp(name, "idx") == 0) {
      p->index = atoi(val);
      continue;
    }

    // parse slave type
    if (strcmp(name, "type") == 0) {
      const EMCEC_CONF_TYPELIST_T *slaveType;
      for (slaveType = slaveTypes; slaveType->name != NULL; slaveType++) {
        if (strcmp(val, slaveType->name) == 0) {
          break;
        }
      }
      if (slaveType->name == NULL) {
        fprintf(stderr, "%s: ERROR: Invalid slave type %s\n", modname, val);
        XML_StopParser(parser, 0);
        return;
      }
      p->type = slaveType->type;
      continue;
    }

    // parse name
    if (strcmp(name, "name") == 0) {
      strncpy(p->name, val, EMCEC_CONF_STR_MAXLEN);
      p->name[EMCEC_CONF_STR_MAXLEN - 1] = 0;
      continue;
    }

    // generic only attributes
    if (p->type == emcecSlaveTypeGeneric) {
      // parse vid (hex value)
      if (strcmp(name, "vid") == 0) {
        p->vid = strtol(val, NULL, 16);
        continue;
      }

      // parse pid (hex value)
      if (strcmp(name, "pid") == 0) {
        p->pid = strtol(val, NULL, 16);
        continue;
      }

      // parse configPdos
      if (strcmp(name, "configPdos") == 0) {
        p->configPdos = (strcasecmp(val, "true") == 0);
        continue;
      }
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid slave attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }

  // set default name
  if (p->name[0] == 0) {
    snprintf(p->name, EMCEC_CONF_STR_MAXLEN, "%d", p->index);
  }

  // type is required
  if (p->type == emcecSlaveTypeInvalid) {
    fprintf(stderr, "%s: ERROR: Slave has no type attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  (*(conf_hal_data->slave_count))++;
  currSlave = p;
}

void parseDcConfAttrs(const char **attr) {
  EMCEC_CONF_DC_T *p = getOutputBuffer(sizeof(EMCEC_CONF_DC_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypeDcConf;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse assignActivate (hex value)
    if (strcmp(name, "assignActivate") == 0) {
      p->assignActivate = strtol(val, NULL, 16);
      continue;
    }

    // parse sync0Cycle
    if (strcmp(name, "sync0Cycle") == 0) {
      p->sync0Cycle = parseSyncCycle(val);
      continue;
    }

    // parse sync0Shift
    if (strcmp(name, "sync0Shift") == 0) {
      p->sync0Shift = atoi(val);
      continue;
    }

    // parse sync1Cycle
    if (strcmp(name, "sync1Cycle") == 0) {
      p->sync1Cycle = parseSyncCycle(val);
      continue;
    }

    // parse sync1Shift
    if (strcmp(name, "sync1Shift") == 0) {
      p->sync1Shift = atoi(val);
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid dcConfig attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }
}

void parseWatchdogAttrs(const char **attr) {
  EMCEC_CONF_WATCHDOG_T *p = getOutputBuffer(sizeof(EMCEC_CONF_WATCHDOG_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypeWatchdog;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse divider
    if (strcmp(name, "divider") == 0) {
      p->divider = atoi(val);
      continue;
    }

    // parse intervals
    if (strcmp(name, "intervals") == 0) {
      p->intervals = atoi(val);
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid watchdog attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }
}

void parseSdoConfigAttrs(const char **attr) {
  int tmp;
  EMCEC_CONF_SDOCONF_T *p = getOutputBuffer(sizeof(EMCEC_CONF_SDOCONF_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypeSdoConfig;
  p->index = 0xffff;
  p->subindex = 0xff;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse index
    if (strcmp(name, "idx") == 0) {
      tmp = strtol(val, NULL, 16);
      if (tmp < 0 || tmp >= 0xffff) {
        fprintf(stderr, "%s: ERROR: Invalid sdoConfig idx %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->index = tmp;
      continue;
    }

    // parse subIdx
    if (strcmp(name, "subIdx") == 0) {
      if (strcasecmp(val, "complete") == 0) {
        p->subindex = EMCEC_CONF_SDO_COMPLETE_SUBIDX;
        continue;
      }
      tmp = strtol(val, NULL, 16);
      if (tmp < 0 || tmp >= 0xff) {
        fprintf(stderr, "%s: ERROR: Invalid sdoConfig subIdx %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->subindex = tmp;
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid sdoConfig attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }

  // idx is required
  if (p->index == 0xffff) {
    fprintf(stderr, "%s: ERROR: sdoConfig has no idx attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  // subIdx is required
  if (p->subindex == 0xff) {
    fprintf(stderr, "%s: ERROR: sdoConfig has no subIdx attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  currSdoConf = p;
  currSlave->sdoConfigLength += sizeof(EMCEC_CONF_SDOCONF_T);
}

void parseSdoDataRawAttrs(const char **attr) {
  int len;
  uint8_t *p;

  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse data
    if (strcmp(name, "data") == 0) {
      len = parseHexdump(val, NULL);
      if (len < 0) {
        fprintf(stderr, "%s: ERROR: Invalid sdoDataRaw data\n", modname);
        XML_StopParser(parser, 0);
        return;
      }
      if (len > 0) {
        p = (uint8_t *) getOutputBuffer(len);
        if (p != NULL) {
          parseHexdump(val, p);
          currSdoConf->length += len;
          currSlave->sdoConfigLength += len;
        }
      } 
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid pdoEntry attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }
}

void parseSyncManagerAttrs(const char **attr) {
  int tmp;
  EMCEC_CONF_SYNCMANAGER_T *p = getOutputBuffer(sizeof(EMCEC_CONF_SYNCMANAGER_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypeSyncManager;
  p->index = 0xff;
  p->dir = EC_DIR_INVALID;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse index
    if (strcmp(name, "idx") == 0) {
      tmp = atoi(val);
      if (tmp < 0 || tmp >= EC_MAX_SYNC_MANAGERS) {
        fprintf(stderr, "%s: ERROR: Invalid syncManager idx %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->index = tmp;
      continue;
    }

    // parse dir
    if (strcmp(name, "dir") == 0) {
      if (strcasecmp(val, "in") == 0) {
        p->dir = EC_DIR_INPUT;
        continue;
      }
      if (strcasecmp(val, "out") == 0) {
        p->dir = EC_DIR_OUTPUT;
        continue;
      }
      fprintf(stderr, "%s: ERROR: Invalid syncManager dir %s\n", modname, val);
      XML_StopParser(parser, 0);
      return;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid syncManager attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }

  // idx is required
  if (p->index == 0xff) {
    fprintf(stderr, "%s: ERROR: syncManager has no idx attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  // dir is required
  if (p->dir == EC_DIR_INVALID) {
    fprintf(stderr, "%s: ERROR: syncManager has no dir attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  (currSlave->syncManagerCount)++;
  currSyncManager = p;
}

void parsePdoAttrs(const char **attr) {
  int tmp;
  EMCEC_CONF_PDO_T *p = getOutputBuffer(sizeof(EMCEC_CONF_PDO_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypePdo;
  p->index = 0xffff;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse index
    if (strcmp(name, "idx") == 0) {
      tmp = strtol(val, NULL, 16);
      if (tmp < 0 || tmp >= 0xffff) {
        fprintf(stderr, "%s: ERROR: Invalid pdo idx %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->index = tmp;
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid pdo attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }

  // idx is required
  if (p->index == 0xffff) {
    fprintf(stderr, "%s: ERROR: pdo has no idx attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  (currSlave->pdoCount)++;
  (currSyncManager->pdoCount)++;
  currPdo = p;
}

void parsePdoEntryAttrs(const char **attr) {
  int tmp;
  EMCEC_CONF_PDOENTRY_T *p = getOutputBuffer(sizeof(EMCEC_CONF_PDOENTRY_T));
  if (p == NULL) {
    return;
  }

  p->confType = emcecConfTypePdoEntry;
  p->index = 0xffff;
  p->subindex = 0xff;
  while (*attr) {
    const char *name = *(attr++);
    const char *val = *(attr++);

    // parse index
    if (strcmp(name, "idx") == 0) {
      tmp = strtol(val, NULL, 16);
      if (tmp < 0 || tmp >= 0xffff) {
        fprintf(stderr, "%s: ERROR: Invalid pdoEntry idx %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->index = tmp;
      continue;
    }

    // parse subIdx
    if (strcmp(name, "subIdx") == 0) {
      tmp = strtol(val, NULL, 16);
      if (tmp < 0 || tmp >= 0xff) {
        fprintf(stderr, "%s: ERROR: Invalid pdoEntry subIdx %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->subindex = tmp;
      continue;
    }

    // parse bitLen
    if (strcmp(name, "bitLen") == 0) {
      tmp = atoi(val);
      if (tmp <= 0 || tmp > 32) {
        fprintf(stderr, "%s: ERROR: Invalid pdoEntry bitLen %d\n", modname, tmp);
        XML_StopParser(parser, 0);
        return;
      } 
      p->bitLength = tmp;
      continue;
    }

    // parse halType
    if (strcmp(name, "halType") == 0) {
      if (strcasecmp(val, "bit") == 0) {
        p->halType = HAL_BIT;
        continue;
      }
      if (strcasecmp(val, "s32") == 0) {
        p->halType = HAL_S32;
        continue;
      }
      if (strcasecmp(val, "u32") == 0) {
        p->halType = HAL_U32;
        continue;
      }
      fprintf(stderr, "%s: ERROR: Invalid pdoEntry halType %s\n", modname, val);
      XML_StopParser(parser, 0);
      return;
    }

    // parse halPin
    if (strcmp(name, "halPin") == 0) {
      strncpy(p->halPin, val, EMCEC_CONF_STR_MAXLEN);
      p->halPin[EMCEC_CONF_STR_MAXLEN - 1] = 0;
      continue;
    }

    // handle error
    fprintf(stderr, "%s: ERROR: Invalid pdoEntry attribute %s\n", modname, name);
    XML_StopParser(parser, 0);
    return;
  }

  // idx is required
  if (p->index == 0xffff) {
    fprintf(stderr, "%s: ERROR: pdoEntry has no idx attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  // subIdx is required
  if (p->subindex == 0xff) {
    fprintf(stderr, "%s: ERROR: pdoEntry has no subIdx attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  // bitLen is required
  if (p->bitLength == 0) {
    fprintf(stderr, "%s: ERROR: pdoEntry has no bitLen attribute\n", modname);
    XML_StopParser(parser, 0);
    return;
  }

  (currSlave->pdoEntryCount)++;
  if (p->halPin[0] != 0) {
    (currSlave->pdoMappingCount)++;
  }
  (currPdo->pdoEntryCount)++;
}

int parseSyncCycle(const char *nptr) {
  // chack for master period multiples
  if (*nptr == '*') {
    nptr++;
    return atoi(nptr) * currMaster->appTimePeriod;
  }

  // custom value
  return atoi(nptr);
}

