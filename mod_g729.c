/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005/2006, Anthony Minessale II <anthmct@yahoo.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthmct@yahoo.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthmct@yahoo.com>
 * Michael Jerris <mike@jerris.com>
 *
 * The g729 codec itself is not distributed with this module.
 *
 * mod_g729.c -- G.729 Codec Module
 *
 */

#include "switch.h"

SWITCH_MODULE_LOAD_FUNCTION(mod_g729_load);

#ifndef G729_PASSTHROUGH
#include "g729.h"

struct g729_context {
	struct dec_state decoder_object;
	struct cod_state encoder_object;
};
#endif

static switch_status_t switch_g729_init(switch_codec_t *codec, switch_codec_flag_t flags, const switch_codec_settings_t *codec_settings)
{
#ifdef G729_PASSTHROUGH
	codec->flags |= SWITCH_CODEC_FLAG_PASSTHROUGH;
	if (codec->fmtp_in) {
		codec->fmtp_out = switch_core_strdup(codec->memory_pool, codec->fmtp_in);
	}
	return SWITCH_STATUS_SUCCESS;
#else
	struct g729_context *context = NULL;
	int encoding, decoding;

	encoding = (flags & SWITCH_CODEC_FLAG_ENCODE);
	decoding = (flags & SWITCH_CODEC_FLAG_DECODE);

	if (!(encoding || decoding) || (!(context = switch_core_alloc(codec->memory_pool, sizeof(struct g729_context))))) {
		return SWITCH_STATUS_FALSE;
	} else {
		if (codec->fmtp_in) {
			codec->fmtp_out = switch_core_strdup(codec->memory_pool, codec->fmtp_in);
		}

		if (encoding) {
			g729_init_coder(&context->encoder_object, 0);
		}

		if (decoding) {
			g729_init_decoder(&context->decoder_object);
		}

		codec->private_info = context;

		return SWITCH_STATUS_SUCCESS;
	}
#endif
}

static switch_status_t switch_g729_destroy(switch_codec_t *codec)
{
#ifndef G729_PASSTHROUGH
        struct g729_context *context;
        context = codec->private_info;
	g729_release_coder( &(context->encoder_object));
	g729_release_decoder( &(context->decoder_object));
	codec->private_info = NULL;
#endif
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t switch_g729_encode(switch_codec_t *codec,
										  switch_codec_t *other_codec,
										  void *decoded_data,
										  uint32_t decoded_data_len,
										  uint32_t decoded_rate, void *encoded_data, uint32_t *encoded_data_len, uint32_t *encoded_rate,
										  unsigned int *flag)
{
#ifdef G729_PASSTHROUGH
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "This codec is only usable in passthrough mode!\n");
	return SWITCH_STATUS_FALSE;
#else
	struct g729_context *context = codec->private_info;
	int cbret = 0;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "g729 encode!!!\n");
	if (!context) {
		return SWITCH_STATUS_FALSE;
	}

	if (decoded_data_len % 160 == 0) {
		uint32_t new_len = 0;
		int16_t *ddp = decoded_data;
		char *edp = encoded_data;
		int x;
		int loops = (int) decoded_data_len / 160;

		for (x = 0; x < loops && new_len < *encoded_data_len; x++) {
			g729_coder(&context->encoder_object, ddp, edp, &cbret);
			edp += 10;
			ddp += 80;
			new_len += 10;
		}

		if (new_len <= *encoded_data_len) {
			*encoded_data_len = new_len;
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "buffer overflow!!! %u >= %u\n", new_len, *encoded_data_len);
			return SWITCH_STATUS_FALSE;
		}
	}
	return SWITCH_STATUS_SUCCESS;
#endif
}
// For zero data
static int16_t lost_frame[80] = { 0 };

static switch_status_t switch_g729_decode(switch_codec_t *codec,
										  switch_codec_t *other_codec,
										  void *encoded_data,
										  uint32_t encoded_data_len,
										  uint32_t encoded_rate, void *decoded_data, uint32_t *decoded_data_len, uint32_t *decoded_rate,
										  unsigned int *flag)
{
#ifdef G729_PASSTHROUGH
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "This codec is only usable in passthrough mode!\n");
	return SWITCH_STATUS_FALSE;
#else
    struct g729_context *context = codec->private_info;
    if (!context) {
        return SWITCH_STATUS_FALSE;
    }
//////////////////// Start code from * g729
    int framesize;
    int x;
    uint32_t new_len = 0;
    char *edp = encoded_data;
    short *ddp = decoded_data;

    if (encoded_data_len == 0) {  /* Native PLC interpolation */
	g729_decoder(&context->decoder_object, ddp,(unsigned char *)lost_frame, 0);
	ddp+=80; decoded_data_len=160;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "g729 zero length frame\n");
        return SWITCH_STATUS_SUCCESS;
    }

    for(x = 0; x < encoded_data_len && new_len < *decoded_data_len; x += framesize) {
        if(encoded_data_len - x < 8)
            framesize = 2;  /* SID */
        else
            framesize = 10; /* regular 729a frame */
	g729_decoder(&context->decoder_object, ddp, edp, framesize);
	ddp += 80;
	edp += framesize;
	new_len += 160;
    }
    if (new_len <= *decoded_data_len) {
        *decoded_data_len = new_len;
    } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "buffer overflow!!!\n");
        return SWITCH_STATUS_FALSE;
    }
    return SWITCH_STATUS_SUCCESS;
//////////////////// End code from * g729
#endif
}

SWITCH_MODULE_LOAD_FUNCTION(mod_g729_load)
{
	switch_codec_interface_t *codec_interface;
	int mpf = 10000, spf = 80, bpf = 160, ebpf = 10, count;

	// Init IPP library
	g729_init_lib();

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_CODEC(codec_interface, "G.729");
	for (count = 12; count > 0; count--) {
		switch_core_codec_add_implementation(pool, codec_interface,
											 SWITCH_CODEC_TYPE_AUDIO, 18, "G729", NULL, 8000, 8000, 8000,
											 mpf * count, spf * count, bpf * count, ebpf * count, 1, count * 10,
											 switch_g729_init, switch_g729_encode, switch_g729_decode, switch_g729_destroy);
	}
	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
