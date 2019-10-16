/*
  Compress - file compressor and decompressor.
  Copyright (C) 2005, 2019  Fabrice Bauzac

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
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

  The main idea is that we can build bijections between (contents of
  files) and natural numbers.

  We then call "compressing" the act of converting the file to number,
  decrement the number, and converting it back to file.

  Likewise, we call "uncompressing" the act of converting the file to
  number, increment the number, and converting it back to file.

  A simple bijection: converting a file to number
  ===============================================

  By convention we choose that the empty file (the one with size 0)
  maps to the number 0.

  Then, we have all files of size 1 (one byte).  There are 2^8 = 256
  files of this category, we'll map them to numbers 1 to 2^8 = 256.
  The file 0x00 maps to 1, 0x01 maps to 2, ... 0xff maps to 2^8 = 256.

  Then, we have all files of size 2.  There are 2^16 = 65,536 files in
  this category, we'll map them to numbers 2^8+1 = 257 to (2^8+1 +
  2^16-1) = 2^8 + 2^16 = 65792.  The file 0x0000 maps to 257, 0x0001
  maps to 258, ... 0xffff maps to 65792.

  And so on so forth.  Let's measure the number of elements in each
  category:

  num_elem_size(0) = 2^0 = 1
  num_elem_size(1) = 2^8 = 256
  num_elem_size(2) = 2^16 = 65,536

  We can generalize:
  num_elem_size(N) = 2^(8*N)

  Let's determine the first number of each category:
  start_of_size(0) = 0
  start_of_size(1) = start_of_size(0) + num_elem_size(0) = 0 + 2^(8*0) = 1
  start_of_size(2) = start_of_size(1) + num_elem_size(1) = 1 + 2^(8*1) = 257

  We can generalize:

                      N
  start_of_size(N) = Sum  num_element_size(i)
                     i=0

                      N
                   = Sum  2^(8*i)
                     i=0

                      N
                   = Sum  256^i
                     i=0

  This is a geometric series.

                      1 - 256^N
  start_of_size(N) = -----------
                       1 - 256

                      256^N - 1     256^N - 1
                   = ----------- = -----------
                       256 - 1         255

  Given a file of size N, what is the offset from start_of_size(N)?
  It is simply the value of the file contents as a number:

  number(FILE) = start_of_size(size(FILE)) + offset

               = start_of_size(size(FILE)) + contents_as_number(FILE)

  The inverse: converting a number to file
  ========================================

  First we need to know in which interval (category) is our number.
  We do the inverse of start_of_size() to get the filesize N.

            256^N - 1
  number = -----------
               255

  255 * number + 1 = 256^N

  N = log_256(255 * number + 1)

  Note that here, for log_256() we return the floor, as in general the
  value is a real but rarely an integer.

  From there we can compute start_of_size(N).

  Then we obtain the offset, which we can write to the file of size N:

  contents_as_number = number - start_of_size
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

/* This constant is simply a mpz_t with value 255.  */
static mpz_t _255;

/* Return the size of the given FILE, in bytes.  */
static long int
size (FILE *file)
{
  if (fseek (file, 0, SEEK_END) != 0)
    FAILFILE;
  return ftell (file);
}

static unsigned char *contents;

/* Convert the given FILENAME to number N.  */
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

  /* Set n = 256^filesize = 256^N.  */
  mpz_setbit (n, 8*filesize);

  mpz_sub_ui (n, n, 1);
  /* Now n = 256^N - 1.  */

  mpz_fdiv_q (n, n, _255);
  /* Now n = (256^N - 1) / 255 = start_of_size(N).  */

  if (debug)
    gmp_printf ("get: interval base = %Zd\n", n);

  rewind (file);
  contents = calloc (filesize + 1, 1);
  if (fread (contents, 1, filesize, file) != filesize)
    FAILFILE;
  {
    mpz_t m;
    mpz_init (m);

    /* Set m = contents_as_number(FILE).  */
    mpz_import (m, filesize, -1, 1, 0, 0, contents);

    if (debug)
      gmp_printf ("get: interval offset = %Zd\n", m);

    /* Set n = n + m = number(FILE).  */
    mpz_add (n, n, m);
    mpz_clear (m);
  }

  fclose (file);
}

/* Convert the given number N to FILENAME.  */
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
  /* m = 255 * start_of_size(N).  */

  mpz_add_ui (m, m, 1);
  /* m = 255 * start_of_size(N) + 1.  */

  filesize = (mpz_sizeinbase (m, 2) - 1) / 8;
  /* filesize = log_256(255 * start_of_size(N) + 1) */

  if (debug)
    printf ("put: filesize = %ld\n", filesize);

  mpz_set_ui (m, 0);

  /* Set m = 256^filesize = 256^N.  */
  mpz_setbit (m, 8*filesize);

  mpz_sub_ui (m, m, 1);
  /* Now m = 256^N - 1.  */

  mpz_fdiv_q (m, m, _255);
  /* Now m = (256^N - 1) / 255 = start_of_size(N).  */

  if (debug)
    gmp_printf ("put: interval base = %Zd\n", m);

  mpz_sub (m, n, m);
  /* Now m = contents_as_number = number - start_of_size */

  if (debug)
    gmp_printf ("put: interval offset = %Zd\n", m);

  filesize2 = (size_t) filesize;
  memset (contents, 0, filesize);

  /* Write contents_as_number to the file.  */
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

  /* Initialize the _255 constant once and for all.  */
  mpz_init_set_ui (_255, 255);

  mpz_init (n);
  get (n, filename);            /* Convert the file to number.  */

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

  /* Subtracting is the way we "compress".  */
  mpz_sub (n, n, compress);

  if (debug)
    gmp_printf ("n - compress = %Zd\n", n);

  put (n, filename);            /* Convert the number to file.  */

  mpz_clear (_255);
  mpz_clear (compress);
  mpz_clear (n);
  return 0;
}
