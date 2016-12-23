#include "tx_cfg.h"

const char name_string[][10] = {
    "Vol",
    "Bank",
    "Tpon",
    "Tpoff",
    "Tout",
    "Tin",
    "Ts_switch",
    "Tautoin",
    "Tautooff",
    "Tmute",
    "TLcolor",
    "TBfreeze",
    "TCfreeze",
    "TCflip",
    "Ccount",
    "TDfreeze",
    "TDflip",
    "Dcount",
    "TEtrigger",
    "Ecycle",
    "TLon",
    "TLOff",
    "Lbright",
    "Lmode",
    "memsHz",
    "memsA",
    "memsThr",
    "memsGap",
    "Sl",
    "Sh",
    "Cl",
    "Ch",
};

void TX_CFG(struct config *cfg) {
    
    char Lbuf[20];
    char name[10], val[10];
    int  pos = 0;
    #if LINUX_TEST
    int fd;
    fd = open(DEFAULT_TEXT_PATH, O_RDONLY);
    if (fd <= 0) {
        return;
    }
    while (TextLine(fd, Lbuf) != 0) {
    #else
    FIL fp;
    if (f_open(&fp, DEFAULT_TEXT_PATH, FA_READ) != FR_OK) {
        return;
    }
    while (TextLine(&fp, Lbuf) != 0) {
    #endif
        TextNameVal(Lbuf, name, val);
        for (pos = 0; pos < VAR_NUM; pos++) {
            if (strcasecmp(name, name_string[pos]))
            continue;
            else break;
        }
        // match
        switch(pos) {
            // val
            case 0: cfg->Vol = atoi(val); break;
            case 1: cfg->Bank = atoi(val); break;
            case 2: cfg->Tpon = atoi(val); break;
            case 3: cfg->Tpoff = atoi(val); break;
            case 4: cfg->Tout = atoi(val); break;
            case 5: cfg->Tin = atoi(val); break;
            case 6: cfg->Ts_switch = atoi(val); break;
            case 7: cfg->Tautoin = atoi(val); break;
            case 8: cfg->Tautooff = atoi(val); break;
            case 9: cfg->Tmute = atoi(val); break;
            case 10: cfg->TLcolor = atoi(val); break;
            case 11: cfg->TBfreeze = atoi(val); break;
            case 12: cfg->TCfreeze = atoi(val); break;
            case 13: cfg->TCflip = atoi(val); break;
            case 14: cfg->Ccount = atoi(val); break;
            case 15: cfg->TDfreeze = atoi(val); break;
            case 16: cfg->TDflip = atoi(val); break;
            case 17: cfg->Dcount = atoi(val); break;
            case 18: cfg->TEtrigger = atoi(val); break;
            case 19: cfg->Ecycle = atoi(val); break;
            case 20: cfg->TLon = atoi(val); break;
            case 21: cfg->TLoff = atoi(val); break;
            case 22: cfg->Lbright = atoi(val); break;
            case 23: cfg->Lmode = atoi(val); break;
            case 24: cfg->memsHz = atoi(val); break;
            case 25: cfg->memsA = atoi(val); break;
            case 26: cfg->memsThr = atoi(val); break;
            case 27: cfg->memsGap = atoi(val); break;
            case 28: cfg->S1 = atoi(val); break;
            case 29: cfg->Sh = atoi(val); break;
            case 30: cfg->Cl = atoi(val); break;
            case 31: cfg->Ch = atoi(val); break;
        }
    }
}
#if LINUX_TEST
static int TextLine(int fd, char *des) {
    char chara[1];
    int num = 1;
    read(fd, chara, 1);
    while (*chara != '\n') {
        *des++ = *chara;
        read(fd, chara, 1);
        num += 1;
    }
    *des = '\0';
    return num;
}
#else
static int TextLine(FIL *pt, char *des) {
    char chara[1];
    UINT cnt = 0;
    int num = 1;
    f_read(pt, chara, 1, &cnt);
    if (cnt <= 0)
        return 0;
    while (*chara != '\n' && cnt > 0) {
        *des++ = *chara;
        f_read(pt, chara, 1, &cnt);
        num += 1;
    }
    *des = '\0';
    return num;
}
#endif

static void TextNameVal(char *line, char *name, char *val) {
    while (*line != '=') {
        //ignore ' ' & '\t'
        if (*line == ' ' || *line == '\t') {
            line += 1;
            continue;
        }
        //Error, can't find name-val
        if (*line == '\0') {
            *val = 0;
            *name = 0;
            return;
        }
        //copy to name
        *name++ = *line++;
    }

    //offset
    line += 1;

    while (*line != '\0') {
        //ignore ' ' & '\t'
        if (*line == ' ' || *line == '\t') {
            line += 1;
            continue;
        }
        //
        if (*line == '/' && *(line + 1) == '/') {
            *val = 0;
            *name = 0;
            return;
        }
        //copy
        *val++ = *line++;
    }
    *val = 0;
    *name = 0;
}
