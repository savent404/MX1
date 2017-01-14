#include "tx_cfg.h"

const char name_string[][10] = {
    "Vol",       "Bank",    "Tpon",     "Tpoff",    "Tout",    "Tin",
    "Ts_switch", "Tautoin", "Tautooff", "Tmute",    "TLcolor", "TBfreeze",
    "TCfreeze",  "TCflip",  "Ccount",   "TDfreeze", "TDflip",  "Dcount",
    "TEtrigger", "Ecycle",  "TLon",     "TLOff",    "Lbright", "Lmode",
    "memsHz",    "memsA",   "memsThr",  "memsGap",  "Sl",      "Sh",
    "Cl",        "Ch",      "BANK1",    "BANK2",    "BANK3",   "BANK4",
    "BANK5",     "BANK6",   "BANK7",    "BANK8",    "BANK9",   "BANK10",
    "BANK11",    "BANK12",  "BANK13",   "BANK14",   "BANK15",  "BANK16",
    "FBANK1",    "FBANK2",  "FBANK3",   "FBANK4",   "FBANK5",  "FBANK6",
    "FBANK7",    "FBANK8",  "FBANK9",   "FBANK10",  "FBANK11", "FBANK12",
    "FBANK13",   "FBANK14", "FBANK15",  "FBANK16"};

void TX_CFG(struct config *cfg, RGBL rgbl[][2]) {
  extern FRESULT res;
  char *Lbuf;
  char name[NAME_SIZE], val[VAL_SIZE];
  static char sbuf[SBUF_SIZE];
  int pos = 0;
  FIL fp;
  if (f_open(&fp, DEFAULT_TEXT_PATH, FA_READ) != FR_OK) {
    return;
  }
  while (Lbuf = f_gets(sbuf, SBUF_SIZE, &fp), Lbuf) {
    TextNameVal(Lbuf, name, val);
    for (pos = 0; pos < VAR_NUM; pos++) {
      if (strcasecmp(name, name_string[pos]))
        continue;
      else
        break;
    }
    // match
    switch (pos) {
      // val
      case 0:
        cfg->Vol = atoi(val);
        break;
      case 1:
        cfg->Bank = atoi(val);
        break;
      case 2:
        cfg->Tpon = atoi(val);
        break;
      case 3:
        cfg->Tpoff = atoi(val);
        break;
      case 4:
        cfg->Tout = atoi(val);
        break;
      case 5:
        cfg->Tin = atoi(val);
        break;
      case 6:
        cfg->Ts_switch = atoi(val);
        break;
      case 7:
        cfg->Tautoin = atoi(val);
        break;
      case 8:
        cfg->Tautooff = atoi(val);
        break;
      case 9:
        cfg->Tmute = atoi(val);
        break;
      case 10:
        cfg->TLcolor = atoi(val);
        break;
      case 11:
        cfg->TBfreeze = atoi(val);
        break;
      case 12:
        cfg->TCfreeze = atoi(val);
        break;
      case 13:
        cfg->TCflip = atoi(val);
        break;
      case 14:
        cfg->Ccount = atoi(val);
        break;
      case 15:
        cfg->TDfreeze = atoi(val);
        break;
      case 16:
        cfg->TDflip = atoi(val);
        break;
      case 17:
        cfg->Dcount = atoi(val);
        break;
      case 18:
        cfg->TEtrigger = atoi(val);
        break;
      case 19:
        cfg->Ecycle = atoi(val);
        break;
      case 20:
        cfg->TLon = atoi(val);
        break;
      case 21:
        cfg->TLoff = atoi(val);
        break;
      case 22:
        cfg->Lbright = atoi(val);
        break;
      case 23:
        cfg->Lmode = atoi(val);
        break;
      case 24:
        cfg->memsHz = atoi(val);
        break;
      case 25:
        cfg->memsA = atoi(val);
        break;
      case 26:
        cfg->memsThr = atoi(val);
        break;
      case 27:
        cfg->memsGap = atoi(val);
        break;
      case 28:
        cfg->S1 = atoi(val);
        break;
      case 29:
        cfg->Sh = atoi(val);
        break;
      case 30:
        cfg->Cl = atoi(val);
        break;
      case 31:
        cfg->Ch = atoi(val);
        break;
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
      case 37:
      case 38:
      case 39:
      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
      case 47:
        sscanf(val, "%d,%d,%d,%d", &(rgbl[pos - 32][0].R),
               &(rgbl[pos - 32][0].G), &(rgbl[pos - 32][0].B),
               &(rgbl[pos - 32][0].L));
        break;
      case 48:
      case 49:
      case 50:
      case 51:
      case 52:
      case 53:
      case 54:
      case 55:
      case 56:
      case 57:
      case 58:
      case 59:
      case 60:
      case 61:
      case 62:
      case 63:
        sscanf(val, "%d,%d,%d,%d", &(rgbl[pos - 48][1].R),
               &(rgbl[pos - 48][1].G), &(rgbl[pos - 32][1].B),
               &(rgbl[pos - 48][1].L));
        break;
    }
  }
  res = f_close(&fp);
}

static void TextNameVal(char *line, char *name, char *val) {
  uint8_t name_cnt = 0, val_cnt = 0;
  while (*line != '=' && name_cnt < NAME_SIZE) {
    // ignore ' ' & '\t'
    if (*line == ' ' || *line == '\t') {
      line += 1;
      continue;
    }
    // Error, can't find name-val
    if (*line == '\0') {
      *val = 0;
      *name = 0;
      return;
    }
    // copy to name
    *name++ = *line++;
    name_cnt++;
  }

  // offset
  line += 1;

  while (*line != '\0' && val_cnt < VAL_SIZE) {
    // ignore ' ' & '\t'
    if (*line == ' ' || *line == '\t') {
      line += 1;
      continue;
    }
    //
    if (*line == '/') {
      *val = 0;
      *name = 0;
      return;
    }
    // copy
    *val++ = *line++;
    val_cnt++;
  }
  *val = 0;
  *name = 0;
}
