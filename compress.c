/*
  Compress - file compressor and decompressor.
  Copyright (C) 2005  Fabrice Bauzac

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

/*
  This is just a proof of concept.
  To compile, you need to link with libgmp http://gmplib.org/
  For any questions, ask Fabrice Bauzac <libnoon@gmail.com>.
*/

#include <stdio.h>
#include <getopt.h>
#include <gmp.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static int debug = 0;

static void
fail (char *str, ...)
{
  va_list ap;
  va_start (ap, str);
  gmp_vprintf (str, ap);
  va_end (ap);
  exit (1);
}

#define FAILFILE fail ("Unable to access file.\n")

static mpz_t _255;

static long int
size (FILE *file)
{
  if (fseek (file, 0, SEEK_END) != 0)
    FAILFILE;
  return ftell (file);
}

static unsigned char *contents;

static void
get (mpz_t n, char *filename)
{
  long int filesize;
  FILE *file;

  mpz_set_ui (n, 0);

  file = fopen (filename, "r");
  if (file == NULL)
    FAILFILE;

  filesize = size (file);
  if (filesize == -1)
    FAILFILE;

  if (debug)
    printf ("get: filesize = %ld\n", filesize);

  mpz_setbit (n, 8*filesize);
  mpz_sub_ui (n, n, 1);
  mpz_fdiv_q (n, n, _255);

  if (debug)
    gmp_printf ("get: interval base = %Zd\n", n);

  rewind (file);
  contents = calloc (filesize + 1, 1);
  if (fread (contents, 1, filesize, file) != filesize)
    FAILFILE;
  {
    mpz_t m;
    mpz_init (m);
    mpz_import (m, filesize, -1, 1, 0, 0, contents);

    if (debug)
      gmp_printf ("get: interval offset = %Zd\n", m);

    mpz_add (n, n, m);
    mpz_clear (m);
  }

  fclose (file);
}

static void
put (mpz_t n, char *filename)
{
  long int filesize;
  size_t filesize2;
  mpz_t m;
  FILE *file;

  file = fopen (filename, "w");
  if (!file)
    FAILFILE;

  mpz_init (m);
  mpz_mul (m, n, _255);
  mpz_add_ui (m, m, 1);
  filesize = (mpz_sizeinbase (m, 2) - 1) / 8;

  if (debug)
    printf ("put: filesize = %ld\n", filesize);

  mpz_set_ui (m, 0);
  mpz_setbit (m, 8*filesize);
  mpz_sub_ui (m, m, 1);
  mpz_fdiv_q (m, m, _255);

  if (debug)
    gmp_printf ("put: interval base = %Zd\n", m);

  mpz_sub (m, n, m);

  if (debug)
    gmp_printf ("put: interval offset = %Zd\n", m);

  filesize2 = (size_t) filesize;
  memset (contents, 0, filesize);
  mpz_export (contents, &filesize2, -1, 1, 0, 0, m);

  if (fwrite (contents, 1, filesize, file) != filesize)
    FAILFILE;

  mpz_clear (m);
}

int
main (int argc, char **argv)
{
  mpz_t compress;
  mpz_t n;
  char *filename;
  int c;

  mpz_init_set_si (compress, 0);

  while ((c = getopt (argc, argv, "vhcdC:D:")) != -1)
    {
      int sign = 1;

      switch (c)
        {
        case 'v':
          debug = 1;
          break;

        case 'D':
          sign = -1;
        case 'C':
          {
            mpz_t arg;
            int ret;
            mpz_init (arg);
            ret = mpz_set_str (arg, optarg, 0);
            if (ret != 0)
              fail ("Argument is not an integer.\n");
            if (sign > 0)
              mpz_add (compress, compress, arg);
            else
              mpz_sub (compress, compress, arg);
            mpz_clear (arg);
            break;
          }

        case 'c':
          mpz_add_ui (compress, compress, 1);
          break;

        case 'd':
          mpz_sub_ui (compress, compress, 1);
          break;

        case 'h':
          printf (("This program compresses and uncompresses files.\n"
                   "Its compression ratio is very near to no compression\n"
                   "(a fraction of a bit), but it ALWAYS compresses,\n"
                   "so you can ALWAYS get 0-length compressed files.\n"
                   "Experiment with very small files first.\n"
                   "Use repeatedly (billions of times or even more).\n"
                   "Usage: %s [-v] -c FILENAME       compress\n"
                   "       %s      -C 5 FILENAME     compress 5 times\n"
                   "       %s      -D 5 FILENAME     decompress 5 times\n"
                   "       %s      -d FILENAME       decompress\n"
                   " -v  debug mode.\n"),
                  argv[0], argv[0], argv[0],
                  argv[0], argv[0], argv[0]);
          exit (0);

        default:
          fail ("Error parsing arguments.\n");
        }
    }

  if (optind < argc)
    filename = argv[optind++];
  else
    fail ("No filename specified.\n");
  if (optind < argc)
    fail ("Too many arguments.\n");

  if (debug)
    gmp_printf ("compress = %Zd\n", compress);

  mpz_init_set_ui (_255, 255);

  mpz_init (n);
  get (n, filename);

  if (debug)
    gmp_printf ("get (filename) = %Zd\n", n);

  if (mpz_cmp_si (n, 0) == 0
      && mpz_cmp_si (compress, 0) > 0)
    fail ("Cannot compress a zero-length file.\n");

  if (mpz_cmp (n, compress) < 0)
    fail ("Cannot compress that much.\n"
          "Hint: compressing %Zd time(s) "
          "will make a zero-length file.\n",
          n);

  mpz_sub (n, n, compress);

  if (debug)
    gmp_printf ("n - compress = %Zd\n", n);

  put (n, filename);

  mpz_clear (_255);
  mpz_clear (compress);
  mpz_clear (n);
  return 0;
}
