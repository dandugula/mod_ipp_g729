/* dujinfang@gmail.com
 * wrapper for C++ lib of EasyG729A
 * make compatible with freeswitch mod_g729
 */

#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SAMPLES  8000

struct g72x_coder_pvt {
    void *coder;
    void *scratch_mem;
    int16_t buf[BUFFER_SAMPLES]; /* 1 second */
};
#define dec_state g72x_coder_pvt
#define cod_state g72x_coder_pvt
#define PVT struct g72x_coder_pvt

void g729_init_lib();
void g729_init_coder(PVT *hEncoder, int);
void g729_release_coder(PVT *hEncoder);
void g729_init_decoder(PVT *hDecoder);
void g729_release_decoder(PVT *hDecoder);
void g729_coder(PVT *hEncoder, short *ddp, char *edp, int *plen);
void g729_decoder(PVT *hDecoder, short *ddp, char *edp, int cbret);
