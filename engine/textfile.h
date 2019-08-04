#ifndef __TEXTFILE__
#define __TEXTFILE__

#ifdef __cplusplus
extern "C" {
#endif

#include "ithelib.h"

struct TF_S
	{
	int blocks;
	char **block;
	long size;
	int lines;
	char **line;
	int *linewords;
	char ***lineword;
	};

extern void TF_init(struct TF_S *s);
extern int TF_load(struct TF_S *s, const char *filename);
extern int TF_merge(struct TF_S *olds, const char *filename, int insertpoint);
extern void TF_splitwords(struct TF_S *s, char comment);
extern void TF_term(struct TF_S *s);

#ifdef __cplusplus
}
#endif

#endif
