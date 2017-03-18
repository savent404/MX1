#include "new_tx_cfg.h"
/**
  * @Brief: Get name from a line string.
  * @Retvl: 0-Normal name, +x, RGBL[x - 1][0], -x, RGBL[-x - 1][0]
  */
static int GetName(char *line, char* name);

/**
  * @Brief: Get multi parameter like 1,2,3
  * @Retvl: Output like: 1111(4,3,2,1)
  */
static int GetMultiPara(char *line);

static const char name_string[][10] = {
    "Vol",      "Tpon",     "Tpoff",    "Tout",    "Tin",       "Ts_switch",
    "Tautoin",  "Tautooff", "Tmute",    "TLcolor", "TBfreeze",  "TBMode",
    "TCfreeze", "TCMode",   "TDfreeze", "TDMode",  "TEtrigger", "TEMode",
    "TLon",     "TLoff",    "Lbright",  "Ldeep",   "LMode",     "memsHz",
    "memsA",    "memsThr",  "memsGap",  "Sl",      "Sh",        "Cl",
    "Ch",       "MD",       "MT",       "CD",      "CT",        "CL",
    "CW"};

void TX_CFG(struct config *cfg, RGBL rgbl[][2]) {
  char name[NAME_SIZE];
  char sbuf[SBUF_SIZE];
	char *Lbuf;
  FIL fp;
  int res;
  if (f_open(&fp, DEFAULT_TEXT_PATH, FA_READ) != FR_OK) return;

  while (Lbuf = f_gets(sbuf, SBUF_SIZE, &fp), Lbuf) {
    res = GetName(Lbuf, name);
    if (res > 0) {
      sscanf(Lbuf, "%*[^=]=%d,%d,%d,%d", &rgbl[res - 1][0].R, &rgbl[res - 1][0].G,
             &rgbl[res - 1][0].B, &rgbl[res - 1][0].L);
      continue;
    } else if (res < 0) {
      sscanf(Lbuf, "%*[^=]=%d,%d,%d,%d", &rgbl[-res - 1][1].R,
             &rgbl[-res - 1][1].G, &rgbl[-res - 1][1].B, &rgbl[-res - 1][1].L);
      continue;
    }

    //   match name string
    for (res = 0; res < sizeof(name_string) / 10; res++) {
      if (!strcmp(name, name_string[res])) break;
    }
    switch (res) {
      case 0: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Vol));break;
      case 1: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tpon));break;
      case 2: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tpoff));break;
      case 3: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tout));break;
      case 4: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tin));break;
      case 5: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Ts_switch));break;
      case 6: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tautoin));break;
      case 7: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tautooff));break;
      case 8: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Tmute));break;
      case 9: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TLcolor));break;
      case 10: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TBfreeze));break;
      case 11: cfg->TBMode=GetMultiPara(Lbuf);break;
      case 12: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TCfreeze));break;
      case 13: cfg->TCMode=GetMultiPara(Lbuf);break;
      case 14: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TDfreeze));break;
      case 15: cfg->TDMode=GetMultiPara(Lbuf);break;
      case 16: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TEtrigger));break;
      case 17: cfg->TEMode=GetMultiPara(Lbuf);break;
      case 18: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TLon));break;
      case 19: sscanf(Lbuf,"%*[^=]=%d", &(cfg->TLoff));break;
      case 20: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Lbright));break;
      case 21: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Ldeep));break;
      case 22: sscanf(Lbuf,"%*[^=]=%d", &(cfg->LMode));break;
      case 23: sscanf(Lbuf,"%*[^=]=%d", &(cfg->memsHz));break;
      case 24: sscanf(Lbuf,"%*[^=]=%d", &(cfg->memsA));break;
      case 25: sscanf(Lbuf,"%*[^=]=%d", &(cfg->memsThr));break;
      case 26: sscanf(Lbuf,"%*[^=]=%d", &(cfg->memsGap));break;
      case 27: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Sl));break;
      case 28: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Sh));break;
      case 29: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Cl));break;
      case 30: sscanf(Lbuf,"%*[^=]=%d", &(cfg->Ch));break;
      case 31: sscanf(Lbuf,"%*[^=]=%d", &(cfg->MD));break;
      case 32: sscanf(Lbuf,"%*[^=]=%d", &(cfg->MT));break;
      case 33: sscanf(Lbuf,"%*[^=]=%d", &(cfg->CD));break;
      case 34: sscanf(Lbuf,"%*[^=]=%d", &(cfg->CT));break;
      case 35: sscanf(Lbuf,"%*[^=]=%d", &(cfg->CL));break;
      case 36: sscanf(Lbuf,"%*[^=]=%d", &(cfg->CW));break;
    }
  }
	f_close(&fp);
}

static int GetName(char *line, char* name) {
  int res = 0, cnt = 0;
  if (!strncasecmp(line, "BANK", 4)) {
    sscanf(line, "%*[ BANKbank]%d", &res);
    return res;
  } else if (!strncasecmp(line, "FBANK", 5)) {
    sscanf(line, "%*[ FBANKfbank]%d", &res);
    return -res;
  } else {
    while (*line != '=') {
      if (*line == ' ' || *line == '\t') {
        line += 1;
      }
      if (*line == '\0') {
        *name = 0;
        return res;
      }
			if (cnt >= NAME_SIZE) {
				*name = 0;
				return res;
			}
      *name++ = *line++;
			cnt++;
    }
    *name = 0;
    return res;
  }
}
static int GetMultiPara(char *line) {
  int res = 0;
  char *pt = line;
  while (*pt != '=') pt += 1;
  pt += 1;
  while (*pt == ' ') pt += 1;
  while (*pt != '\n' && *pt != '/' && *pt != '\0') {
    if (*pt == '1') res |= 0x01;
    if (*pt == '2') res |= 0x02;
    if (*pt == '3') res |= 0x04;
    if (*pt == '4') res |= 0x08;
    pt += 1;
  }
  return res;
}
