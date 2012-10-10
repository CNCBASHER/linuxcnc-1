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

  // analog in, 2ch, 16 bits
  { "EL3102", emcecSlaveTypeEL3102 },
  { "EL3112", emcecSlaveTypeEL3112 },
  { "EL3122", emcecSlaveTypeEL3122 },
  { "EL3142", emcecSlaveTypeEL3142 },
  { "EL3152", emcecSlaveTypeEL3152 },
  { "EL3162", emcecSlaveTypeEL3162 },

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

int shmem_id;

void xml_start_handler(void *data, const char *el, const char **attr);
void xml_end_handler(void *data, const char *el);

void *getOutputBuffer(size_t len);

void parseMasterAttrs(const char **attr);
void parseSlaveAttrs(const char **attr);
void parseDcConfAttrs(const char **attr);
void parseWatchdogAttrs(const char **attr);

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
      break;
  }

  fprintf(stderr, "%s: ERROR: unexpected node %s found\n", modname, el);
  XML_StopParser(parser, 0);
}

void xml_end_handler(void *data, const char *el) {
  switch (currConfType) {
    case emcecConfTypeMaster:
      currConfType = emcecConfTypeNone;
      break;
    case emcecConfTypeSlave:
      currConfType = emcecConfTypeMaster;
      break;
    case emcecConfTypeDcConf:
    case emcecConfTypeWatchdog:
      currConfType = emcecConfTypeSlave;
      break;
    default:
      fprintf(stderr, "%s: ERROR: unexpected close tag %s found\n", modname, el);
      XML_StopParser(parser, 0);
  }
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

int parseSyncCycle(const char *nptr) {
  // chack for master period multiples
  if (*nptr == '*') {
    nptr++;
    return atoi(nptr) * currMaster->appTimePeriod;
  }

  // custom value
  return atoi(nptr);
}

