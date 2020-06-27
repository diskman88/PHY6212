/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    short	valprev;	/* Previous output value */
    char	index;		/* Index into stepsize table */
    uint16_t seq_id;    /* 传输桢id */
}adpcm_state;


int ima_adpcm_encoder(short *p_pcm_data, char *p_adpcm_data, int len, adpcm_state *state);

int ima_adpcm_decoder(char *p_adpcm_data, short *p_pcm_data, int len, adpcm_state *state);
