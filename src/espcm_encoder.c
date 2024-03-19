
#include "types.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sample.h>
#include <errors.h>
#include <errno.h>
#include <bit_pack_unpack.h>
#include <wav.h>

#define SAMPLES_PER_BLOCK 19

/* Upper half only used for ESPCM_3 mode. */
/* TODO: Extract actual table (or exact ranges + range interpolation algo, whatever it is) from chip, someday, somehow.
 * This current table is part software reverse engineering, part guesswork/extrapolation.
 * It's close enough to what's in the chip to produce acceptable results, but not exact.
 **/
int8_t espcm_range_map[512] = {
       -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,   2,   3,   4,   5,   6,   7,
      -10,  -8,  -7,  -5,  -4,  -3,  -2,  -1,   0,   2,   3,   4,   5,   6,   8,   9,
      -12, -11,  -9,  -8,  -6,  -5,  -3,  -2,   0,   2,   3,   5,   6,   8,  10,  11,
      -14, -12, -11,  -9,  -7,  -5,  -4,  -2,   0,   2,   4,   5,   7,   9,  11,  13,
      -16, -14, -12, -10,  -8,  -6,  -4,  -2,   0,   2,   4,   6,   8,  10,  12,  14,
      -21, -18, -16, -13, -11,  -8,  -6,  -3,   0,   2,   5,   7,  10,  12,  15,  18,
      -27, -24, -21, -17, -14, -11,  -8,  -4,   0,   3,   7,  10,  13,  17,  20,  24,
      -35, -28, -24, -20, -16, -12,  -8,  -4,   0,   4,   8,  12,  16,  20,  24,  28,
      -40, -35, -30, -25, -20, -15, -10,  -5,   0,   5,  10,  15,  20,  25,  30,  35,
      -48, -42, -36, -30, -24, -18, -12,  -6,   0,   6,  12,  18,  24,  30,  36,  43,
      -56, -49, -42, -35, -28, -21, -14,  -7,   0,   7,  14,  21,  28,  35,  42,  49,
      -72, -63, -54, -45, -36, -27, -18,  -9,   0,   9,  18,  27,  36,  45,  54,  63,
      -85, -74, -64, -53, -43, -32, -22, -11,   0,  11,  22,  33,  43,  54,  64,  75,
     -102, -98, -85, -71, -58, -45, -31, -14,   0,  13,  26,  39,  52,  65,  78,  90,
     -127,-112, -96, -80, -64, -48, -32, -16,   0,  16,  32,  48,  64,  80,  96, 112,
     -128,-127,-109, -91, -73, -54, -36, -18,   0,  18,  36,  54,  73,  91, 109, 127,
       -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,   2,   3,   4,   5,   6,   7,
      -10,  -9,  -8,  -6,  -5,  -4,  -3,  -2,  -1,   1,   2,   3,   4,   6,   7,   8,
      -13, -11,  -9,  -7,  -6,  -5,  -3,  -2,  -1,   2,   3,   5,   6,   7,   9,  10,
      -15, -13, -12, -10,  -8,  -6,  -5,  -3,  -1,   2,   3,   5,   6,   8,  10,  12,
      -18, -15, -13, -11,  -9,  -7,  -5,  -3,  -1,   2,   3,   5,   7,   9,  11,  13,
      -24, -20, -17, -15, -12, -10,  -7,  -5,  -2,   2,   3,   6,   8,  11,  13,  16,
      -29, -26, -23, -19, -16, -13, -10,  -6,  -2,   2,   5,   8,  11,  15,  18,  22,
      -34, -30, -26, -22, -18, -14, -10,  -6,  -2,   2,   6,  10,  14,  18,  22,  26,
      -43, -38, -33, -28, -23, -18, -13,  -8,  -3,   2,   7,  12,  17,  22,  27,  32,
      -51, -45, -39, -33, -27, -21, -15,  -9,  -3,   3,   9,  15,  21,  27,  33,  39,
      -60, -53, -46, -39, -32, -25, -18, -11,  -4,   3,  10,  17,  24,  31,  38,  45,
      -77, -68, -59, -50, -41, -32, -23, -14,  -5,   4,  13,  22,  31,  40,  49,  58,
      -90, -80, -69, -59, -48, -38, -27, -17,  -6,   5,  16,  27,  38,  48,  59,  69,
     -112,-104, -91, -78, -65, -52, -38, -23,  -7,   6,  19,  32,  45,  58,  71,  84,
     -128,-120,-104, -88, -72, -56, -40, -24,  -8,   8,  24,  40,  56,  72,  88, 104,
     -128,-128,-118,-100, -82, -64, -45, -27,  -9,   9,  27,  45,  63,  82, 100, 118
};

/* address = table_index(9:8) | dsp->espcm_last_value(7:3) | codeword(2:0)
 * the value is a base index into espcm_range_map with bits at (8, 3:0),
 * to be OR'ed with dsp->espcm_range at (7:4)
 */
uint16_t espcm3_dpcm_tables[1024] =
{
    /* Table 0 */
     256, 257, 258, 259, 260, 263, 266, 269,   0, 257, 258, 259, 260, 263, 266, 269,
       0,   1, 258, 259, 260, 263, 266, 269,   1,   2, 259, 260, 261, 263, 266, 269,
       1,   3, 260, 261, 262, 264, 266, 269,   1,   3,   4, 261, 262, 264, 266, 269,
       2,   4,   5, 262, 263, 264, 266, 269,   2,   4,   6, 263, 264, 265, 267, 269,
       2,   4,   6,   7, 264, 265, 267, 269,   2,   5,   7,   8, 265, 266, 267, 269,
       2,   5,   7,   8,   9, 266, 268, 270,   2,   5,   7,   9,  10, 267, 268, 270,
       2,   5,   8,  10,  11, 268, 269, 270,   2,   5,   8,  11,  12, 269, 270, 271,
       2,   5,   8,  11,  12,  13, 270, 271,   2,   5,   8,  11,  12,  13,  14, 271,
       0, 257, 258, 259, 260, 263, 266, 269,   0,   1, 258, 259, 260, 263, 266, 269,
       0,   1,   2, 259, 260, 263, 266, 269,   1,   2,   3, 260, 261, 263, 266, 269,
       1,   3,   4, 261, 262, 264, 266, 269,   1,   3,   5, 262, 263, 264, 266, 269,
       2,   4,   5,   6, 263, 264, 266, 269,   2,   4,   6,   7, 264, 265, 267, 269,
       2,   4,   6,   7,   8, 265, 267, 269,   2,   5,   7,   8,   9, 266, 267, 269,
       2,   5,   7,   9,  10, 267, 268, 270,   2,   5,   7,   9,  10,  11, 268, 270,
       2,   5,   8,  10,  11,  12, 269, 270,   2,   5,   8,  11,  12,  13, 270, 271,
       2,   5,   8,  11,  12,  13,  14, 271,   2,   5,   8,  11,  12,  13,  14,  15,
    /* Table 1 */
     257, 260, 262, 263, 264, 265, 267, 270, 257, 260, 262, 263, 264, 265, 267, 270,
       1, 260, 262, 263, 264, 265, 267, 270,   1, 260, 262, 263, 264, 265, 267, 270,
       1, 260, 262, 263, 264, 265, 267, 270,   1,   4, 262, 263, 264, 265, 267, 270,
       1,   4, 262, 263, 264, 265, 267, 270,   1,   4,   6, 263, 264, 265, 267, 270,
       1,   4,   6,   7, 264, 265, 267, 270,   1,   4,   6,   7,   8, 265, 267, 270,
       1,   4,   6,   7,   8,   9, 267, 270,   1,   4,   6,   7,   8,   9, 267, 270,
       1,   4,   6,   7,   8,   9,  11, 270,   1,   4,   6,   7,   8,   9,  11, 270,
       1,   4,   6,   7,   8,   9,  11, 270,   1,   4,   6,   7,   8,   9,  11,  14,
     257, 260, 262, 263, 264, 265, 267, 270,   1, 260, 262, 263, 264, 265, 267, 270,
       1, 260, 262, 263, 264, 265, 267, 270,   1, 260, 262, 263, 264, 265, 267, 270,
       1,   4, 262, 263, 264, 265, 267, 270,   1,   4, 262, 263, 264, 265, 267, 270,
       1,   4,   6, 263, 264, 265, 267, 270,   1,   4,   6,   7, 264, 265, 267, 270,
       1,   4,   6,   7,   8, 265, 267, 270,   1,   4,   6,   7,   8,   9, 267, 270,
       1,   4,   6,   7,   8,   9, 267, 270,   1,   4,   6,   7,   8,   9,  11, 270,
       1,   4,   6,   7,   8,   9,  11, 270,   1,   4,   6,   7,   8,   9,  11, 270,
       1,   4,   6,   7,   8,   9,  11,  14,   1,   4,   6,   7,   8,   9,  11,  14,
    /* Table 2 */
     256, 257, 258, 259, 260, 262, 265, 268,   0, 257, 258, 259, 260, 262, 265, 268,
       0,   1, 258, 259, 260, 262, 265, 269,   1,   2, 259, 260, 261, 263, 265, 269,
       1,   3, 260, 261, 262, 263, 265, 269,   1,   3,   4, 261, 262, 263, 265, 269,
       1,   3,   5, 262, 263, 264, 266, 269,   1,   4,   5,   6, 263, 264, 266, 269,
       1,   4,   6,   7, 264, 265, 266, 269,   1,   4,   6,   7,   8, 265, 266, 269,
       2,   4,   6,   7,   8,   9, 267, 269,   2,   4,   6,   7,   8,   9, 267, 269,
       2,   5,   7,   8,   9,  10,  11, 270,   2,   5,   7,   8,   9,  10,  11, 270,
       2,   5,   8,   9,  10,  11,  12, 270,   2,   6,   8,  10,  11,  12,  13,  14,
     257, 258, 259, 260, 261, 263, 265, 269,   1, 259, 260, 261, 262, 263, 266, 269,
       1, 260, 261, 262, 263, 264, 266, 269,   1, 260, 261, 262, 263, 264, 266, 269,
       2,   4, 262, 263, 264, 265, 267, 269,   2,   4, 262, 263, 264, 265, 267, 269,
       2,   5,   6, 263, 264, 265, 267, 270,   2,   5,   6,   7, 264, 265, 267, 270,
       2,   5,   7,   8, 265, 266, 267, 270,   2,   5,   7,   8,   9, 266, 268, 270,
       2,   6,   8,   9,  10, 267, 268, 270,   2,   6,   8,   9,  10,  11, 268, 270,
       2,   6,   8,  10,  11,  12, 269, 270,   2,   6,   9,  11,  12,  13, 270, 271,
       3,   6,   9,  11,  12,  13,  14, 271,   3,   6,   9,  11,  12,  13,  14,  15,
    /* Table 3 */
     256, 258, 260, 261, 262, 263, 264, 265,   0, 258, 260, 261, 262, 263, 264, 265,
       1, 259, 260, 261, 262, 263, 264, 266,   1, 259, 260, 261, 262, 263, 264, 266,
       1,   3, 260, 261, 262, 263, 264, 266,   1,   3,   4, 261, 262, 263, 264, 267,
       1,   3,   4,   5, 262, 263, 264, 267,   1,   3,   4,   5,   6, 263, 264, 267,
       1,   3,   5,   6,   7, 264, 265, 268,   1,   3,   5,   6,   7,   8, 265, 268,
       1,   4,   6,   7,   8,   9, 266, 269,   1,   4,   6,   7,   8,   9,  10, 269,
       1,   4,   6,   7,   8,   9,  10, 269,   1,   4,   6,   7,   8,   9,  11, 270,
       1,   4,   6,   7,   8,   9,  11, 270,   1,   4,   6,   7,   8,   9,  11,  14,
     257, 260, 262, 263, 264, 265, 267, 270,   1, 260, 262, 263, 264, 265, 267, 270,
       1, 260, 262, 263, 264, 265, 267, 270,   2, 261, 262, 263, 264, 265, 267, 270,
       2, 261, 262, 263, 264, 265, 267, 270,   2,   5, 262, 263, 264, 265, 267, 270,
       3,   6, 263, 264, 265, 266, 268, 270,   3,   6,   7, 264, 265, 266, 268, 270,
       4,   7,   8, 265, 266, 267, 268, 270,   4,   7,   8,   9, 266, 267, 268, 270,
       4,   7,   8,   9,  10, 267, 268, 270,   5,   7,   8,   9,  10,  11, 268, 270,
       5,   7,   8,   9,  10,  11,  12, 270,   5,   7,   8,   9,  10,  11,  12, 270,
       6,   7,   8,   9,  10,  11,  13, 271,   6,   7,   8,   9,  10,  11,  13,  15
};

void
quantize (int8_t *samples, codeword_t *out, codeword_t *range_out)
{
	int32_t i, table_addr, sigma, last_sigma;
	int16_t min_sample = 127, max_sample = -128, s;
	uint8_t range;

	for (i = 0; i < SAMPLES_PER_BLOCK; i++)
	{
		s = samples[i];
		if (s < min_sample)
		{
			min_sample = s;
		}
		if (s > max_sample)
		{
			max_sample = s;
		}
	}
	if (min_sample < 0)
	{
		min_sample = -min_sample;
	}
	if (max_sample < 0)
	{
		max_sample = -max_sample;
	}
	if (min_sample > max_sample)
	{
		max_sample = min_sample;
	}

	for (table_addr = 15; table_addr < 256; table_addr += 16)
	{
		if (max_sample <= espcm_range_map[table_addr])
		{
			break;
		}
	}
	if (table_addr > 256)
	{
		range = 0x0F;
	}
	else
	{
		range = table_addr >> 4;
	}
	*range_out = range;

	for (i = 0; i < SAMPLES_PER_BLOCK; i++)
	{
		table_addr = range << 4;
		last_sigma = 9999;
		s = samples[i];
		for (; (table_addr >> 4) == range; table_addr++)
		{
			sigma = espcm_range_map[table_addr] - s;
			if (sigma < 0)
			{
				sigma = -sigma;
			}
			if (sigma > last_sigma)
			{
				break;
			}
			last_sigma = sigma;
		}
		table_addr--;
		out[i] = table_addr & 0x0F;
	}
}

void
dequantize (codeword_t *input, int8_t *samples_out, codeword_t range)
{
	int i;
	int16_t address;
	for (i = 0; i < SAMPLES_PER_BLOCK; i++)
	{
		address = (input[i] & 0x0F) | ((input[i] & 0x10) << 4) | range << 4;
		samples_out[i] = espcm_range_map[address];
	}
}

uint64_t
dpcm_encode (int8_t *input, codeword_t *delta_out, codeword_t range, codeword_t initial_sample, int table_index)
{
	codeword_t cur_sample = initial_sample, best_delta = 0;
	int i, base_address, offset;
	uint64_t sigma = 0, local_sigma;
	
	for (i = 0; i < SAMPLES_PER_BLOCK - 1; i++)
	{
		local_sigma = 9999;
		base_address = ((table_index & 0x03) << 8) | ((cur_sample & 0x0F) << 3) | ((cur_sample & 0x100) >> 1);
		for (offset = 0; offset < 8; offset++)
		{
			int64_t diff = espcm_range_map[espcm3_dpcm_tables[base_address | offset] | (range << 4)];
			diff -= input[i + 1];
			if (diff < 0)
			{
				diff = -diff;
			}
			if ((uint64_t)diff < local_sigma)
			{
				best_delta = offset;
				local_sigma = diff;
			}
		}
		delta_out[i] = best_delta;
		sigma += local_sigma * local_sigma;
		cur_sample = espcm3_dpcm_tables[base_address + best_delta];
	}
	
	return sigma;
}

void
dpcm_decode(codeword_t *input, codeword_t *code_out, int table_index)
{
	uint16_t cur_sample = code_out[0];
	int i, base_address, offset;
	
	for (i = 0; i < SAMPLES_PER_BLOCK - 1; i++)
	{
		base_address = ((table_index & 0x03) << 8) | ((cur_sample & 0x0F) << 3) | ((cur_sample & 0x100) >> 1);
		offset = input[i];
		cur_sample = espcm3_dpcm_tables[base_address + offset];
		code_out[i + 1] = (cur_sample & 0x0F) | ((cur_sample & 0x100) >> 4);
	}
}

void
exit_error (const char *msg, const char *error)
{
	if (error == NULL)
	{
		fprintf(stderr, "\nOof!\n%s\n", msg);
	}
	else
	{
		fprintf(stderr, "\nOof!\n%s: %s\n", msg, error);
	}
	exit(1);
}

static const char usage[] = "\
\033[97mUsage:\033[0m encoder (mode) infile.wav outfile.aud\n\
- Parameters\n\
  - \033[96mmode\033[0m - Selects the encoding mode; the following modes are\n\
    supported (in increasing order of bitrate):\n\
    - \033[96mespcm1\033[0m  - 1-bit ESPCM\n\
    - \033[96mespcm3\033[0m  - 3-bit ESPCM\n\
    - \033[96mespcm4\033[0m  - 4-bit ESPCM\n\
    - \033[96mdecode\033[0m - decodes an encoded input file\n\
- \033[96minfile.wav\033[0m should be an 8-bit unsigned PCM or 16-bit signed\n\
  PCM WAV file for the encoding modes, or an encoded .aud ESPCM file for the\n\
  decode mode.\n\
- \033[96moutfile.aud\033[0m is the path for the encoded output file, or the\n\
  decoded WAV file in the case of the decode mode.";

int
main (int argc, char **argv)
{
	wav_handle *infile;
	wav_handle *outfile;
	
	size_t block_count = 0;
	
	char *infile_name;
	char *outfile_name;
	wav_sample_fmt format;
	wav_sample_fmt output_format;
	uint32_t sample_rate;
	espcm_block_mode mode;
	int num_channels;
	int i;
	bool decode_mode = false;
	
	uint8_t code_buffer[SAMPLES_PER_BLOCK];
	uint8_t bs_buffer[10];
	uint8_t sample_conv_buffer[SAMPLES_PER_BLOCK * 2];
	uint8_t sample_conv_buffer_2[SAMPLES_PER_BLOCK * 2];
	bitstream_buffer bitpacker;
	int8_t sample_buffer[SAMPLES_PER_BLOCK];
	codeword_t delta_buffer[(SAMPLES_PER_BLOCK - 1) * 4];
	codeword_t range;
	err_t err;
	
	if (argc != 4)
	{
		exit_error(usage, NULL);
	}

	if (!strcmp("espcm4", argv[1]))
	{
		mode = ESS_ESPCM4;
	}
	else if (!strcmp("espcm3", argv[1]))
	{
		mode = ESS_ESPCM3;
	}
	else if (!strcmp("espcm1", argv[1]))
	{
		mode = ESS_ESPCM1;
	}
	else if (!strcmp("decode", argv[1]))
	{
		decode_mode = true;
	}
	else
	{
		mode = 0;
		exit_error(usage, NULL);
	}
	
	infile_name = argv[2];
	outfile_name = argv[3];
	
	infile = wav_alloc(&err);
	outfile = wav_alloc(&err);
	
	infile = wav_open(infile, infile_name, W_READ, &err);
	if (infile == NULL)
	{
		char err_msg[256];
		snprintf(err_msg, 256, "Could not open input file (%s). errno", error_enum_strs[err]);
		exit_error(err_msg, strerror(errno));
	}
	
	format = wav_get_format(infile, &err);
	sample_rate = wav_get_sample_rate(infile);
	num_channels = wav_get_num_channels(infile, &err);
	if (num_channels > 1)
	{
		char err_msg[256];
		snprintf(err_msg, 256, "Input file has %d channels, expected a mono WAV file", num_channels);
		exit_error(err_msg, NULL);
	}
	
	switch (format)
	{
	case W_U8:
	case W_S16LE:
		if (decode_mode)
		{
			exit_error("Input file appears to be an uncompressed WAV, not ESPCM - please use one of the encoding modes instead", NULL);
		}
		else
		{
			output_format = W_ESPCM;
		}
		break;
	case W_ESPCM:
		if (!decode_mode)
		{
			exit_error("Input file appears to be ESPCM, not uncompressed WAV - please use the decode mode instead", NULL);
		}
		else
		{
			output_format = W_U8;
		}
		break;
	case W_SSDPCM:
		exit_error("Input file is not an ESPCM .aud file (signature matches \"SSDPCM\" format)", NULL);
		break;
	default:
		exit_error("Input file has unrecognized format/codec - only 8/16-bit PCM is allowed", NULL);
		break;
	}
	
	outfile = wav_open(outfile, outfile_name, W_CREATE, &err);
	if (outfile == NULL)
	{
		char err_msg[256];
		snprintf(err_msg, 256, "Could not open output file (%s). errno", error_enum_strs[err]);
		exit_error(err_msg, strerror(errno));
	}
	
	if (decode_mode)
	{
		wav_set_format(outfile, output_format);
		wav_set_sample_rate(outfile, sample_rate);
		mode = wav_get_espcm_mode(infile, &err);
		if (err != E_OK)
		{
			exit_error("wav_get_espcm_mode returned error", error_enum_strs[err]);
		}
	}
	else
	{
		wav_init_espcm(outfile, mode, sample_rate);
	}
	
	memset(&bitpacker, 0, sizeof(bitstream_buffer));
	bitpacker.byte_buf.buffer = bs_buffer;
	bitpacker.byte_buf.buffer_size = 10;
	
	wav_write_header(outfile);
	wav_seek(infile, 0, SEEK_SET);
	wav_seek(outfile, 0, SEEK_SET);

	while (err == E_OK)
	{
		if (!decode_mode)
		{
			wav_read(infile, sample_conv_buffer, SAMPLES_PER_BLOCK, &err);
			if (err != E_OK)
			{
				if (err == E_END_OF_STREAM)
				{
					break;
				}
				char err_msg[256];
				int errno_copy = errno;
				snprintf(err_msg, 256, "Read error (%s)", error_enum_strs[err]);
				// Try to properly close the WAV file anyway
				wav_close(outfile, &err);
				exit_error(err_msg, strerror(errno_copy));
			}
			
			switch (format)
			{
			case W_U8:
				memcpy(sample_conv_buffer_2, sample_conv_buffer, SAMPLES_PER_BLOCK);
				break;
			case W_S16LE:
				sample_convert_s16_to_u8(sample_conv_buffer_2, (int16_t *)sample_conv_buffer, SAMPLES_PER_BLOCK);
				break;
			default:
				// unreachable
				break;
			}
			sample_convert_u8_to_s8(sample_buffer, sample_conv_buffer_2, SAMPLES_PER_BLOCK);
			
			fprintf(stderr, "\rEncoding block %lu...", block_count);
			quantize(sample_buffer, code_buffer, &range);
			
			bitpacker.bit_index = 0;
			bitpacker.byte_buf.offset = 0;
			memset(bs_buffer, 0, sizeof(bs_buffer));
			
			put_bits_lsbfirst(&bitpacker, range, 4);
			switch (mode)
			{
			case ESS_ESPCM4:
				for (i = 0; i < 19; i++)
				{
					put_bits_lsbfirst(&bitpacker, code_buffer[i], 4);
				}
				break;
			case ESS_ESPCM1:
				put_bits_lsbfirst(&bitpacker, 0, 1);
				for (i = 0; i < 19; i++)
				{
					put_bits_lsbfirst(&bitpacker, code_buffer[i] >> 3, 1);
				}
				break;
			case ESS_ESPCM3:
				put_bits_lsbfirst(&bitpacker, code_buffer[0], 4);
				int best_table = 0;
				uint64_t sigma, lowest_sigma = UINT64_MAX;
				for (i = 0; i < 4; i++)
				{
					sigma = dpcm_encode(sample_buffer, delta_buffer + (SAMPLES_PER_BLOCK - 1) * i, range, code_buffer[0], i);
					if (sigma < lowest_sigma)
					{
						best_table = i;
						lowest_sigma = sigma;
					}
				}
				put_bits_lsbfirst(&bitpacker, best_table, 2);
				for (i = 0; i < SAMPLES_PER_BLOCK - 1; i++)
				{
					put_bits_lsbfirst(&bitpacker, (delta_buffer + (SAMPLES_PER_BLOCK - 1) * best_table)[i], 3);
				}
				break;
			default:
				// unreachable
				break;
			}
			
			wav_write(outfile, bs_buffer, 1, -1, &err);
			if (err != E_OK)
			{
				char err_msg[256];
				int errno_copy = errno;
				snprintf(err_msg, 256, "Write error (%s)", error_enum_strs[err]);
				// Try to properly close the WAV file anyway
				wav_close(outfile, &err);
				exit_error(err_msg, strerror(errno_copy));
			}
		}
		
		if (decode_mode)
		{
			wav_read(infile, bs_buffer, 1, &err);
			if (err != E_OK)
			{
				if (err == E_END_OF_STREAM)
				{
					break;
				}
				char err_msg[256];
				int errno_copy = errno;
				snprintf(err_msg, 256, "Read error (%s)", error_enum_strs[err]);
				// Try to properly close the WAV file anyway
				wav_close(outfile, &err);
				exit_error(err_msg, strerror(errno_copy));
			}
			
			bitpacker.bit_index = 0;
			bitpacker.byte_buf.offset = 0;
			
			fprintf(stderr, "\rDecoding block %lu...", block_count);
			
			get_bits_lsbfirst(&range, &bitpacker, 4);
			switch (mode)
			{
			case ESS_ESPCM4:
				for (i = 0; i < 19; i++)
				{
					get_bits_lsbfirst(&code_buffer[i], &bitpacker, 4);
				}
				break;
			case ESS_ESPCM1:
				get_bits_lsbfirst(NULL, &bitpacker, 1);
				for (i = 0; i < 19; i++)
				{
					get_bits_lsbfirst(&code_buffer[i], &bitpacker, 1);
					code_buffer[i] = code_buffer[i] ? 0x0C : 0x04;
				}
				break;
			case ESS_ESPCM3:
				get_bits_lsbfirst(&code_buffer[0], &bitpacker, 4);
				codeword_t table_index;
				get_bits_lsbfirst(&table_index, &bitpacker, 2);
				for (i = 0; i < 18; i++)
				{
					get_bits_lsbfirst(&delta_buffer[i], &bitpacker, 3);
				}
				dpcm_decode(delta_buffer, code_buffer, table_index);
				break;
			default:
				// unreachable
				break;
			}
			
			dequantize(code_buffer, sample_buffer, range);
			sample_convert_s8_to_u8(sample_conv_buffer, sample_buffer, SAMPLES_PER_BLOCK);
			
			switch (output_format)
			{
			case W_U8:
				memcpy(sample_conv_buffer_2, sample_conv_buffer, SAMPLES_PER_BLOCK);
				break;
			case W_S16LE:
				sample_convert_u8_to_s16((int16_t *)sample_conv_buffer_2, sample_conv_buffer, SAMPLES_PER_BLOCK);
				break;
			default:
				// unreachable
				break;
			}
			
			wav_write(outfile, sample_conv_buffer_2, SAMPLES_PER_BLOCK, -1, &err);
			if (err != E_OK)
			{
				char err_msg[256];
				int errno_copy = errno;
				snprintf(err_msg, 256, "Write error (%s)", error_enum_strs[err]);
				// Try to properly close the WAV file anyway
				wav_close(outfile, &err);
				exit_error(err_msg, strerror(errno_copy));
			}
		}
		
		block_count++;
	}
	
	wav_close(infile, &err);
	wav_close(outfile, &err);
	
	fprintf(stderr, "\nDone.\n");

	free(infile);
	free(outfile);
	
	return 0;
}
