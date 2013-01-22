//#include <stdio.h>
//#include <stdlib.h>

#include <ippcore.h>
#include <ipps.h>

#define G72X_CODEC "g729"

#include "g729api.h"
#include "g729.h"

#define SLIN_FRAME_LEN  160
#define G729_FRAME_LEN  10
#define G729_SAMPLES    80 /* 10ms at 8000 hz, 160 bytes signed linear */
#define BUFFER_SAMPLES  8000

#define G72X_FRAME_LEN  G729_FRAME_LEN
#define G72X_SAMPLES    G729_SAMPLES


static int encoder_size;
static int decoder_size;
static int coder_size_scratch;

void g729_init_lib(){
#ifdef IPPCORE_STATIC_INIT
    ippStaticInit();
#endif
    apiG729Decoder_Alloc(G729A_CODEC, &decoder_size);
    apiG729Encoder_Alloc(G729A_CODEC, &encoder_size);
    apiG729Codec_ScratchMemoryAlloc(&coder_size_scratch);
}

void g729_init_coder(PVT *hEncoder, int dummy){
    struct g72x_coder_pvt *state = hEncoder;

#ifndef IPPCORE_NO_SSE
    ippSetFlushToZero(1, NULL); /* is FZM flag per-thread or not? does it matter at all? */
#endif
    state->coder = ippsMalloc_8u(encoder_size);
    state->scratch_mem = ippsMalloc_8u(coder_size_scratch);

    apiG729Encoder_InitBuff(state->coder, state->scratch_mem);
    apiG729Encoder_Init(state->coder, G729A_CODEC, G729Encode_VAD_Disabled);

    return 0;
}

void g729_release_coder(PVT *hEncoder){
    int i;
    struct g72x_coder_pvt *state = hEncoder;
    ippsFree(state->coder);
    ippsFree(state->scratch_mem);
}

void g729_init_decoder(PVT *hDecoder){
    struct g72x_coder_pvt *state = hDecoder;

#ifndef IPPCORE_NO_SSE
    ippSetFlushToZero(1, NULL);
#endif
    state->coder = ippsMalloc_8u(decoder_size);
    state->scratch_mem = ippsMalloc_8u(coder_size_scratch);

    apiG729Decoder_InitBuff(state->coder, state->scratch_mem);
    apiG729Decoder_Init(state->coder, G729A_CODEC);

    return 0;
}

void g729_release_decoder(PVT *hDecoder){
    int i;
    struct g72x_coder_pvt *state = hDecoder;
    ippsFree(state->coder);
    ippsFree(state->scratch_mem);
}
/*
 * EasyG729A_decoder
 * Description
 * Decodes a 10 bytes packed bit stream into an 80 words speech frame.
 * Syntax
 * #include “EasyG729A.h”
 * bool EasyG729A_decoder(g72x_coder_pvt hDecoder, unsigned
 * char *bitstream, short *synth_short );
 * Arguments
 * hDecoder The decoder handle returned by EasyG729A_init_decoder
 * bitstream
 * Input buffer containing packed bit-stream.
 * synth_short
 * Output buffer containing one frame of decoded 16 bits PCM
 */

void g729_coder(PVT *hEncoder, short *ddp, char *edp, int *cbret){
	// EasyG729A_encoder(*hEncoder, ddp, (unsigned char *)edp);
	int frametype;
        apiG729Encode(hEncoder->coder, ddp, edp, G729A_CODEC, &frametype);
}
static int g729_frame_type(int datalen)
{
    switch (datalen) {
        case 0: return -1;  /* erased */
     /* case 0: return 0; maybe it should be 0 - untransmitted silence? */
        case 2: return 1;  /* SID */
        case 8: return 2;  /* 729d */
        case 10: return 3; /* 729, 729a */
        case 15: return 4; /* 729e */
    }
    return 0;
}
void g729_decoder(PVT *hDecoder, short *ddp, char *edp, int plen){
	// EasyG729A_decoder(*hDecoder, (unsigned char *)edp, (short *)ddp);	
        apiG729Decode(hDecoder->coder, edp, g729_frame_type(plen), ddp);
}
