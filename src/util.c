/* util.c - Genereal utility functions.
 * Copyright (C) 1998, 1999, 2000, 2001, 2003, 2004, 2005, 2006, 2007,
 *               2008, 2009, 2010  Free Software Foundation, Inc.
 * Copyright (C) 2014 g10 Code GmbH
 *
 * This file is part of Payproc.
 *
 * Payproc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Payproc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include "util.h"
#include "estream.h"

/* The error source number for Payproc.  */
gpg_err_source_t default_errsource;



static void
out_of_core(void)
{
  fputs ("\nfatal: out of memory\n", stderr);
  exit (2);
}


void *
xmalloc( size_t n )
{
  void *p = malloc( n );
  if (!p)
    out_of_core ();
  return p;
}


void *
xrealloc (void *a, size_t n)
{
  void *p = realloc (a, n);
  if (!p)
    out_of_core();
  return p;
}


void *
xcalloc (size_t n, size_t m)
{
  void *p = calloc( n, m );
  if (!p)
    out_of_core ();
  return p;
}


char *
xstrdup (const char *string)
{
  void *p = xmalloc (strlen(string)+1);
  strcpy (p, string);
  return p;
}



static char *
do_strconcat (const char *s1, va_list arg_ptr)
{
  const char *argv[48];
  size_t argc;
  size_t needed;
  char *buffer, *p;

  argc = 0;
  argv[argc++] = s1;
  needed = strlen (s1);
  while (((argv[argc] = va_arg (arg_ptr, const char *))))
    {
      needed += strlen (argv[argc]);
      if (argc >= DIM (argv)-1)
        {
          gpg_err_set_errno (EINVAL);
          return NULL;
        }
      argc++;
    }
  needed++;
  buffer = xtrymalloc (needed);
  if (buffer)
    {
      for (p = buffer, argc=0; argv[argc]; argc++)
        p = stpcpy (p, argv[argc]);
    }
  return buffer;
}


/* Concatenate the string S1 with all the following strings up to a
   NULL.  Returns a malloced buffer with the new string or NULL on a
   malloc error or if too many arguments are given.  */
char *
strconcat (const char *s1, ...)
{
  va_list arg_ptr;
  char *result;

  if (!s1)
    result = xtrystrdup ("");
  else
    {
      va_start (arg_ptr, s1);
      result = do_strconcat (s1, arg_ptr);
      va_end (arg_ptr);
    }
  return result;
}



/*
 * Check whether STRING starts with KEYWORD.  The keyword is
 * delimited by end of string, a space or a tab.  Returns NULL if not
 * found or a pointer into STRING to the next non-space character
 * after the KEYWORD (which may be end of string).
 */
char *
has_leading_keyword (const char *string, const char *keyword)
{
  size_t n = strlen (keyword);

  if (!strncmp (string, keyword, n)
      && (!string[n] || string[n] == ' ' || string[n] == '\t'))
    {
      string += n;
      while (*string == ' ' || *string == '\t')
        string++;
      return (char*)string;
    }
  return NULL;
}



keyvalue_t
keyvalue_create (const char *key, const char *value)
{
  keyvalue_t kv;

  /* Insert a new data item. */
  kv = xtrymalloc (sizeof *kv + strlen (value));
  if (!kv)
    return NULL;
  kv->next = NULL;
  strcpy (kv->name, key);
  kv->value = xtrystrdup (value);
  if (!kv->value)
    {
      xfree (kv);
      return NULL;
    }
  return kv;
}


/* Append the string VALUE to the current value of KV.  */
gpg_error_t
keyvalue_append_to_last (keyvalue_t kv, const char *value)
{
  size_t n;
  char *p;

  n = strlen (kv->value) + strlen (value);
  p = xtrymalloc (n+1);
  if (!p)
    return gpg_err_code_from_syserror ();
  strcpy (stpcpy (p, kv->value), value);
  xfree (kv->value);
  kv->value = p;
  return 0;
}


gpg_error_t
keyvalue_put (keyvalue_t *list, const char *key, const char *value)
{
  keyvalue_t kv;

  kv = keyvalue_create (key, value);
  if (!kv)
    return gpg_error_from_syserror ();
  kv->next = *list;
  *list = kv;
  return 0;
}


gpg_error_t
keyvalue_putf (keyvalue_t *list, const char *key, const char *format, ...)
{
  va_list arg_ptr;
  char *value;
  keyvalue_t kv;

  va_start (arg_ptr, format);
  value = es_vasprintf (format, arg_ptr);
  va_end (arg_ptr);
  if (!value)
    return gpg_error_from_syserror ();

  kv = keyvalue_create (key, value);
  if (!kv)
    return gpg_error_from_syserror ();
  kv->next = *list;
  *list = kv;
  return 0;
}


void
keyvalue_release (keyvalue_t kv)
{
  while (kv)
    {
      keyvalue_t nxt = kv->next;
      xfree (kv->value);
      kv = nxt;
    }
}


const char *
keyvalue_get (keyvalue_t list, const char *key)
{
  keyvalue_t kv;

  for (kv = list; kv; kv = kv->next)
    if (!strcmp (kv->name, key))
      return kv->value;
  return NULL;
}


const char *
keyvalue_get_string (keyvalue_t list, const char *key)
{
  const char *s = keyvalue_get (list, key);
  return s? s: "";
}
