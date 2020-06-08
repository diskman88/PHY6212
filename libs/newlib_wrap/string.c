#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern size_t  __heap_start;

char   *_sbrk(int incr)
{
    static unsigned char *heap = NULL;
    unsigned char *prev_heap;

    if (heap == NULL) {
        heap = (unsigned char *)&__heap_start;
    }

    prev_heap = heap;
    /* check removed to show basic approach */

    heap += incr;

    return (char *) prev_heap;
}

void *memcpy( void *dst, const void *src, unsigned int len )
{
  extern void *osal_memcpy(void *dst, const void *src, unsigned int len);
  return osal_memcpy(dst, src, len);
}

void * memset(void * s,int c, int count)
{
	unsigned int *sl = (unsigned int *) s;
	unsigned int cl = 0;
	char *s8;
	int i;

	/* do it one word at a time (32 bits or 64 bits) while possible */
	if ( ((int)s & (sizeof(*sl) - 1)) == 0) {
		for (i = 0; i < sizeof(*sl); i++) {
			cl <<= 8;
			cl |= c & 0xff;
		}
		while (count >= sizeof(*sl)) {
			*sl++ = cl;
			count -= sizeof(*sl);
		}
	}
	/* fill 8 bits at a time */
	s8 = (char *)sl;
	while (count--)
		*s8++ = c;

	return s;
}

int memcmp( const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;

}

