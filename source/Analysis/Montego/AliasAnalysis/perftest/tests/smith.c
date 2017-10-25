
typedef unsigned int size_t;
typedef long long __quad_t;
typedef int __off_t;
typedef __quad_t __off64_t;
typedef int __clock_t;
typedef int __time_t;
typedef struct _IO_FILE FILE;
typedef void _IO_lock_t;
struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;
  int _pos;
};
struct _IO_FILE {
  int _flags;
  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;
  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;
  struct _IO_marker *_markers;
  struct _IO_FILE *_chain;
  int _fileno;
  int _flags2;
  __off_t _old_offset;
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];
  _IO_lock_t *_lock;
  __off64_t _offset;
  void *__pad1;
  void *__pad2;
  void *__pad3;
  void *__pad4;
  size_t __pad5;
  int _mode;
  char _unused2[40];
};
int  fclose(FILE *__stream);
FILE * fopen(char const * restrict __filename, char const * restrict __modes);
int  fprintf(FILE * restrict __stream, char const * restrict __format, ...);
int  printf(char const * restrict __format, ...);
int  fscanf(FILE * restrict __stream, char const * restrict __format, ...);
int  fgetc(FILE *__stream);
int  fseek(FILE *__stream, int __off, int __whence);
int  ftell(FILE *__stream);
int  feof(FILE *__stream);
void * malloc(size_t __size);
void * calloc(size_t __nmemb, size_t __size);
void  free(void *__ptr);
enum __uni213anon{
  _ISupper = 256,
  _ISlower = 512,
  _ISalpha = 1024,
  _ISdigit = 2048,
  _ISxdigit = 4096,
  _ISspace = 8192,
  _ISprint = 16384,
  _ISgraph = 32768,
  _ISblank = 1,
  _IScntrl = 2,
  _ISpunct = 4,
  _ISalnum = 8,
};
unsigned short const ** __ctype_b_loc();
int  toupper(int __c);
typedef __clock_t clock_t;
typedef __time_t time_t;
clock_t  clock();
int Kol_block_v_stroke;
int Kol_block_v_stolbce;
char *String1;
char *String2;
int M;
int N;
struct MaxElement;
struct __uni214anon {
  int BLOCKI;
  int BLOCKJ;
  int maxi;
  int maxj;
  struct MaxElement *next;
};
typedef struct __uni214anon *PMaxElement;
struct MAXIMAL;
struct __uni215anon {
  int max;
  int maxi;
  int maxj;
  struct MAXIMAL *next;
};
typedef struct __uni215anon *PMAX;
struct __uni216anon {
  int *col;
  int *row;
  PMAX el_max;
};
typedef struct __uni216anon BLOCK;
typedef struct __uni216anon *PBLOCK;
struct String;
struct __uni217anon {
  char inf;
  struct String *next;
};
typedef struct __uni217anon *PString;
PString  Cons(char x, PString L)
{
  PString T1;
  T1 = (PString )malloc(8);
  (T1[0]).inf = x;
  (T1[0]).next = (struct String *)L;
  return T1;
}
inline int  max_from_two(int x, int y)
{
  int __uni219;
  if (x > y)
  {
    __uni219 = x;
  }
  else
  {
    __uni219 = y;
  }
  return __uni219;
}
inline int  min_from_two(int x, int y)
{
  int __uni220;
  if (x < y)
  {
    __uni220 = x;
  }
  else
  {
    __uni220 = y;
  }
  return __uni220;
}
int  size_of_file(FILE *fileS1)
{
  int size_file;
  fseek(fileS1, 0, 2);
  size_file = ftell(fileS1);
  fseek(fileS1, 0, 0);
  return size_file;
}
int  read_file(FILE *file_string, char *String)
{
  char b;
  int i;
  i = 0;
  while (!feof(file_string))
  {
    unsigned short const **__uni221;
    b = fgetc(file_string);
    if (b)
    {
      int c;
      c = toupper(b);
      switch (c)
      {
      case 65:
        String[i] = 0;
        goto __uni218l;
      case 71:
        String[i] = 1;
        goto __uni218l;
      case 67:
        String[i] = 2;
        goto __uni218l;
      case 84:
        String[i] = 3;
        goto __uni218l;
      case 45:
        String[i] = 4;
        goto __uni218l;
      case 78:
        String[i] = 5;
        goto __uni218l;
      }
      __uni218l: 
      ;
      i = i + 1;
    }
  }
  return i;
}
FILE * write_to_file(FILE *file1, PString Str1, int x, int y)
{
  char i2c[6] = {'A', 'G', 'C', 'T', '-', 'N'};
  PString Str;
  fprintf(file1, " %c", '*');
  while (Str1)
  {
    fprintf(file1, "%c", (i2c[((Str1[0]).inf)]));
    Str = Str1;
    Str1 = (PString )(Str1[0]).next;
    free(Str);
  }
  fprintf(file1, " { %i  %i }", x, y);
  fprintf(file1, "%c", 10);
  return file1;
}
PMaxElement  maxel(BLOCK **mas, PMaxElement B, int *p)
{
  int i;
  int j;
  int maximal;
  int maximalI;
  int maximalJ;
  PMaxElement m;
  maximal = 0;
  maximalI = 0;
  maximalJ = 0;
  for (i = 0; i < Kol_block_v_stolbce; i = i + 1)
  {
    for (j = 0; j < Kol_block_v_stroke; j = j + 1)
    {
      if ((((mas[i][j]).el_max)[0]).max >= maximal)
      {
        maximal = (((mas[i][j]).el_max)[0]).max;
        maximalI = (((mas[i][j]).el_max)[0]).maxi;
        maximalJ = (((mas[i][j]).el_max)[0]).maxj;
      }
    }
  }
  printf(" TT %i ", maximal);
  for (i = 0; i < Kol_block_v_stolbce; i = i + 1)
  {
    for (j = 0; j < Kol_block_v_stroke; j = j + 1)
    {
      if ((((mas[i][j]).el_max)[0]).max == maximal)
      {
        while ((mas[i][j]).el_max)
        {
          m = (PMaxElement )malloc(20);
          (m[0]).BLOCKI = i;
          (m[0]).BLOCKJ = j;
          (m[0]).maxi = (((mas[i][j]).el_max)[0]).maxi - 1;
          (m[0]).maxj = (((mas[i][j]).el_max)[0]).maxj - 1;
          (m[0]).next = (struct MaxElement *)B;
          B = m;
          (mas[i][j]).el_max = (PMAX )(((mas[i][j]).el_max)[0]).next;
        }
      }
    }
  }
  p[0] = maximal;
  return m;
}
inline int  differences(char S1, char S2, char (*BLOSUM)[6])
{
  int difference;
  difference = BLOSUM[S1][S2];
  return difference;
}
inline int  similarities(char S2, char (*BLOSUM)[6])
{
  return BLOSUM[S2][S2];
}
unsigned char ** recollect(int *rows, int *cols, int diag, int M_bl, int N_bl, unsigned char **P, char (*BLOSUM)[6])
{
  int **h;
  unsigned char previous;
  int wp1;
  int wp2;
  int i;
  int j;
  int Size_colomn;
  int Size_row;
  int w;
  int max;
  int __uni224;
  if (N % 1024 != 0 && N_bl == Kol_block_v_stroke - 1)
  {
    Size_row = N % 1024;
  }
  else
  {
    Size_row = 1024;
  }
  if (M % 1024 != 0 && M_bl == Kol_block_v_stolbce - 1)
  {
    Size_colomn = M % 1024;
  }
  else
  {
    Size_colomn = 1024;
  }
  __uni224 = (Size_colomn + 1) * 4;
  h = (int **)malloc(__uni224);
  for (i = 0; i < Size_colomn + 1; i = i + 1)
  {
    int __uni225;
    __uni225 = (Size_row + 1) * 4;
    h[i] = (int *)malloc(__uni225);
  }
  h[0][0] = diag;
  for (i = 1; i <= Size_colomn; i = i + 1)
  {
    h[i][0] = cols[(i - 1)];
  }
  for (j = 1; j <= Size_row; j = j + 1)
  {
    h[0][j] = rows[(j - 1)];
  }
  for (i = 1; i <= Size_colomn; i = i + 1)
  {
    for (j = 1; j <= Size_row; j = j + 1)
    {
      max = 0;
      previous = 0;
      if (String1[((i - 1) + M_bl * 1024)] == String2[((j - 1) + N_bl * 1024)])
      {
        w = similarities((String2[((j - 1) + N_bl * 1024)]), BLOSUM);
      }
      else
      {
        w = differences((String1[((i - 1) + M_bl * 1024)]), (String2[((j - 1) + N_bl * 1024)]), BLOSUM);
      }
      if (String1[((i - 1) + M_bl * 1024)] != 4)
      {
        wp1 = differences((String1[((i - 1) + M_bl * 1024)]), 4, BLOSUM);
      }
      else
      {
        wp1 = similarities(4, BLOSUM);
      }
      if (String2[((j - 1) + N_bl * 1024)] != 4)
      {
        wp2 = differences(4, (String2[((j - 1) + N_bl * 1024)]), BLOSUM);
      }
      else
      {
        wp2 = similarities(4, BLOSUM);
      }
      if (h[(i - 1)][(j - 1)] + w > max)
      {
        max = h[(i - 1)][(j - 1)] + w;
        previous = 2;
      }
      if (h[i][(j - 1)] + wp2 > max)
      {
        max = h[i][(j - 1)] + wp2;
        previous = 1;
      }
      if (h[(i - 1)][j] + wp1 > max)
      {
        max = h[(i - 1)][j] + wp1;
        previous = 3;
      }
      h[i][j] = max;
      P[(i - 1)][(j - 1)] = previous;
    }
  }
  for (j = 0; j < Size_colomn; j = j + 1)
  {
    free((h[j]));
  }
  free(h);
  return P;
}
int  main()
{
  FILE *fileS1;
  FILE *fileS2;
  int size_file;
  int c;
  int i;
  int j;
  int i1;
  int j1;
  int i2;
  int j2;
  int i3;
  int j3;
  char BLOSUM[6][6];
  int *row1;
  unsigned char **Prev;
  int space;
  int dif;
  int sim;
  int score;
  int *p;
  int penalty;
  int endI;
  int endJ;
  int beginI;
  int beginJ;
  int tmp;
  PString Str1;
  PString Str2;
  BLOCK **mat;
  PMaxElement L;
  time_t time1;
  time_t time2;
  int __uni226;
  int __uni227;
  int *__uni228;
  int __uni230;
  int __uni242;
  int __uni243;
  time1 = clock();
  fileS1 = fopen("string1.txt", "rb");
  size_file = size_of_file(fileS1);
  __uni226 = size_file * 1;
  String1 = (char *)malloc(__uni226);
  M = read_file(fileS1, String1);
  fclose(fileS1);
  fileS2 = fopen("string2.txt", "rb");
  size_file = size_of_file(fileS2);
  __uni227 = size_file * 1;
  String2 = (char *)malloc(__uni227);
  N = read_file(fileS2, String2);
  fclose(fileS2);
  printf("\n Length of First string is  %i\n", M);
  printf("\n Length of Second string is  %i\n", N);
  fileS1 = fopen("BLOSUM.txt", "r");
  __uni228 = &penalty;
  tmp = fscanf(fileS1, " %i", __uni228);
  for (i = 0; i < 5; i = i + 1)
  {
    for (j = 0; j < 5; j = j + 1)
    {
      int *__uni229;
      __uni229 = &c;
      tmp = fscanf(fileS1, " %i", __uni229);
      while (c != ' ' && !feof(fileS1))
      {
        BLOSUM[i][j] = c;
        c = fgetc(fileS1);
      }
    }
  }
  for (i = 0; i < 6; i = i + 1)
  {
    BLOSUM[5][i] = penalty / 2;
    BLOSUM[i][5] = penalty / 2;
  }
  fclose(fileS1);
  if (N % 1024 == 0)
  {
    Kol_block_v_stroke = N / 1024;
  }
  else
  {
    Kol_block_v_stroke = N / 1024 + 1;
  }
  if (M % 1024 == 0)
  {
    Kol_block_v_stolbce = M / 1024;
  }
  else
  {
    Kol_block_v_stolbce = M / 1024 + 1;
  }
  __uni230 = Kol_block_v_stolbce * 4;
  mat = (BLOCK **)malloc(__uni230);
  for (i = 0; i < Kol_block_v_stolbce; i = i + 1)
  {
    int __uni231;
    __uni231 = Kol_block_v_stroke * 12;
    mat[i] = (BLOCK *)malloc(__uni231);
  }
  row1 = (int *)malloc((1024 * 4));
  for (i = 0; i < 1024; i = i + 1)
  {
    row1[i] = 0;
  }
  for (j = 0; j <= (Kol_block_v_stolbce + Kol_block_v_stroke) - 2; j = j + 1)
  {
    int __uni232;
    int __uni233;
	int loop_from, loop_to;
    __uni232 = (j - Kol_block_v_stroke) + 1;
	loop_from = max_from_two(0, __uni232);
	loop_to = min_from_two(Kol_block_v_stolbce, j);

    for (i = loop_from; i <= loop_to; i = i + 1)
    {
          {
            int **__uni259h;
            int __uni260wp1;
            int __uni261wp2;
            int __uni262i;
            int __uni263j;
            int __uni264Size_colomn;
            int __uni265Size_row;
            int __uni266max;
            int __uni267maximum;
            int __uni272__uni222;
            if (N % 1024 != 0 && (j-i) == Kol_block_v_stroke - 1)
            {
              __uni265Size_row = N % 1024;
            }
            else
            {
              __uni265Size_row = 1024;
            }
            if (M % 1024 != 0 && i == Kol_block_v_stolbce - 1)
            {
              __uni264Size_colomn = M % 1024;
            }
            else
            {
              __uni264Size_colomn = 1024;
            }
            __uni272__uni222 = (__uni264Size_colomn + 1) * 4;
            __uni259h = (int **)malloc(__uni272__uni222);
            for (__uni262i = 0; __uni262i < __uni264Size_colomn + 1; __uni262i = __uni262i + 1)
            {
              int __uni273__uni223;
              __uni273__uni223 = (__uni265Size_row + 1) * 4;
              __uni259h[__uni262i] = (int *)malloc(__uni273__uni223);
            }
            __uni259h[0][0] = 0;
            for (__uni262i = 1; __uni262i <= __uni264Size_colomn; __uni262i = __uni262i + 1)
            {
              __uni259h[__uni262i][0] = row1[(__uni262i - 1)];
            }
            for (__uni263j = 1; __uni263j <= __uni265Size_row; __uni263j = __uni263j + 1)
            {
              __uni259h[0][__uni263j] = row1[(__uni263j - 1)];
            }
            __uni267maximum = 0;
            (mat[i][(j - i)]).row = (int *)calloc(1024, 4);
            (mat[i][(j - i)]).col = (int *)calloc(1024, 4);
            for (__uni262i = 1; __uni262i <= __uni264Size_colomn; __uni262i = __uni262i + 1)
            {
              int __uni270C1;
              __uni270C1 = String1[((__uni262i - 1) + i * 1024)];
              for (__uni263j = 1; __uni263j <= __uni265Size_row; __uni263j = __uni263j + 1)
              {
                int __uni271C2;
                __uni271C2 = String2[((__uni263j - 1) + (j-i) * 1024)];
                __uni260wp1 = BLOSUM[4][__uni270C1];
                __uni261wp2 = BLOSUM[4][__uni271C2];
                __uni266max = __uni259h[(__uni262i - 1)][(__uni263j - 1)] + BLOSUM[__uni270C1][__uni271C2];
                if (__uni266max < __uni259h[__uni262i][(__uni263j - 1)] + __uni261wp2)
                {
                  __uni266max = __uni259h[__uni262i][(__uni263j - 1)] + __uni261wp2;
                }
                if (__uni266max < __uni259h[(__uni262i - 1)][__uni263j] + __uni260wp1)
                {
                  __uni266max = __uni259h[(__uni262i - 1)][__uni263j] + __uni260wp1;
                }
                __uni259h[__uni262i][__uni263j] = __uni266max;
                if (__uni266max > __uni267maximum)
                {
                  __uni267maximum = __uni266max;
                }
              }
            }
            if (__uni264Size_colomn == 1024)
            {
              for (__uni263j = 1; __uni263j <= __uni265Size_row; __uni263j = __uni263j + 1)
              {
                ((mat[i][(j - i)]).row)[(__uni263j - 1)] = __uni259h[1024][__uni263j];
              }
            }
            if (__uni265Size_row == 1024)
            {
              for (__uni262i = 1; __uni262i <= __uni264Size_colomn; __uni262i = __uni262i + 1)
              {
                ((mat[i][(j - i)]).col)[(__uni262i - 1)] = __uni259h[__uni262i][1024];
              }
            }
            (mat[i][(j - i)]).el_max = (void *)0;
            for (__uni262i = 1; __uni262i <= __uni264Size_colomn; __uni262i = __uni262i + 1)
            {
              for (__uni263j = 1; __uni263j <= __uni265Size_row; __uni263j = __uni263j + 1)
              {
                if (__uni259h[__uni262i][__uni263j] == __uni267maximum)
                {
                  PMAX __uni279__uni249;
                  __uni279__uni249 = (PMAX )malloc(16);
                  (__uni279__uni249[0]).max = __uni259h[__uni262i][__uni263j];
                  (__uni279__uni249[0]).maxi = __uni262i;
                  (__uni279__uni249[0]).maxj = __uni263j;
                  (__uni279__uni249[0]).next = (mat[i][(j - i)]).el_max;
                  (mat[i][(j - i)]).el_max = __uni279__uni249;
                }
              }
            }
            for (__uni263j = 0; __uni263j < __uni264Size_colomn; __uni263j = __uni263j + 1)
            {
              free((__uni259h[__uni263j]));
            }
            free(__uni259h);
            __uni280l: 
            ;
          }
    }
  }
  __uni242 = (int )(clock() - time1);
  printf("Fill matrix time =  %i\n", __uni242);
  L = (void *)0;
  p = &score;
  L = maxel(mat, L, p);
  printf("SCORE %i ", score);
  printf("Alignment\n");
  fileS1 = fopen("newS1.txt", "w");
  fileS2 = fopen("newS2.txt", "w");
  Prev = (unsigned char **)calloc(1024, 4);
  for (i = 0; i < 1024; i = i + 1)
  {
    Prev[i] = (unsigned char *)calloc(1024, 1);
  }
  while (L)
  {
    space = 0;
    dif = 0;
    sim = 0;
    if (L)
    {
      Str1 = (void *)0;
      Str2 = (void *)0;
      i = (L[0]).maxi + 1024 * (L[0]).BLOCKI;
      j = (L[0]).maxj + 1024 * (L[0]).BLOCKJ;
      printf(" %i %i ", i, j);
      i1 = (L[0]).BLOCKI;
      j1 = (L[0]).BLOCKJ;
      if ((L[0]).BLOCKI == 0)
      {
        if ((L[0]).BLOCKJ == 0)
        {
          Prev = recollect(row1, row1, 0, i1, j1, Prev, BLOSUM);
        }
        else
        {
          Prev = recollect(row1, ((mat[i1][(j1 - 1)]).col), 0, i1, j1, Prev, BLOSUM);
        }
      }
      else
      {
        if ((L[0]).BLOCKJ == 0)
        {
          Prev = recollect(((mat[(i1 - 1)][j1]).row), row1, 0, i1, j1, Prev, BLOSUM);
        }
        else
        {
          Prev = recollect(((mat[(i1 - 1)][j1]).row), ((mat[i1][(j1 - 1)]).col), (((mat[(i1 - 1)][(j1 - 1)]).col)[(1024 - 1)]), i1, j1, Prev, BLOSUM);
        }
      }
      endI = i + 1;
      endJ = j + 1;
      i2 = (L[0]).maxi;
      j2 = (L[0]).maxj;
      printf("Alignment\n");
      while ((((Prev[i2][j2] != 0 && i2 != -1) && j2 != -1) && i >= 0) && j >= 0)
      {
        beginI = i + 1;
        beginJ = j + 1;
        if ((i2 >= 0 && j2 >= 0) && Prev[i2][j2] == 3)
        {
          Str1 = Cons(4, Str1);
          Str2 = Cons((String1[i]), Str2);
          space = space + 1;
          i = i - 1;
          i2 = i2 - 1;
        }
        if ((i2 >= 0 && j2 >= 0) && Prev[i2][j2] == 2)
        {
          Str1 = Cons((String2[j]), Str1);
          Str2 = Cons((String1[i]), Str2);
          i = i - 1;
          j = j - 1;
          i2 = i2 - 1;
          j2 = j2 - 1;
          if ((Str1[0]).inf == (Str2[0]).inf)
          {
            sim = sim + 1;
          }
          else
          {
            dif = dif + 1;
          }
        }
        if ((i2 >= 0 && j2 >= 0) && Prev[i2][j2] == 1)
        {
          Str2 = Cons(4, Str2);
          Str1 = Cons((String2[j]), Str1);
          space = space + 1;
          j = j - 1;
          j2 = j2 - 1;
        }
        if (i2 == -1 || j2 == -1)
        {
          i3 = i1;
          j3 = j1;
          i1 = i / 1024;
          j1 = j / 1024;
          if (i >= 0 && j >= 0)
          {
            if (i1 == 0)
            {
              if (j1 == 0)
              {
                Prev = recollect(row1, row1, 0, i1, j1, Prev, BLOSUM);
              }
              else
              {
                Prev = recollect(row1, ((mat[i1][(j1 - 1)]).col), 0, i1, j1, Prev, BLOSUM);
              }
            }
            else
            {
              if (j1 == 0)
              {
                Prev = recollect(((mat[(i1 - 1)][j1]).row), row1, 0, i1, j1, Prev, BLOSUM);
              }
              else
              {
                Prev = recollect(((mat[(i1 - 1)][j1]).row), ((mat[i1][(j1 - 1)]).col), (((mat[(i1 - 1)][(j1 - 1)]).col)[(1024 - 1)]), i1, j1, Prev, BLOSUM);
              }
            }
          }
        }
        if (i2 == -1)
        {
          i2 = 1024 - 1;
        }
        if (j2 == -1)
        {
          j2 = 1024 - 1;
        }
      }
      printf("\n First string : \n");
      fileS1 = write_to_file(fileS1, Str2, beginI, endI);
      printf("\n Second string : \n");
      fileS2 = write_to_file(fileS2, Str1, beginJ, endJ);
      printf("\n Score = %i ", score);
      printf(" \n Simularities = %i", sim);
      printf("\n Differences = %i ", dif);
      printf(" \n Spaces = %i \n", space);
      printf(" Position of alignment at First string %i %i", beginI, endI);
      printf(" \n Position of alignment at Second string %i %i \n", beginJ, endJ);
      L = (PMaxElement )(L[0]).next;
    }
  }
  time2 = clock();
  __uni243 = (int )(time2 - time1);
  printf("\n Time =  %i  ", __uni243);
  fclose(fileS1);
  fclose(fileS2);
  for (j = 0; j < 1024; j = j + 1)
  {
    free((Prev[j]));
  }
  free(Prev);
  for (j = 0; j < Kol_block_v_stolbce; j = j + 1)
  {
    free((mat[j]));
  }
  free(mat);
  return 0;
}