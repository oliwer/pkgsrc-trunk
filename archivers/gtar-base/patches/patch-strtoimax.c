$NetBSD: patch-strtoimax.c,v 1.1 2014/07/18 11:12:41 ryoon Exp $

SCO OpenServer 5.0.7/3.2 has strtoimax() declaration in inttypes.h,
but libc.so has no implementation. And gnulib in GNU tar 1.27 has broken
strtoimax() support. We have to use BSDL implementation instead.

--- strtoimax.c.orig	2014-02-28 16:27:33.000000000 +0000
+++ strtoimax.c
@@ -0,0 +1,151 @@
+/*	NetBSD: strtoimax.c,v 1.3 2003/08/07 16:43:44 agc Exp 	*/
+
+/*-
+ * Copyright (c) 1992, 1993
+ *	The Regents of the University of California.  All rights reserved.
+ *
+ * Redistribution and use in source and binary forms, with or without
+ * modification, are permitted provided that the following conditions
+ * are met:
+ * 1. Redistributions of source code must retain the above copyright
+ *    notice, this list of conditions and the following disclaimer.
+ * 2. Redistributions in binary form must reproduce the above copyright
+ *    notice, this list of conditions and the following disclaimer in the
+ *    documentation and/or other materials provided with the distribution.
+ * 3. Neither the name of the University nor the names of its contributors
+ *    may be used to endorse or promote products derived from this software
+ *    without specific prior written permission.
+ *
+ * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
+ * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
+ * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
+ * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
+ * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
+ * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
+ * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
+ * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
+ * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
+ * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
+ * SUCH DAMAGE.
+ */
+
+#include <assert.h>
+#include <ctype.h>
+#include <errno.h>
+#include <inttypes.h>
+#include <stddef.h>
+
+/*
+ * Convert a string to an intmax_t.
+ *
+ * Ignores `locale' stuff.  Assumes that the upper and lower case
+ * alphabets and digits are each contiguous.
+ */
+intmax_t
+strtoimax(nptr, endptr, base)
+	const char *nptr;
+	char **endptr;
+	int base;
+{
+	const char *s;
+	intmax_t acc, cutoff;
+	int c;
+	int neg, any, cutlim;
+
+#ifdef __GNUC__
+	/* This outrageous construct just to shut up a GCC warning. */
+	(void) &acc; (void) &cutoff;
+#endif
+
+	/*
+	 * Skip white space and pick up leading +/- sign if any.
+	 * If base is 0, allow 0x for hex and 0 for octal, else
+	 * assume decimal; if base is already 16, allow 0x.
+	 */
+	s = nptr;
+	do {
+		c = (unsigned char) *s++;
+	} while (isspace(c));
+	if (c == '-') {
+		neg = 1;
+		c = *s++;
+	} else {
+		neg = 0;
+		if (c == '+')
+			c = *s++;
+	}
+	if ((base == 0 || base == 16) &&
+	    c == '0' && (*s == 'x' || *s == 'X')) {
+		c = s[1];
+		s += 2;
+		base = 16;
+	}
+	if (base == 0)
+		base = c == '0' ? 8 : 10;
+
+	/*
+	 * Compute the cutoff value between legal numbers and illegal
+	 * numbers.  That is the largest legal value, divided by the
+	 * base.  An input number that is greater than this value, if
+	 * followed by a legal input character, is too big.  One that
+	 * is equal to this value may be valid or not; the limit
+	 * between valid and invalid numbers is then based on the last
+	 * digit.  For instance, if the range for intmax_t is
+	 * [-9223372036854775808..9223372036854775807] and the input base
+	 * is 10, cutoff will be set to 922337203685477580 and cutlim to
+	 * either 7 (neg==0) or 8 (neg==1), meaning that if we have
+	 * accumulated a value > 922337203685477580, or equal but the
+	 * next digit is > 7 (or 8), the number is too big, and we will
+	 * return a range error.
+	 *
+	 * Set any if any `digits' consumed; make it negative to indicate
+	 * overflow.
+	 */
+	cutoff = neg ? INTMAX_MIN : INTMAX_MAX;
+	cutlim = (int)(cutoff % base);
+	cutoff /= base;
+	if (neg) {
+		if (cutlim > 0) {
+			cutlim -= base;
+			cutoff += 1;
+		}
+		cutlim = -cutlim;
+	}
+	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
+		if (isdigit(c))
+			c -= '0';
+		else if (isalpha(c))
+			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
+		else
+			break;
+		if (c >= base)
+			break;
+		if (any < 0)
+			continue;
+		if (neg) {
+			if (acc < cutoff || (acc == cutoff && c > cutlim)) {
+				any = -1;
+				acc = INTMAX_MIN;
+				errno = ERANGE;
+			} else {
+				any = 1;
+				acc *= base;
+				acc -= c;
+			}
+		} else {
+			if (acc > cutoff || (acc == cutoff && c > cutlim)) {
+				any = -1;
+				acc = INTMAX_MAX;
+				errno = ERANGE;
+			} else {
+				any = 1;
+				acc *= base;
+				acc += c;
+			}
+		}
+	}
+	if (endptr != 0)
+		/* LINTED interface specification */
+		*endptr = (char *)(any ? s - 1 : nptr);
+	return (acc);
+}