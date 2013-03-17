/*
 * blt(), also know as memmove(3), include file for Mathomatic.
 *
 * Copyright (C) 1987-2012 George Gesslein II.
 
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

The chief copyright holder can be contacted at gesslein@mathomatic.org, or
George Gesslein II, P.O. Box 224, Lansing, NY  14882-0224  USA.
 
 */

#if	1
#define	blt(dest, src, cnt)	memmove((dest), (src), (cnt))	/* memory copy function; must allow overlapping of src and dest */
#else
/* If no fast or working memmove(3) routine exists use this one. */
static inline char *
blt(dest, src, cnt)
char		*dest;
const char	*src;
int		cnt;
{
	char		*tdest;
	const char	*tsrc;
	int		tcnt;

	if (cnt <= 0) {
		if (cnt == 0) {
			return dest;
		} else {
			error_bug("blt() cnt < 0");
		}
	}
	if (src == dest) {
		return dest;
	}

	tdest = dest;
	tsrc = src;
	tcnt = cnt;

	if (tdest > tsrc) {
		tdest += tcnt;
		tsrc += tcnt;
		while (--tcnt >= 0)
			*--tdest = *--tsrc;
	} else {
		while (--tcnt >= 0)
			*tdest++ = *tsrc++;
	}
	return dest;
}
#endif
