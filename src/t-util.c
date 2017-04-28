/* t-util.c - Regression test for util.c
 * Copyright (C) 2015 g10 Code GmbH
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include <assert.h>

#include "t-common.h"

#include "util.h" /* The module under test.  */


static void
test_keyvalue_put_meta (void)
{
  static struct { const char *string; } tests[] = {
    { "Name=Werner&Email=wk=test@gnupg.org&" },
    { "Name=" },
    { "Name=&" },
    { "Name==" },
    { "Name=&Foo=%3d%26" },
    /* Fixme: Add bad case tests.  */
    { NULL }
  };
  gpg_error_t err;
  keyvalue_t data = NULL;
  keyvalue_t kv;
  int idx;

  for (idx=0; tests[idx].string; idx++)
    {
      if (verbose)
        printf ("test %d: '%s'\n", idx, tests[idx].string);
      err = keyvalue_put_meta (&data, tests[idx].string);
      if (err)
        {
          fprintf (stderr, "test %d ('%s') failed: %s\n",
                  idx, tests[idx].string, gpg_strerror (err));
          fail (idx);
        }
      else if (verbose)
        {
          for (kv = data; kv; kv = kv->next)
            printf ("  %s: %s\n", kv->name, kv->value);
        }
      keyvalue_release (data);
      data = NULL;
    }
}


int
main (int argc, char **argv)
{
  if (argc > 1 && !strcmp (argv[1], "--verbose"))
    verbose = 1;

  test_keyvalue_put_meta ();

  return !!errorcount;
}