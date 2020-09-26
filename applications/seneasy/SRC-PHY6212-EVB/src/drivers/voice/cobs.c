/*
https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing

    Consistent Overhead Byte Stuffing (COBS) is an algorithm for encoding data bytes that results in efficient, reliable, 
unambiguous packet framing regardless of packet content, thus making it easy for receiving applications to recover 
from malformed packets.
    It employs a particular byte value, typically zero, to serve as a packet delimiter (a special value that indicates the 
boundary between packets). When zero is used as a delimiter, the algorithm replaces each zero data byte with a non-zero value 
so that no zero data bytes will appear in the packet and thus be misinterpreted as packet boundaries.

    Byte stuffing is a process that transforms a sequence of data bytes that may contain 'illegal' or 'reserved' values (such 
as packet delimiter) into a potentially longer sequence that contains no occurrences of those values. The extra length of the 
transformed sequence is typically referred to as the overhead of the algorithm. 
    The COBS algorithm tightly bounds the worst-case overhead, limiting it to a minimum of one byte and a maximum of ⌈n/254⌉ bytes 
(one byte in 254,rounded up). Consequently, the time to transmit the encoded byte sequence is highly predictable, which makes 
COBS useful for real-time applications in which jitter may be problematic. 
    The algorithm is computationally inexpensive and its average overhead is low compared to other unambiguous framing algorithms.[1][2]

    COBS does, however, require up to 254 bytes of lookahead. Before transmitting its first byte, it needs to know the position 
of the first zero byte (if any) in the following 254 bytes.
*/

/*
 * StuffData byte stuffs "length" bytes of data
 * at the location pointed to by "ptr", writing
 * the output to the location pointed to by "dst".
 *
 * Returns the length of the encoded data.
 */
#include <stdint.h>
#include <stddef.h>

size_t StuffData(const uint8_t *ptr, size_t length, uint8_t *dst)
{
	uint8_t *start = dst;
	uint8_t code = 1, *code_ptr = dst++; /* Where to insert the leading count */

	while (length--)
	{
		if (*ptr) /* Input byte not zero */
			*dst++ = *ptr, ++code;

		if (!*ptr++ || code == 0xFF) /* Input is zero or complete block */
			*code_ptr = code, code = 1, code_ptr = dst++;
	}
	*code_ptr = code; /* Final code */

	return dst - start;
}

/*
 * UnStuffData decodes "length" bytes of data at
 * the location pointed to by "ptr", writing the
 * output to the location pointed to by "dst".
 *
 * Returns the length of the decoded data
 * (which is guaranteed to be <= length).
 */
size_t UnStuffData(const uint8_t *ptr, size_t length, uint8_t *dst)
{
	const uint8_t *start = dst, *end = ptr + length;
	uint8_t code = 0xFF, copy = 0;

	for (; ptr < end; copy--) {
		if (copy != 0) {
			*dst++ = *ptr++;
		} else {
			if (code != 0xFF)
				*dst++ = 0;
			copy = code = *ptr++;
			if (code == 0)
				break; /* Source length too long */
		}
	}
	return dst - start;
}