/*
 * Misc/memory.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-11-29
 */

/*
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
*/ 


#include <memory.h>

#define CUTOFF 8

static void shortsort(char *lo, char *hi, unsigned element_size, int (*compare_func)(const void *, const void *));
static void swap(char *p, char *q, unsigned int element_size);

void memory_sort(void *base, size_t count, size_t element_size, int (*compare_func)(const void *, const void *))
{
  char *lo, *hi;
  char *mid;
  char *loguy, *higuy;
  unsigned size;
  char *lostk[30], *histk[30];
  int stkptr;

  if (count < 2 || element_size == 0) return;
  stkptr = 0;

  lo = base;
  hi = (char *) base + element_size * (count-1);

recurse:
  size = (hi - lo) / element_size + 1;        // countber of el's to sort

  if (size <= CUTOFF) 
  {
    shortsort(lo, hi, element_size, compare_func);
  }
  else 
  {
    mid = lo + (size / 2) * element_size;
    swap(mid, lo, element_size);

    loguy = lo;
    higuy = hi + element_size;

    for (;;) 
    {
      do  { loguy += element_size; } while (loguy <= hi && compare_func(loguy, lo) <= 0);
      do  { higuy -= element_size; } while (higuy > lo && compare_func(higuy, lo) >= 0);
      if (higuy < loguy) break;
      swap(loguy, higuy, element_size);
    }

    swap(lo, higuy, element_size);

    if ( higuy - 1 - lo >= hi - loguy ) 
    {
      if (lo + element_size < higuy) 
      {
	lostk[stkptr] = lo;
	histk[stkptr] = higuy - element_size;
	++stkptr;
      }

      if (loguy < hi) 
      {
	lo = loguy;
	goto recurse;
      }
    }
    else
    {
      if (loguy < hi) 
      {
        lostk[stkptr] = loguy;
        histk[stkptr] = hi;
        ++stkptr;
      }

      if (lo + element_size < higuy) 
      {
      	hi = higuy - element_size;
      	goto recurse;
      }
    }
  }

  --stkptr;
  if (stkptr >= 0) 
  {
    lo = lostk[stkptr];
    hi = histk[stkptr];
    goto recurse;
  }
  else
    return;
}

static void shortsort(char *lo, char *hi, unsigned element_size, int (*compare_func)(const void *, const void *))
{
  char *p, *max;

  while (hi > lo) 
  {
    max = lo;
    for (p = lo+element_size; p <= hi; p += element_size) if (compare_func(p, max) > 0) max = p;
    swap(max, hi, element_size);
    hi -= element_size;
  }
}

static void swap(char *a, char *b, unsigned element_size)
{
  char tmp;

  if (a != b)
  {
    while (element_size--) 
    {
      tmp = *a;
      *a++ = *b;
      *b++ = tmp;
    }
  }
}
