#include <types.h>
#include <errors.h>

uint8_t const bitmasks_u8[] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

static inline err_t
peek_byte (uint8_t *dest, buffer_u8 *buf)
{
	if (buf->offset < buf->buffer_size)
	{
		*dest = buf->buffer[buf->offset];
		return E_OK;
	}
	return E_END_OF_STREAM;
}

static inline err_t
poke_byte (buffer_u8 *buf, uint8_t src)
{
	if (buf->offset < buf->buffer_size)
	{
		buf->buffer[buf->offset] = src;
		return E_OK;
	}
	return E_END_OF_STREAM;
}

err_t
get_bits_msbfirst (codeword_t *dest, bitstream_buffer *buf, uint8_t num_bits)
{
	codeword_t result;
	int num_bits_missing;
	uint8_t byte;
	
	buf->bit_index += num_bits;
	num_bits_missing = -8 + buf->bit_index;
	FAIL_ON_ERR(peek_byte(&byte, &buf->byte_buf));
	
	if (num_bits_missing <= 0)
	{
		byte >>= (8 - buf->bit_index);
		byte &= bitmasks_u8[num_bits];
		result = byte;
	}
	else
	{
		buf->byte_buf.offset++;
		buf->bit_index = 0;
		byte &= bitmasks_u8[num_bits - num_bits_missing];
		FAIL_ON_ERR(get_bits_msbfirst(&result, buf, num_bits_missing));
		result |= byte << num_bits_missing;
	}
	
	if (dest != NULL)
	{
		*dest = result;
	}
	
	return E_OK;
}

err_t
get_bits_lsbfirst (codeword_t *dest, bitstream_buffer *buf, uint8_t num_bits)
{
	codeword_t result;
	int num_bits_missing;
	uint8_t byte;
	
	buf->bit_index += num_bits;
	num_bits_missing = -8 + buf->bit_index;
	FAIL_ON_ERR(peek_byte(&byte, &buf->byte_buf));
	
	if (num_bits_missing <= 0)
	{
		byte >>= buf->bit_index - num_bits;
		byte &= bitmasks_u8[num_bits];
		result = byte;
	}
	else
	{
		buf->byte_buf.offset++;
		byte >>= buf->bit_index - num_bits;
		buf->bit_index = 0;
		byte &= bitmasks_u8[num_bits - num_bits_missing];
		FAIL_ON_ERR(get_bits_lsbfirst(&result, buf, num_bits_missing));
		byte |= result << (num_bits - num_bits_missing);
		result = byte;
	}
	
	if (dest != NULL)
	{
		*dest = result;
	}
	
	return E_OK;
}

err_t
put_bits_msbfirst (bitstream_buffer *buf, codeword_t src, uint8_t num_bits)
{
	int num_bits_left;
	uint8_t byte;
	
	buf->bit_index += num_bits;
	num_bits_left = -8 + buf->bit_index;
	FAIL_ON_ERR(peek_byte(&byte, &buf->byte_buf));
	
	if (num_bits_left <= 0)
	{
		src &= bitmasks_u8[num_bits];
		src <<= (8 - buf->bit_index);
		byte |= src;
		(void) poke_byte(&buf->byte_buf, byte);
	}
	else
	{
		codeword_t temp = src >> num_bits_left;
		temp &= bitmasks_u8[num_bits - num_bits_left];
		byte |= temp;
		(void) poke_byte(&buf->byte_buf, byte);
		buf->byte_buf.offset++;
		buf->bit_index = 0;
		FAIL_ON_ERR(put_bits_msbfirst(buf, src, num_bits_left));
	}
	
	return E_OK;
}

err_t
put_bits_lsbfirst (bitstream_buffer *buf, codeword_t src, uint8_t num_bits)
{
	int num_bits_left;
	uint8_t byte;
	
	buf->bit_index += num_bits;
	num_bits_left = -8 + buf->bit_index;
	FAIL_ON_ERR(peek_byte(&byte, &buf->byte_buf));
	
	if (num_bits_left <= 0)
	{
		src &= bitmasks_u8[num_bits];
		src <<= (buf->bit_index - num_bits);
		byte |= src;
		(void) poke_byte(&buf->byte_buf, byte);
	}
	else
	{
		codeword_t temp = src & bitmasks_u8[num_bits - num_bits_left];
		codeword_t temp2 = src >> (num_bits - num_bits_left);
		temp <<= (buf->bit_index - num_bits);
		byte |= temp;
		(void) poke_byte(&buf->byte_buf, byte);
		buf->byte_buf.offset++;
		buf->bit_index = 0;
		FAIL_ON_ERR(put_bits_lsbfirst(buf, temp2, num_bits_left));
	}
	
	return E_OK;
}

