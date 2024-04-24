
/*
 * Generates an 8-bit character set table from a .TXT table as found on
 * ftp.unicode.org or from a table containing the 256 Unicode values as
 * hexadecimal integers.
 * Examples:
 *
 *   ./8bit_tab_to_h ISO-8859-1 iso8859_1 < tab8859_1
 *   ./8bit_tab_to_h ISO-8859-2 iso8859_2 < tab8859_2
 *   ./8bit_tab_to_h ISO-8859-3 iso8859_3 < tab8859_3
 *   ./8bit_tab_to_h ISO-8859-4 iso8859_4 < tab8859_4
 *   ./8bit_tab_to_h ISO-8859-5 iso8859_5 < tab8859_5
 *   ./8bit_tab_to_h ISO-8859-6 iso8859_6 < tab8859_6
 *   ./8bit_tab_to_h ISO-8859-7 iso8859_7 < tab8859_7
 *   ./8bit_tab_to_h ISO-8859-8 iso8859_8 < tab8859_8
 *   ./8bit_tab_to_h ISO-8859-9 iso8859_9 < tab8859_9
 *   ./8bit_tab_to_h ISO-8859-10 iso8859_10 < tab8859_10
 *   ./8bit_tab_to_h ISO-8859-14 iso8859_14 < tab8859_14
 *   ./8bit_tab_to_h ISO-8859-15 iso8859_15 < tab8859_15
 *   ./8bit_tab_to_h JISX0201.1976-0 jisx0201 < jis0201
 *   ./8bit_tab_to_h TIS620-0 tis620 < tabtis620
 *   ./8bit_tab_to_h KOI8-R koi8_r < tabkoi8_r
 *   ./8bit_tab_to_h KOI8-U koi8_u < tabkoi8_u
 *   ./8bit_tab_to_h ARMSCII-8 armscii_8 < tabarmscii_8
 *   ./8bit_tab_to_h CP1133 cp1133 < tabibm_cp1133
 *   ./8bit_tab_to_h MULELAO-1 mulelao < tabmulelao_1
 *   ./8bit_tab_to_h VISCII1.1-1 viscii1 < tabviscii
 *   ./8bit_tab_to_h TCVN-5712 tcvn < tabtcvn
 *   ./8bit_tab_to_h GEORGIAN-ACADEMY georgian_ac < tabgeorgian_academy
 *   ./8bit_tab_to_h GEORGIAN-PS georgian_ps < tabgeorgian_ps
 *
 *   ./8bit_tab_to_h ISO-8859-1 iso8859_1 < 8859-1.TXT
 *   ./8bit_tab_to_h ISO-8859-2 iso8859_2 < 8859-2.TXT
 *   ./8bit_tab_to_h ISO-8859-3 iso8859_3 < 8859-3.TXT
 *   ./8bit_tab_to_h ISO-8859-4 iso8859_4 < 8859-4.TXT
 *   ./8bit_tab_to_h ISO-8859-5 iso8859_5 < 8859-5.TXT
 *   ./8bit_tab_to_h ISO-8859-6 iso8859_6 < 8859-6.TXT
 *   ./8bit_tab_to_h ISO-8859-7 iso8859_7 < 8859-7.TXT
 *   ./8bit_tab_to_h ISO-8859-8 iso8859_8 < 8859-8.TXT
 *   ./8bit_tab_to_h ISO-8859-9 iso8859_9 < 8859-9.TXT
 *   ./8bit_tab_to_h ISO-8859-10 iso8859_10 < 8859-10.TXT
 *   ./8bit_tab_to_h ISO-8859-14 iso8859_14 < 8859-14.TXT
 *   ./8bit_tab_to_h ISO-8859-15 iso8859_15 < 8859-15.TXT
 *   ./8bit_tab_to_h JISX0201.1976-0 jisx0201 < JIS0201.TXT
 *   ./8bit_tab_to_h KOI8-R koi8_r < KOI8-R.TXT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int main (int argc, char *argv[])
{
  const char* charsetname;
  const char* c_charsetname;
  const char* filename;
  const char* directory;
  int charset2uni[0x100];

  if (argc != 3 && argc != 4 && argc != 5)
    exit(1);
  charsetname = argv[1];
  c_charsetname = argv[2];
  if (argc > 3) {
    filename = argv[3];
  } else {
    char* s = malloc(strlen(c_charsetname)+strlen(".h")+1);
    strcpy(s,c_charsetname); strcat(s,".h");
    filename = s;
  }
  directory = (argc > 4 ? argv[4] : "");

  fprintf(stderr, "Creating %s%s\n", directory, filename);

  {
    int i, c;
    c = getc(stdin);
    ungetc(c,stdin);
    if (c == '#') {
      /* Read a unicode.org style .TXT file. */
      for (i = 0; i < 0x100; i++)
        charset2uni[i] = 0xfffd;
      for (;;) {
        c = getc(stdin);
        if (c == EOF)
          break;
        if (c == '\n' || c == ' ' || c == '\t')
          continue;
        if (c == '#') {
          do { c = getc(stdin); } while (!(c == EOF || c == '\n'));
          continue;
        }
        ungetc(c,stdin);
        if (scanf("0x%x", &i) != 1 || !(i >= 0 && i < 0x100))
          exit(1);
        do { c = getc(stdin); } while (c == ' ' || c == '\t');
        if (c != EOF)
          ungetc(c,stdin);
        if (c == '\n' || c == '#')
          continue;
        if (scanf("0x%x", &charset2uni[i]) != 1)
          exit(1);
      }
    } else {
      /* Read a table of hexadecimal Unicode values. */
      for (i = 0; i < 0x100; i++) {
        if (scanf("%x", &charset2uni[i]) != 1)
          exit(1);
        if (charset2uni[i] < 0 || charset2uni[i] == 0xffff)
          charset2uni[i] = 0xfffd;
      }
      if (scanf("%x", &i) != EOF)
        exit(1);
    }
  }

  /* Write the output file. */
  {
    FILE* f;

    {
      char* fname = malloc(strlen(directory)+strlen(filename)+1);
      strcpy(fname,directory); strcat(fname,filename);
      f = fopen(fname,"w");
      if (f == NULL)
        exit(1);
    }

    fprintf(f, "\n");
    fprintf(f, "/*\n");
    fprintf(f, " * %s\n", charsetname);
    fprintf(f, " */\n");
    fprintf(f, "\n");

    {
      int i, i1, i2, i3;
      int line[16];
      int tableno;
      struct { int minline; int maxline; } tables[16];
      bool some_invalid;
      bool final_ret_reached;

      for (i1 = 0; i1 < 16; i1++) {
        bool all_invalid = true;
        bool all_identity = true;
        for (i2 = 0; i2 < 16; i2++) {
          i = 16*i1+i2;
          if (charset2uni[i] != 0xfffd)
            all_invalid = false;
          if (charset2uni[i] != i)
            all_identity = false;
        }
        if (all_invalid)
          line[i1] = -2;
        else if (all_identity)
          line[i1] = -1;
        else
          line[i1] = 0;
      }
      tableno = 0;
      for (i1 = 0; i1 < 16; i1++) {
        if (line[i1] >= 0) {
          if (i1 > 0 && tableno > 0 && line[i1-1] == tableno-1) {
            line[i1] = tableno-1;
            tables[tableno-1].maxline = i1;
          } else {
            tableno++;
            line[i1] = tableno-1;
            tables[tableno-1].minline = tables[tableno-1].maxline = i1;
          }
        }
      }
      some_invalid = false;
      for (i = 0; i < 0x100; i++)
        if (charset2uni[i] == 0xfffd)
          some_invalid = true;
      if (tableno > 0) {
        int t;
        for (t = 0; t < tableno; t++) {
          fprintf(f, "static const unsigned short %s_2uni", c_charsetname);
          if (tableno > 1)
            fprintf(f, "_%d", t+1);
          fprintf(f, "[%d] = {\n", 16*(tables[t].maxline-tables[t].minline+1));
          for (i1 = tables[t].minline; i1 <= tables[t].maxline; i1++) {
            fprintf(f, "  /* 0x%02x */\n", 16*i1);
            for (i2 = 0; i2 < 2; i2++) {
              fprintf(f, " ");
              for (i3 = 0; i3 < 8; i3++) {
                i = 16*i1+8*i2+i3;
                fprintf(f, " 0x%04x,", charset2uni[i]);
              }
              fprintf(f, "\n");
            }
          }
          fprintf(f, "};\n");
        }
        fprintf(f, "\n");
      }
      final_ret_reached = false;
      fprintf(f, "static int\n%s_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)\n", c_charsetname);
      fprintf(f, "{\n");
      fprintf(f, "  unsigned char c = *s;\n");
      if (some_invalid) {
        for (i1 = 0; i1 < 16;) {
          int t = line[i1];
          const char* indent;
          for (i2 = i1; i2 < 16 && line[i2] == t; i2++);
          indent = (i1 == 0 && i2 == 16 ? "  " : "    ");
          if (i1 == 0) {
            if (i2 == 16) {
            } else {
              fprintf(f, "  if (c < 0x%02x) {\n", 16*i2);
            }
          } else {
            if (i2 == 16) {
              fprintf(f, "  else {\n");
            } else {
              fprintf(f, "  else if (c < 0x%02x) {\n", 16*i2);
            }
          }
          if (t == -2) {
            final_ret_reached = true;
          } else if (t == -1) {
            fprintf(f, "%s*pwc = (ucs4_t) c;\n", indent);
            fprintf(f, "%sreturn 1;\n", indent);
          } else {
            fprintf(f, "%s", indent);
            some_invalid = false;
            for (i = 16*i1; i < 16*i2; i++)
              if (charset2uni[i] == 0xfffd)
                some_invalid = true;
            if (some_invalid)
              fprintf(f, "unsigned short wc = ");
            else
              fprintf(f, "*pwc = (ucs4_t) ");
            fprintf(f, "%s_2uni", c_charsetname);
            if (tableno > 1)
              fprintf(f, "_%d", t+1);
            fprintf(f, "[c");
            if (tables[t].minline > 0)
              fprintf(f, "-0x%02x", 16*tables[t].minline);
            fprintf(f, "];\n");
            if (some_invalid) {
              fprintf(f, "%sif (wc != 0xfffd) {\n", indent);
              fprintf(f, "%s  *pwc = (ucs4_t) wc;\n", indent);
              fprintf(f, "%s  return 1;\n", indent);
              fprintf(f, "%s}\n", indent);
              final_ret_reached = true;
            } else {
              fprintf(f, "%sreturn 1;\n", indent);
            }
          }
          if (!(i1 == 0 && i2 == 16))
            fprintf(f, "  }\n");
          i1 = i2;
        }
        if (final_ret_reached)
          fprintf(f, "  return RET_ILSEQ;\n");
      } else {
        for (i1 = 0; i1 < 16;) {
          int t = line[i1];
          for (i2 = i1; i2 < 16 && line[i2] == t; i2++);
          if (i1 == 0) {
            if (i2 == 16) {
              fprintf(f, "  ");
            } else {
              fprintf(f, "  if (c < 0x%02x)\n    ", 16*i2);
            }
          } else {
            if (i2 == 16) {
              fprintf(f, "  else\n    ");
            } else {
              fprintf(f, "  else if (c < 0x%02x)\n    ", 16*i2);
            }
          }
          if (t == -1)
            fprintf(f, "*pwc = (ucs4_t) c;\n");
          else {
            fprintf(f, "*pwc = (ucs4_t) %s_2uni", c_charsetname);
            if (tableno > 1)
              fprintf(f, "_%d", t+1);
            fprintf(f, "[c");
            if (tables[t].minline > 0)
              fprintf(f, "-0x%02x", 16*tables[t].minline);
            fprintf(f, "];\n");
          }
          i1 = i2;
        }
        fprintf(f, "  return 1;\n");
      }
      fprintf(f, "}\n");

    }

    fprintf(f, "\n");

    {
      int uni2charset[0x10000];
      bool pages[0x100];
      int line[0x2000];
      int tableno;
      struct { int minline; int maxline; int usecount; const char* suffix; } tables[0x2000];
      bool need_c;
      bool fix_0000;
      int i, j, p, j1, j2, t;

      for (j = 0; j < 0x10000; j++)
        uni2charset[j] = 0;
      for (p = 0; p < 0x100; p++)
        pages[p] = false;
      for (i = 0; i < 0x100; i++) {
        j = charset2uni[i];
        if (j != 0xfffd) {
          uni2charset[j] = i;
          pages[j>>8] = true;
        }
      }
      for (j1 = 0; j1 < 0x2000; j1++) {
        bool all_invalid = true;
        bool all_identity = true;
        for (j2 = 0; j2 < 8; j2++) {
          j = 8*j1+j2;
          if (uni2charset[j] != 0)
            all_invalid = false;
          if (uni2charset[j] != j)
            all_identity = false;
        }
        if (all_invalid)
          line[j1] = -2;
        else if (all_identity)
          line[j1] = -1;
        else
          line[j1] = 0;
      }
      tableno = 0;
      for (j1 = 0; j1 < 0x2000; j1++) {
        if (line[j1] >= 0) {
          if (tableno > 0
              && ((j1 > 0 && line[j1-1] == tableno-1)
                  || ((tables[tableno-1].maxline >> 5) == (j1 >> 5)
                      && j1 - tables[tableno-1].maxline <= 8))) {
            line[j1] = tableno-1;
            tables[tableno-1].maxline = j1;
          } else {
            tableno++;
            line[j1] = tableno-1;
            tables[tableno-1].minline = tables[tableno-1].maxline = j1;
          }
        }
      }
      for (t = 0; t < tableno; t++) {
        tables[t].usecount = 0;
        j1 = 8*tables[t].minline;
        j2 = 8*(tables[t].maxline+1);
        for (j = j1; j < j2; j++)
          if (uni2charset[j] != 0)
            tables[t].usecount++;
      }
      for (t = 0, p = -1, i = 0; t < tableno; t++) {
        if (tables[t].usecount > 1) {
          char* s;
          if (p == tables[t].minline >> 5) {
            s = malloc(5+1);
            sprintf(s, "%02x_%d", p, ++i);
          } else {
            p = tables[t].minline >> 5;
            s = malloc(2+1);
            sprintf(s, "%02x", p);
          }
          tables[t].suffix = s;
        } else
          tables[t].suffix = NULL;
      }
      {
        p = -1;
        for (t = 0; t < tableno; t++)
          if (tables[t].usecount > 1) {
            p = 0;
            fprintf(f, "static const unsigned char %s_page%s[%d] = {\n", c_charsetname, tables[t].suffix, 8*(tables[t].maxline-tables[t].minline+1));
            for (j1 = tables[t].minline; j1 <= tables[t].maxline; j1++) {
              if ((j1 % 0x20) == 0 && j1 > tables[t].minline)
                fprintf(f, "  /* 0x%04x */\n", 8*j1);
              fprintf(f, " ");
              for (j2 = 0; j2 < 8; j2++) {
                j = 8*j1+j2;
                fprintf(f, " 0x%02x,", uni2charset[j]);
              }
              fprintf(f, " /* 0x%02x-0x%02x */\n", 8*(j1 % 0x20), 8*(j1 % 0x20)+7);
            }
            fprintf(f, "};\n");
          }
        if (p >= 0)
          fprintf(f, "\n");
      }
      need_c = false;
      for (j1 = 0; j1 < 0x2000;) {
        t = line[j1];
        for (j2 = j1; j2 < 0x2000 && line[j2] == t; j2++);
        if (t >= 0)
          j2 = tables[t].maxline+1;
        if (!(t == -2 || (t == -1 && j1 == 0)))
          need_c = true;
        j1 = j2;
      }
      fix_0000 = false;
      fprintf(f, "static int\n%s_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)\n", c_charsetname);
      fprintf(f, "{\n");
      if (need_c)
        fprintf(f, "  unsigned char c = 0;\n");
      for (j1 = 0; j1 < 0x2000;) {
        t = line[j1];
        for (j2 = j1; j2 < 0x2000 && line[j2] == t; j2++);
        if (t >= 0) {
          if (j1 != tables[t].minline) abort();
          if (j2 > tables[t].maxline+1) abort();
          j2 = tables[t].maxline+1;
        }
        if (t == -2) {
        } else {
          if (j1 == 0)
            fprintf(f, "  ");
          else
            fprintf(f, "  else ");
          if (t >= 0 && tables[t].usecount == 0) abort();
          if (t >= 0 && tables[t].usecount == 1) {
            if (j2 != j1+1) abort();
            for (j = 8*j1; j < 8*j2; j++)
              if (uni2charset[j] != 0) {
                fprintf(f, "if (wc == 0x%04x)\n    c = 0x%02x;\n", j, uni2charset[j]);
                break;
              }
          } else {
            if (j1 == 0) {
              fprintf(f, "if (wc < 0x%04x)", 8*j2);
            } else {
              fprintf(f, "if (wc >= 0x%04x && wc < 0x%04x)", 8*j1, 8*j2);
            }
            if (t == -1) {
              if (j1 == 0)
                /* If wc == 0, the function must return 1, not -1. */
                fprintf(f, " {\n    *r = wc;\n    return 1;\n  }\n");
              else
                fprintf(f, "\n    c = wc;\n");
            } else {
              fprintf(f, "\n    c = %s_page%s[wc", c_charsetname, tables[t].suffix);
              if (tables[t].minline > 0)
                fprintf(f, "-0x%04x", 8*j1);
              fprintf(f, "];\n");
              if (j1 == 0 && uni2charset[0] == 0)
                /* If wc == 0, the function must return 1, not -1. */
                fix_0000 = true;
            }
          }
        }
        j1 = j2;
      }
      if (need_c) {
        if (fix_0000)
          fprintf(f, "  if (c != 0 || wc == 0) {\n");
        else
          fprintf(f, "  if (c != 0) {\n");
        fprintf(f, "    *r = c;\n");
        fprintf(f, "    return 1;\n");
        fprintf(f, "  }\n");
      }
      fprintf(f, "  return RET_ILSEQ;\n");
      fprintf(f, "}\n");

    }

    if (ferror(f) || fclose(f))
      exit(1);
  }

#if 0

    int i1, i2, i3, i1_min, i1_max, j1, j2;

  i1_min = 16;
  i1_max = -1;
  for (i1 = 0; i1 < 16; i1++)
    for (i2 = 0; i2 < 16; i2++)
      if (charset2uni[16*i1+i2] != 0xfffd) {
        if (i1_min > i1) i1_min = i1;
        if (i1_max < i1) i1_max = i1;
      }
  printf("static const unsigned short %s_2uni[%d] = {\n",
         name, 16*(i1_max-i1_min+1));
  for (i1 = i1_min; i1 <= i1_max; i1++) {
    printf("  /""* 0x%02x *""/\n", 16*i1);
    for (i2 = 0; i2 < 2; i2++) {
      printf("  ");
      for (i3 = 0; i3 < 8; i3++) {
        if (i3 > 0) printf(" ");
        printf("0x%04x,", charset2uni[16*i1+8*i2+i3]);
      }
      printf("\n");
    }
  }
  printf("};\n");
  printf("\n");

  for (p = 0; p < 0x100; p++)
    pages[p] = 0;
  for (i = 0; i < 0x100; i++)
    if (charset2uni[i] != 0xfffd)
      pages[charset2uni[i]>>8] = 1;
  for (p = 0; p < 0x100; p++)
    if (pages[p]) {
      int j1_min = 32;
      int j1_max = -1;
      for (j1 = 0; j1 < 32; j1++)
        for (j2 = 0; j2 < 8; j2++)
          if (uni2charset[256*p+8*j1+j2] != 0) {
            if (j1_min > j1) j1_min = j1;
            if (j1_max < j1) j1_max = j1;
          }
      printf("static const unsigned char %s_page%02x[%d] = {\n",
             name, p, 8*(j1_max-j1_min+1));
      for (j1 = j1_min; j1 <= j1_max; j1++) {
        printf("  ");
        for (j2 = 0; j2 < 8; j2++)
          printf("0x%02x, ", uni2charset[256*p+8*j1+j2]);
        printf("/""* 0x%02x-0x%02x *""/\n", 8*j1, 8*j1+7);
      }
      printf("};\n");
    }
  printf("\n");

}
#endif

  exit(0);
}
