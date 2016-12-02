#ifndef _TX_CFG_H_
#define _TX_CFG_H_
#include <stdlib.h>
#include <string.h>
#include "ff.h"

#define DEFAULT_TEXT_PATH "0:/demo.txt"
#define VAR_NUM            1
// Default name string
extern const char name_string[][10];
struct config {
    int vol;
    /*
     .....
     */
};

void TX_CFG(struct config *cfg);
/**
  * @Brief  Get a line from pt
  * @Para   des output to des
  * @Retval should sub number
  */
static int TextLine(FIL *pt,char *des);

/**
  * @Brief   Get ans:val string
  * @Para line src-string
  * @Para name des-name-string
  * @Para val  des-val-string
  */
static void TextNameVal(char *line, char *name, char *val);
#endif