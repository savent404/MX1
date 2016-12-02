#include "tx_cfg.h"

const char name_string[][10] = {
    "vol",
};

void TX_CFG(struct config *cfg) {
    FIL fp;
    char Lbuf[20];
    char name[10], val[10];
    int  pos = 0;
    if (f_open(&fp, DEFAULT_TEXT_PATH, FA_READ) != FR_OK) {
        return;
    }
    while (TextLine(&fp, Lbuf) != 0) {
        TextNameVal(Lbuf, name, val);
        for (pos = 0; pos < VAR_NUM; pos++) {
            if (strcasecmp(name, name_string[pos]))
            continue;
            else break;
        }
        // match
        switch(pos) {
            // val
            case 0: cfg->vol = atoi(val); break;
            /*
            case VAR_NUM - 1: berak;
            */
            default:
        }
    }
}

static int TextLine(FIL *pt, char *des) {
    char chara[1];
    UINT cnt = 0;
    int num = 1;
    f_read(pt, chara, 1, &cnt)
    if (cnt <= 0)
        return 0;
    while (*chara != '\n' && cnt >= 0) {
        *des++ = *chara;
        f_read(pt, chara, 1, &cnt);
        num += 1;
    }
    *des = '\0';
    return num;
}

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
