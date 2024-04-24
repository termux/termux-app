
/*
 * Generates a CJK character set table from a .TXT table as found on
 * ftp.unicode.org or in the X nls directory.
 * Examples:
 *
 *   ./cjk_tab_to_h GB2312.1980-0 gb2312 > gb2312.h < gb2312
 *   ./cjk_tab_to_h JISX0208.1983-0 jisx0208 > jisx0208.h < jis0208
 *   ./cjk_tab_to_h KSC5601.1987-0 ksc5601 > ksc5601.h < ksc5601
 *
 *   ./cjk_tab_to_h GB2312.1980-0 gb2312 > gb2312.h < GB2312.TXT
 *   ./cjk_tab_to_h JISX0208.1983-0 jisx0208 > jisx0208.h < JIS0208.TXT
 *   ./cjk_tab_to_h JISX0212.1990-0 jisx0212 > jisx0212.h < JIS0212.TXT
 *   ./cjk_tab_to_h KSC5601.1987-0 ksc5601 > ksc5601.h < KSC5601.TXT
 *   ./cjk_tab_to_h KSX1001.1992-0 ksc5601 > ksc5601.h < KSX1001.TXT
 *
 *   ./cjk_tab_to_h BIG5 big5 > big5.h < BIG5.TXT
 *
 *   ./cjk_tab_to_h JOHAB johab > johab.h < JOHAB.TXT
 *
 *   ./cjk_tab_to_h BIG5HKSCS-0 big5hkscs >big5hkscs.h < BIG5HKSCS.TXT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
  int start;
  int end;
} Block;

typedef struct {
  int rows;    /* number of possible values for the 1st byte */
  int cols;    /* number of possible values for the 2nd byte */
  int (*row_byte) (int row); /* returns the 1st byte value for a given row */
  int (*col_byte) (int col); /* returns the 2nd byte value for a given col */
  int (*byte_row) (int byte); /* converts a 1st byte value to a row, else -1 */
  int (*byte_col) (int byte); /* converts a 2nd byte value to a col, else -1 */
  const char* check_row_expr; /* format string for 1st byte value checking */
  const char* check_col_expr; /* format string for 2nd byte value checking */
  const char* byte_row_expr; /* format string for 1st byte value to row */
  const char* byte_col_expr; /* format string for 2nd byte value to col */
  int** charset2uni; /* charset2uni[0..rows-1][0..cols-1] is valid */
  /* You'll understand the terms "row" and "col" when you buy Ken Lunde's book.
     Once a row is fixed, choosing a "col" is the same as choosing a "cell". */
  int* charsetpage; /* charsetpage[0..rows]: how large is a page for a row */
  int ncharsetblocks;
  Block* charsetblocks; /* blocks[0..nblocks-1] */
  int* uni2charset; /* uni2charset[0x0000..0xffff] */
} Encoding;

/*
 * Outputs the file title.
 */
static void output_title (const char *charsetname)
{
  printf("\n");
  printf("/*\n");
  printf(" * %s\n", charsetname);
  printf(" */\n");
  printf("\n");
}

/*
 * Reads the charset2uni table from standard input.
 */
static void read_table (Encoding* enc)
{
  int row, col, i, i1, i2, c, j;

  enc->charset2uni = malloc(enc->rows*sizeof(int*));
  for (row = 0; row < enc->rows; row++)
    enc->charset2uni[row] = malloc(enc->cols*sizeof(int));

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++)
      enc->charset2uni[row][col] = 0xfffd;

  c = getc(stdin);
  ungetc(c,stdin);
  if (c == '#') {
    /* Read a unicode.org style .TXT file. */
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
      if (scanf("0x%x", &j) != 1)
        exit(1);
      i1 = j >> 8;
      i2 = j & 0xff;
      row = enc->byte_row(i1);
      col = enc->byte_col(i2);
      if (row < 0 || col < 0) {
        fprintf(stderr, "lost entry for %02x %02x\n", i1, i2);
        exit(1);
      }
      if (scanf(" 0x%x", &enc->charset2uni[row][col]) != 1)
        exit(1);
    }
  } else {
    /* Read a table of hexadecimal Unicode values. */
    for (i1 = 32; i1 < 132; i1++)
      for (i2 = 32; i2 < 132; i2++) {
        i = scanf("%x", &j);
        if (i == EOF)
          goto read_done;
        if (i != 1)
          exit(1);
        if (j < 0 || j == 0xffff)
          j = 0xfffd;
        if (j != 0xfffd) {
          if (enc->byte_row(i1) < 0 || enc->byte_col(i2) < 0) {
            fprintf(stderr, "lost entry at %02x %02x\n", i1, i2);
            exit (1);
          }
          enc->charset2uni[enc->byte_row(i1)][enc->byte_col(i2)] = j;
        }
      }
   read_done: ;
  }
}

/*
 * Computes the charsetpage[0..rows] array.
 */
static void find_charset2uni_pages (Encoding* enc)
{
  int row, col;

  enc->charsetpage = malloc((enc->rows+1)*sizeof(int));

  for (row = 0; row <= enc->rows; row++)
    enc->charsetpage[row] = 0;

  for (row = 0; row < enc->rows; row++) {
    int used = 0;
    for (col = 0; col < enc->cols; col++)
      if (enc->charset2uni[row][col] != 0xfffd)
        used = col+1;
    enc->charsetpage[row] = used;
  }
}

/*
 * Fills in nblocks and blocks.
 */
static void find_charset2uni_blocks (Encoding* enc)
{
  int n, row, lastrow;

  enc->charsetblocks = malloc(enc->rows*sizeof(Block));

  n = 0;
  for (row = 0; row < enc->rows; row++)
    if (enc->charsetpage[row] > 0 && (row == 0 || enc->charsetpage[row-1] == 0)) {
      for (lastrow = row; enc->charsetpage[lastrow+1] > 0; lastrow++);
      enc->charsetblocks[n].start = row * enc->cols;
      enc->charsetblocks[n].end = lastrow * enc->cols + enc->charsetpage[lastrow];
      n++;
    }
  enc->ncharsetblocks = n;
}

/*
 * Outputs the charset to unicode table and function.
 */
static void output_charset2uni (const char* name, Encoding* enc)
{
  int row, col, lastrow, col_max, i, i1_min, i1_max;

  find_charset2uni_pages(enc);

  find_charset2uni_blocks(enc);

  for (row = 0; row < enc->rows; row++)
    if (enc->charsetpage[row] > 0) {
      if (row == 0 || enc->charsetpage[row-1] == 0) {
        /* Start a new block. */
        for (lastrow = row; enc->charsetpage[lastrow+1] > 0; lastrow++);
        printf("static const unsigned short %s_2uni_page%02x[%d] = {\n",
               name, enc->row_byte(row),
               (lastrow-row) * enc->cols + enc->charsetpage[lastrow]);
      }
      printf("  /""* 0x%02x *""/\n ", enc->row_byte(row));
      col_max = (enc->charsetpage[row+1] > 0 ? enc->cols : enc->charsetpage[row]);
      for (col = 0; col < col_max; col++) {
        printf(" 0x%04x,", enc->charset2uni[row][col]);
        if ((col % 8) == 7 && (col+1 < col_max)) printf("\n ");
      }
      printf("\n");
      if (enc->charsetpage[row+1] == 0) {
        /* End a block. */
        printf("};\n");
      }
    }
  printf("\n");

  printf("static int\n");
  printf("%s_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)\n", name);
  printf("{\n");
  printf("  unsigned char c1 = s[0];\n");
  printf("  if (");
  for (i = 0; i < enc->ncharsetblocks; i++) {
    i1_min = enc->row_byte(enc->charsetblocks[i].start / enc->cols);
    i1_max = enc->row_byte((enc->charsetblocks[i].end-1) / enc->cols);
    if (i > 0)
      printf(" || ");
    if (i1_min == i1_max)
      printf("(c1 == 0x%02x)", i1_min);
    else
      printf("(c1 >= 0x%02x && c1 <= 0x%02x)", i1_min, i1_max);
  }
  printf(") {\n");
  printf("    if (n >= 2) {\n");
  printf("      unsigned char c2 = s[1];\n");
  printf("      if (");
  printf(enc->check_col_expr, "c2");
  printf(") {\n");
  printf("        unsigned int i = %d * (", enc->cols);
  printf(enc->byte_row_expr, "c1");
  printf(") + (");
  printf(enc->byte_col_expr, "c2");
  printf(");\n");
  printf("        unsigned short wc = 0xfffd;\n");
  for (i = 0; i < enc->ncharsetblocks; i++) {
    printf("        ");
    if (i > 0)
      printf("} else ");
    if (i < enc->ncharsetblocks-1)
      printf("if (i < %d) ", enc->charsetblocks[i+1].start);
    printf("{\n");
    printf("          if (i < %d)\n", enc->charsetblocks[i].end);
    printf("            wc = %s_2uni_page%02x[i", name, enc->row_byte(enc->charsetblocks[i].start / enc->cols));
    if (enc->charsetblocks[i].start > 0)
      printf("-%d", enc->charsetblocks[i].start);
    printf("];\n");
  }
  printf("        }\n");
  printf("        if (wc != 0xfffd) {\n");
  printf("          *pwc = (ucs4_t) wc;\n");
  printf("          return 2;\n");
  printf("        }\n");
  printf("      }\n");
  printf("      return RET_ILSEQ;\n");
  printf("    }\n");
  printf("    return RET_TOOFEW(0);\n");
  printf("  }\n");
  printf("  return RET_ILSEQ;\n");
  printf("}\n");
  printf("\n");
}

/*
 * Computes the uni2charset[0x0000..0xffff] array.
 */
static void invert (Encoding* enc)
{
  int row, col, j;

  enc->uni2charset = malloc(0x10000*sizeof(int));

  for (j = 0; j < 0x10000; j++)
    enc->uni2charset[j] = 0;

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++) {
      j = enc->charset2uni[row][col];
      if (j != 0xfffd)
        enc->uni2charset[j] = 0x100 * enc->row_byte(row) + enc->col_byte(col);
    }
}

/*
 * Outputs the unicode to charset table and function, using a linear array.
 * (Suitable if the table is dense.)
 */
static void output_uni2charset_dense (const char* name, Encoding* enc)
{
  /* Like in 8bit_tab_to_h.c */
  bool pages[0x100];
  int line[0x2000];
  int tableno;
  struct { int minline; int maxline; int usecount; } tables[0x2000];
  bool first;
  int row, col, j, p, j1, j2, t;

  for (p = 0; p < 0x100; p++)
    pages[p] = false;
  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++) {
      j = enc->charset2uni[row][col];
      if (j != 0xfffd)
        pages[j>>8] = true;
    }
  for (j1 = 0; j1 < 0x2000; j1++) {
    bool all_invalid = true;
    for (j2 = 0; j2 < 8; j2++) {
      j = 8*j1+j2;
      if (enc->uni2charset[j] != 0)
        all_invalid = false;
    }
    if (all_invalid)
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
      if (enc->uni2charset[j] != 0)
        tables[t].usecount++;
  }
  {
    p = -1;
    for (t = 0; t < tableno; t++)
      if (tables[t].usecount > 1) {
        p = tables[t].minline >> 5;
        printf("static const unsigned short %s_page%02x[%d] = {\n", name, p, 8*(tables[t].maxline-tables[t].minline+1));
        for (j1 = tables[t].minline; j1 <= tables[t].maxline; j1++) {
          if ((j1 % 0x20) == 0 && j1 > tables[t].minline)
            printf("  /* 0x%04x */\n", 8*j1);
          printf(" ");
          for (j2 = 0; j2 < 8; j2++) {
            j = 8*j1+j2;
            printf(" 0x%04x,", enc->uni2charset[j]);
          }
          printf(" /*0x%02x-0x%02x*/\n", 8*(j1 % 0x20), 8*(j1 % 0x20)+7);
        }
        printf("};\n");
      }
    if (p >= 0)
      printf("\n");
  }
  printf("static int\n%s_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)\n", name);
  printf("{\n");
  printf("  if (n >= 2) {\n");
  printf("    unsigned short c = 0;\n");
  first = true;
  for (j1 = 0; j1 < 0x2000;) {
    t = line[j1];
    for (j2 = j1; j2 < 0x2000 && line[j2] == t; j2++);
    if (t >= 0) {
      if (j1 != tables[t].minline) abort();
      if (j2 > tables[t].maxline+1) abort();
      j2 = tables[t].maxline+1;
      if (first)
        printf("    ");
      else
        printf("    else ");
      first = false;
      if (tables[t].usecount == 0) abort();
      if (tables[t].usecount == 1) {
        if (j2 != j1+1) abort();
        for (j = 8*j1; j < 8*j2; j++)
          if (enc->uni2charset[j] != 0) {
            printf("if (wc == 0x%04x)\n      c = 0x%02x;\n", j, enc->uni2charset[j]);
            break;
          }
      } else {
        if (j1 == 0) {
          printf("if (wc < 0x%04x)", 8*j2);
        } else {
          printf("if (wc >= 0x%04x && wc < 0x%04x)", 8*j1, 8*j2);
        }
        printf("\n      c = %s_page%02x[wc", name, j1 >> 5);
        if (tables[t].minline > 0)
          printf("-0x%04x", 8*j1);
        printf("];\n");
      }
    }
    j1 = j2;
  }
  printf("    if (c != 0) {\n");
  printf("      r[0] = (c >> 8); r[1] = (c & 0xff);\n");
  printf("      return 2;\n");
  printf("    }\n");
  printf("    return RET_ILSEQ;\n");
  printf("  }\n");
  printf("  return RET_TOOSMALL;\n");
  printf("}\n");
}

/*
 * Outputs the unicode to charset table and function, using a packed array.
 * (Suitable if the table is sparse.)
 */
static void output_uni2charset_sparse (const char* name, Encoding* enc)
{
  bool pages[0x100];
  Block pageblocks[0x100]; int npageblocks;
  int indx2charset[0x10000];
  int summary_indx[0x1000];
  int summary_used[0x1000];
  int i, row, col, j, p, j1, j2, indx;

  /* Fill pages[0x100]. */
  for (p = 0; p < 0x100; p++)
    pages[p] = false;
  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++) {
      j = enc->charset2uni[row][col];
      if (j != 0xfffd)
        pages[j>>8] = true;
    }

#if 0
  for (p = 0; p < 0x100; p++)
    if (pages[p]) {
      printf("static const unsigned short %s_page%02x[256] = {\n", name, p);
      for (j1 = 0; j1 < 32; j1++) {
        printf("  ");
        for (j2 = 0; j2 < 8; j2++)
          printf("0x%04x, ", enc->uni2charset[256*p+8*j1+j2]);
        printf("/""*0x%02x-0x%02x*""/\n", 8*j1, 8*j1+7);
      }
      printf("};\n");
    }
  printf("\n");
#endif

  /* Fill summary_indx[] and summary_used[]. */
  indx = 0;
  for (j1 = 0; j1 < 0x1000; j1++) {
    summary_indx[j1] = indx;
    summary_used[j1] = 0;
    for (j2 = 0; j2 < 16; j2++) {
      j = 16*j1+j2;
      if (enc->uni2charset[j] != 0) {
        indx2charset[indx++] = enc->uni2charset[j];
        summary_used[j1] |= (1 << j2);
      }
    }
  }

  /* Fill npageblocks and pageblocks[]. */
  npageblocks = 0;
  for (p = 0; p < 0x100; ) {
    if (pages[p] && (p == 0 || !pages[p-1])) {
      pageblocks[npageblocks].start = 16*p;
      do p++; while (p < 0x100 && pages[p]);
      j1 = 16*p;
      while (summary_used[j1-1] == 0) j1--;
      pageblocks[npageblocks].end = j1;
      npageblocks++;
    } else
      p++;
  }

  printf("static const unsigned short %s_2charset[%d] = {\n", name, indx);
  for (i = 0; i < indx; ) {
    if ((i % 8) == 0) printf(" ");
    printf(" 0x%04x,", indx2charset[i]);
    i++;
    if ((i % 8) == 0 || i == indx) printf("\n");
  }
  printf("};\n");
  printf("\n");
  for (i = 0; i < npageblocks; i++) {
    printf("static const Summary16 %s_uni2indx_page%02x[%d] = {\n", name,
           pageblocks[i].start/16, pageblocks[i].end-pageblocks[i].start);
    for (j1 = pageblocks[i].start; j1 < pageblocks[i].end; ) {
      if (((16*j1) % 0x100) == 0) printf("  /""* 0x%04x *""/\n", 16*j1);
      if ((j1 % 4) == 0) printf(" ");
      printf(" { %4d, 0x%04x },", summary_indx[j1], summary_used[j1]);
      j1++;
      if ((j1 % 4) == 0 || j1 == pageblocks[i].end) printf("\n");
    }
    printf("};\n");
  }
  printf("\n");

  printf("static int\n");
  printf("%s_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)\n", name);
  printf("{\n");
  printf("  if (n >= 2) {\n");
  printf("    const Summary16 *summary = NULL;\n");
  for (i = 0; i < npageblocks; i++) {
    printf("    ");
    if (i > 0)
      printf("else ");
    printf("if (wc >= 0x%04x && wc < 0x%04x)\n",
           16*pageblocks[i].start, 16*pageblocks[i].end);
    printf("      summary = &%s_uni2indx_page%02x[(wc>>4)", name,
           pageblocks[i].start/16);
    if (pageblocks[i].start > 0)
      printf("-0x%03x", pageblocks[i].start);
    printf("];\n");
  }
  printf("    if (summary) {\n");
  printf("      unsigned short used = summary->used;\n");
  printf("      unsigned int i = wc & 0x0f;\n");
  printf("      if (used & ((unsigned short) 1 << i)) {\n");
  printf("        unsigned short c;\n");
  printf("        /* Keep in `used' only the bits 0..i-1. */\n");
  printf("        used &= ((unsigned short) 1 << i) - 1;\n");
  printf("        /* Add `summary->indx' and the number of bits set in `used'. */\n");
  printf("        used = (used & 0x5555) + ((used & 0xaaaa) >> 1);\n");
  printf("        used = (used & 0x3333) + ((used & 0xcccc) >> 2);\n");
  printf("        used = (used & 0x0f0f) + ((used & 0xf0f0) >> 4);\n");
  printf("        used = (used & 0x00ff) + (used >> 8);\n");
  printf("        c = %s_2charset[summary->indx + used];\n", name);
  printf("        r[0] = (c >> 8); r[1] = (c & 0xff);\n");
  printf("        return 2;\n");
  printf("      }\n");
  printf("    }\n");
  printf("    return RET_ILSEQ;\n");
  printf("  }\n");
  printf("  return RET_TOOSMALL;\n");
  printf("}\n");
}

/* ISO-2022/EUC specifics */

static int row_byte_normal (int row) { return 0x21+row; }
static int col_byte_normal (int col) { return 0x21+col; }
static int byte_row_normal (int byte) { return byte-0x21; }
static int byte_col_normal (int byte) { return byte-0x21; }

static void do_normal (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 94;
  enc.row_byte = row_byte_normal;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_normal;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc);
}

/* Note: On first sight, the jisx0212_2charset[] table seems to be in order,
   starting from the charset=0x3021/uni=0x4e02 pair. But it's only mostly in
   order. There are 75 out-of-order values, scattered all throughout the table.
 */

static void do_normal_only_charset2uni (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 94;
  enc.row_byte = row_byte_normal;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_normal;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table(&enc);
  output_charset2uni(name,&enc);
}

/* CNS 11643 specifics - trick to put two tables into one */

static int row_byte_cns11643 (int row) {
  return 0x100 * (row / 94) + (row % 94) + 0x21;
}
static int byte_row_cns11643 (int byte) {
  return (byte >= 0x100 && byte < 0x200 ? byte-0x121 :
          byte >= 0x200 && byte < 0x300 ? byte-0x221+94 :
          byte >= 0x300 && byte < 0x400 ? byte-0x321+2*94 :
          -1);
}

static void do_cns11643_only_uni2charset (const char* name)
{
  Encoding enc;
  int j, x;

  enc.rows = 3*94;
  enc.cols = 94;
  enc.row_byte = row_byte_cns11643;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_cns11643;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table(&enc);
  invert(&enc);
  /* Move the 2 plane bits into the unused bits 15 and 7. */
  for (j = 0; j < 0x10000; j++) {
    x = enc.uni2charset[j];
    if (x != 0) {
      if (x & 0x8080) abort();
      switch (x >> 16) {
        case 0: /* plane 1 */ x = (x & 0xffff) | 0x0000; break;
        case 1: /* plane 2 */ x = (x & 0xffff) | 0x0080; break;
        case 2: /* plane 3 */ x = (x & 0xffff) | 0x8000; break;
        default: abort();
      }
      enc.uni2charset[j] = x;
    }
  }
  output_uni2charset_sparse(name,&enc);
}

/* GBK specifics */

static int row_byte_gbk1 (int row) {
  return 0x81+row;
}
static int col_byte_gbk1 (int col) {
  return (col >= 0x3f ? 0x41 : 0x40) + col;
}
static int byte_row_gbk1 (int byte) {
  if (byte >= 0x81 && byte < 0xff)
    return byte-0x81;
  else
    return -1;
}
static int byte_col_gbk1 (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0x80 && byte < 0xff)
    return byte-0x41;
  else
    return -1;
}

static void do_gbk1 (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 190;
  enc.row_byte = row_byte_gbk1;
  enc.col_byte = col_byte_gbk1;
  enc.byte_row = byte_row_gbk1;
  enc.byte_col = byte_col_gbk1;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_dense(name,&enc);
}

static void do_gbk1_only_charset2uni (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 190;
  enc.row_byte = row_byte_gbk1;
  enc.col_byte = col_byte_gbk1;
  enc.byte_row = byte_row_gbk1;
  enc.byte_col = byte_col_gbk1;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
}

static int row_byte_gbk2 (int row) {
  return 0x81+row;
}
static int col_byte_gbk2 (int col) {
  return (col >= 0x3f ? 0x41 : 0x40) + col;
}
static int byte_row_gbk2 (int byte) {
  if (byte >= 0x81 && byte < 0xff)
    return byte-0x81;
  else
    return -1;
}
static int byte_col_gbk2 (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0x80 && byte < 0xa1)
    return byte-0x41;
  else
    return -1;
}

static void do_gbk2_only_charset2uni (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 96;
  enc.row_byte = row_byte_gbk2;
  enc.col_byte = col_byte_gbk2;
  enc.byte_row = byte_row_gbk2;
  enc.byte_col = byte_col_gbk2;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xa1)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
}

static void do_gbk1_only_uni2charset (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 190;
  enc.row_byte = row_byte_gbk1;
  enc.col_byte = col_byte_gbk1;
  enc.byte_row = byte_row_gbk1;
  enc.byte_col = byte_col_gbk1;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc);
}

/* KSC 5601 specifics */

/*
 * Reads the charset2uni table from standard input.
 */
static void read_table_ksc5601 (Encoding* enc)
{
  int row, col, i, i1, i2, c, j;

  enc->charset2uni = malloc(enc->rows*sizeof(int*));
  for (row = 0; row < enc->rows; row++)
    enc->charset2uni[row] = malloc(enc->cols*sizeof(int));

  for (row = 0; row < enc->rows; row++)
    for (col = 0; col < enc->cols; col++)
      enc->charset2uni[row][col] = 0xfffd;

  c = getc(stdin);
  ungetc(c,stdin);
  if (c == '#') {
    /* Read a unicode.org style .TXT file. */
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
      if (scanf("0x%x", &j) != 1)
        exit(1);
      i1 = j >> 8;
      i2 = j & 0xff;
      if (scanf(" 0x%x", &j) != 1)
        exit(1);
      /* Take only the range covered by KS C 5601.1987-0 = KS C 5601.1989-0
         = KS X 1001.1992, ignore the rest. */
      if (!(i1 >= 128+33 && i1 < 128+127 && i2 >= 128+33 && i2 < 128+127))
        continue;  /* KSC5601 specific */
      i1 &= 0x7f;  /* KSC5601 specific */
      i2 &= 0x7f;  /* KSC5601 specific */
      row = enc->byte_row(i1);
      col = enc->byte_col(i2);
      if (row < 0 || col < 0) {
        fprintf(stderr, "lost entry for %02x %02x\n", i1, i2);
        exit(1);
      }
      enc->charset2uni[row][col] = j;
    }
  } else {
    /* Read a table of hexadecimal Unicode values. */
    for (i1 = 33; i1 < 127; i1++)
      for (i2 = 33; i2 < 127; i2++) {
        i = scanf("%x", &j);
        if (i == EOF)
          goto read_done;
        if (i != 1)
          exit(1);
        if (j < 0 || j == 0xffff)
          j = 0xfffd;
        if (j != 0xfffd) {
          if (enc->byte_row(i1) < 0 || enc->byte_col(i2) < 0) {
            fprintf(stderr, "lost entry at %02x %02x\n", i1, i2);
            exit (1);
          }
          enc->charset2uni[enc->byte_row(i1)][enc->byte_col(i2)] = j;
        }
      }
   read_done: ;
  }
}

static void do_ksc5601 (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 94;
  enc.row_byte = row_byte_normal;
  enc.col_byte = col_byte_normal;
  enc.byte_row = byte_row_normal;
  enc.byte_col = byte_col_normal;
  enc.check_row_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.check_col_expr = "%1$s >= 0x21 && %1$s < 0x7f";
  enc.byte_row_expr = "%1$s - 0x21";
  enc.byte_col_expr = "%1$s - 0x21";

  read_table_ksc5601(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc);
}

/* Big5 specifics */

static int row_byte_big5 (int row) {
  return 0xa1+row;
}
static int col_byte_big5 (int col) {
  return (col >= 0x3f ? 0x62 : 0x40) + col;
}
static int byte_row_big5 (int byte) {
  if (byte >= 0xa1 && byte < 0xff)
    return byte-0xa1;
  else
    return -1;
}
static int byte_col_big5 (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0xa1 && byte < 0xff)
    return byte-0x62;
  else
    return -1;
}

static void do_big5 (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 157;
  enc.row_byte = row_byte_big5;
  enc.col_byte = col_byte_big5;
  enc.byte_row = byte_row_big5;
  enc.byte_col = byte_col_big5;
  enc.check_row_expr = "%1$s >= 0xa1 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0xa1 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0xa1";
  enc.byte_col_expr = "%1$s - (%1$s >= 0xa1 ? 0x62 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc);
}

/* Big5-HKSCS specifics */

static int row_byte_big5hkscs (int row) {
  return 0x81+row;
}
static int col_byte_big5hkscs (int col) {
  return (col >= 0x3f ? 0x62 : 0x40) + col;
}
static int byte_row_big5hkscs (int byte) {
  if (byte >= 0x81 && byte < 0xff)
    return byte-0x81;
  else
    return -1;
}
static int byte_col_big5hkscs (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0xa1 && byte < 0xff)
    return byte-0x62;
  else
    return -1;
}

static void do_big5hkscs (const char* name)
{
  Encoding enc;

  enc.rows = 126;
  enc.cols = 157;
  enc.row_byte = row_byte_big5hkscs;
  enc.col_byte = col_byte_big5hkscs;
  enc.byte_row = byte_row_big5hkscs;
  enc.byte_col = byte_col_big5hkscs;
  enc.check_row_expr = "%1$s >= 0x81 && %1$s < 0xff";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0xa1 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x81";
  enc.byte_col_expr = "%1$s - (%1$s >= 0xa1 ? 0x62 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc);
}

/* Johab Hangul specifics */

static int row_byte_johab_hangul (int row) {
  return 0x84+row;
}
static int col_byte_johab_hangul (int col) {
  return (col >= 0x3e ? 0x43 : 0x41) + col;
}
static int byte_row_johab_hangul (int byte) {
  if (byte >= 0x84 && byte < 0xd4)
    return byte-0x84;
  else
    return -1;
}
static int byte_col_johab_hangul (int byte) {
  if (byte >= 0x41 && byte < 0x7f)
    return byte-0x41;
  else if (byte >= 0x81 && byte < 0xff)
    return byte-0x43;
  else
    return -1;
}

static void do_johab_hangul (const char* name)
{
  Encoding enc;

  enc.rows = 80;
  enc.cols = 188;
  enc.row_byte = row_byte_johab_hangul;
  enc.col_byte = col_byte_johab_hangul;
  enc.byte_row = byte_row_johab_hangul;
  enc.byte_col = byte_col_johab_hangul;
  enc.check_row_expr = "%1$s >= 0x84 && %1$s < 0xd4";
  enc.check_col_expr = "(%1$s >= 0x41 && %1$s < 0x7f) || (%1$s >= 0x81 && %1$s < 0xff)";
  enc.byte_row_expr = "%1$s - 0x84";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x81 ? 0x43 : 0x41)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_dense(name,&enc);
}

/* SJIS specifics */

static int row_byte_sjis (int row) {
  return (row >= 0x1f ? 0xc1 : 0x81) + row;
}
static int col_byte_sjis (int col) {
  return (col >= 0x3f ? 0x41 : 0x40) + col;
}
static int byte_row_sjis (int byte) {
  if (byte >= 0x81 && byte < 0xa0)
    return byte-0x81;
  else if (byte >= 0xe0)
    return byte-0xc1;
  else
    return -1;
}
static int byte_col_sjis (int byte) {
  if (byte >= 0x40 && byte < 0x7f)
    return byte-0x40;
  else if (byte >= 0x80 && byte < 0xfd)
    return byte-0x41;
  else
    return -1;
}

static void do_sjis (const char* name)
{
  Encoding enc;

  enc.rows = 94;
  enc.cols = 188;
  enc.row_byte = row_byte_sjis;
  enc.col_byte = col_byte_sjis;
  enc.byte_row = byte_row_sjis;
  enc.byte_col = byte_col_sjis;
  enc.check_row_expr = "(%1$s >= 0x81 && %1$s < 0xa0) || (%1$s >= 0xe0)";
  enc.check_col_expr = "(%1$s >= 0x40 && %1$s < 0x7f) || (%1$s >= 0x80 && %1$s < 0xfd)";
  enc.byte_row_expr = "%1$s - (%1$s >= 0xe0 ? 0xc1 : 0x81)";
  enc.byte_col_expr = "%1$s - (%1$s >= 0x80 ? 0x41 : 0x40)";

  read_table(&enc);
  output_charset2uni(name,&enc);
  invert(&enc); output_uni2charset_sparse(name,&enc);
}

/* Main program */

int main (int argc, char *argv[])
{
  const char* charsetname;
  const char* name;

  if (argc != 3)
    exit(1);
  charsetname = argv[1];
  name = argv[2];

  output_title(charsetname);

  if (!strcmp(name,"gb2312") || !strcmp(name,"gb12345ext")
      || !strcmp(name,"jisx0208") || !strcmp(name,"jisx0212"))
    do_normal(name);
  else if (!strcmp(name,"cns11643_1") || !strcmp(name,"cns11643_2")
           || !strcmp(name,"cns11643_3"))
    do_normal_only_charset2uni(name);
  else if (!strcmp(name,"cns11643_inv"))
    do_cns11643_only_uni2charset(name);
  else if (!strcmp(name,"gbkext1"))
    do_gbk1_only_charset2uni(name);
  else if (!strcmp(name,"gbkext2"))
    do_gbk2_only_charset2uni(name);
  else if (!strcmp(name,"gbkext_inv"))
    do_gbk1_only_uni2charset(name);
  else if (!strcmp(name,"cp936ext"))
    do_gbk1(name);
  else if (!strcmp(name,"ksc5601"))
    do_ksc5601(name);
  else if (!strcmp(name,"big5") || !strcmp(name,"cp950ext"))
    do_big5(name);
  else if (!strcmp(name,"big5hkscs"))
    do_big5hkscs(name);
  else if (!strcmp(name,"johab_hangul"))
    do_johab_hangul(name);
  else if (!strcmp(name,"cp932ext"))
    do_sjis(name);
  else
    exit(1);

  return 0;
}
