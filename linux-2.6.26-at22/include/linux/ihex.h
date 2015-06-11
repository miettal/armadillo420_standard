/*
 * Compact binary representation of ihex records. Some devices need their
 * firmware loaded in strange orders rather than a single big blob, but 
 * actually parsing ihex-as-text within the kernel seems silly. Thus,...
 */

#ifndef __LINUX_IHEX_H__
#define __LINUX_IHEX_H__

#include <linux/types.h>

struct ihex_binrec {
	__be32 addr;
	uint8_t len;
	uint8_t data[0];
} __attribute__((aligned(4)));

/* Find the next record, taking into account the 4-byte alignment */
static inline const struct ihex_binrec *
ihex_next_binrec(const struct ihex_binrec *rec)
{
	int next = ((rec->len + 4) & ~3) - 1;
	rec = (void *)&rec->data[next];

	return rec->len ? rec : NULL;
}

#endif /* __LINUX_IHEX_H__ */
