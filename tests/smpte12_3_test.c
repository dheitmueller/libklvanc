/*
 * Copyright (c) 2026 Kernel Labs Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <libklvanc/vanc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct klvanc_packet_smpte_12_3_s decoded;
static unsigned int callback_count;

static int on_smpte_12_3(void *opaque, struct klvanc_context_s *ctx,
			 struct klvanc_packet_smpte_12_3_s *packet)
{
	(void)opaque;
	(void)ctx;
	decoded = *packet;
	callback_count++;
	return 0;
}

static int check(int condition, const char *message)
{
	if (condition)
		return 0;
	fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}

static int decode(struct klvanc_context_s *ctx, const uint8_t payload[16])
{
	uint16_t *words = NULL;
	uint16_t word_count = 0;
	int result;

	memset(&decoded, 0, sizeof(decoded));
	callback_count = 0;
	result = klvanc_sdi_create_payload(0x61, 0x60, payload, 16, &words,
					   &word_count, 10);
	if (result)
		return result;
	result = klvanc_packet_parse(ctx, 9, words, word_count);
	free(words);
	return result;
}

static int test_120_frame_mapping(struct klvanc_context_s *ctx)
{
	/* Bitstream 3, 24 super-frames/s, N=5. At 01:02:03,
	 * super-frame 23 and frame identifier 4 produce frame 119. */
	const uint8_t payload[16] = {
		0x38, 0x08, 0x20, 0x00, 0x30, 0x00, 0x80, 0x08,
		0x28, 0x00, 0x08, 0x00, 0x10, 0x00, 0x00, 0x00,
	};
	int failures = 0;

	failures += check(decode(ctx, payload) >= 0, "120-frame packet parses");
	failures += check(callback_count == 1, "120-frame callback fires once");
	failures += check(decoded.bitstream_number == 3, "bitstream number is 3");
	failures += check(decoded.super_frame_rate == 24, "super-frame rate is 24");
	failures += check(decoded.frames_per_super_frame == 5, "N is 5");
	failures += check(decoded.hours == 1 && decoded.minutes == 2 &&
			  decoded.seconds == 3, "time address is 01:02:03");
	failures += check(decoded.super_frames == 23, "super-frame count is 23");
	failures += check(decoded.frame_identifier == 4, "frame identifier is 4");
	failures += check(decoded.frames == 119, "frame number is 119");
	failures += check(!decoded.drop_frame, "drop-frame flag is clear");
	return failures;
}

static int test_100_frame_mapping(struct klvanc_context_s *ctx)
{
	/* The 100-frame mapping carries sub-frame_1 in UDW15 bit 7.
	 * Super-frame 24 and frame identifier 3 produce frame 99. */
	const uint8_t payload[16] = {
		0x40, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x08,
		0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x80, 0x00,
	};
	int failures = 0;

	failures += check(decode(ctx, payload) >= 0, "100-frame packet parses");
	failures += check(callback_count == 1, "100-frame callback fires once");
	failures += check(decoded.super_frame_rate == 25, "super-frame rate is 25");
	failures += check(decoded.frames_per_super_frame == 4, "N is 4");
	failures += check(decoded.super_frames == 24, "super-frame count is 24");
	failures += check(decoded.sub_frame_1 == 1 && decoded.sub_frame_2 == 1,
			  "100-frame sub-frame bits are decoded");
	failures += check(decoded.frame_identifier == 3, "frame identifier is 3");
	failures += check(decoded.frames == 99, "frame number is 99");
	return failures;
}

int main(void)
{
	struct klvanc_context_s *ctx = NULL;
	struct klvanc_callbacks_s callbacks = {0};
	int failures;

	if (klvanc_context_create(&ctx) < 0 || !ctx) {
		fprintf(stderr, "FAIL: unable to create libklvanc context\n");
		return 1;
	}
	callbacks.smpte_12_3 = on_smpte_12_3;
	ctx->callbacks = &callbacks;
	failures = test_120_frame_mapping(ctx) + test_100_frame_mapping(ctx);
	klvanc_context_destroy(ctx);
	if (failures) {
		fprintf(stderr, "SMPTE ST 12-3: %d failure(s)\n", failures);
		return 1;
	}
	printf("SMPTE ST 12-3 decoder tests passed\n");
	return 0;
}
