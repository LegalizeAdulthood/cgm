#if !defined(GKS_CORE_H)
#define GKS_CORE_H

#define MAX_COLOR 1256  /* maximum number of predefined colors */

#define Color8Bit(c)                                                         \
  c <= 256 || c >= 980 ? c :                                                 \
  c >= 588 ? 80 + (c - 588) / 56 * 12 + nint((c - 588) % 56 * 11.0 / 56.0) : \
  8 + nint((c - 257) / 330.0 * (72 - 1))

void gks_seg_xform(double *x, double *y);
int gks_open_file(const char *path, const char *mode);
int gks_read_file(int fd, void *buf, int count);
int gks_write_file(int fd, const void *buf, int count);
int gks_close_file(int fd);

const char *gks_getenv(const char *env);

char *gks_malloc(int size);
char *gks_realloc(void *ptr, int size);
void gks_free(void *ptr);

void gks_perror(const char *format, ...);
void gks_fatal_error(const char *, ...);

void gks_inq_rgb(int idx, double *r, double *g, double *b);
void gks_inq_current_xformno(int *error, int *value);
void gks_inq_xform(int num, int *error, double wn[4], double vp[4]);
void gks_inq_clip(int *error, int *value, double rect[4]);
void gks_inq_pline_linetype(int *error, int *value);
void gks_inq_pline_linewidth(int *error, double *value);
void gks_inq_pline_color_index(int *error, int *value);
void gks_inq_pmark_type(int *error, int *value);
void gks_inq_pmark_size(int *error, double *value);
void gks_inq_pmark_color_index(int *error, int *value);
void gks_inq_text_fontprec(int *error, int *font, int *prec);
void gks_inq_text_expfac(int *error, double *value);
void gks_inq_text_spacing(int *error, double *value);
void gks_inq_text_color_index(int *error, int *value);
void gks_set_chr_xform();
void gks_chr_height(double *value);
void gks_inq_text_upvec(int *error, double *x, double *y);
void gks_inq_text_path(int *error, int *value);
void gks_inq_text_align(int *error, int *halign, int *valign);
void gks_inq_fill_int_style(int *error, int *value);
void gks_inq_fill_color_index(int *error, int *value);
void gks_inq_fill_style_index(int *error, int *value);

#endif
