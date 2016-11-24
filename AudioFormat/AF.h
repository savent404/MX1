#ifndef _AF_H_
#define _AF_H_

/* This port will deal with ".wav" format */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(4)
struct _AF_PCM {
    /* Chunk block
       should be "RIFF" */
    char ChunkID[4];

    /* Chunk Size
       Size without ChunkID and ChunkSize */
    int32_t ChunkSize;

    char    Format[4];

    char    Subchunk1ID[4];

    int32_t Subchunk1Size;

    /* Format, if it's PCM Format,
       should be 0x0001 */
    int16_t AudioFormat;

    int16_t NumChannels;

    int32_t SampleRate;

    int32_t ByteRate;

    int16_t BlockAlign;

    int16_t BitsPerSample;
};

#ifdef __cplusplus
}
#endif

#endif
