/* jbb.h definitions peculiar to John Boyland */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#define BOOL int
#endif
#ifndef OK
#define OK 0
#define NOT_OK 1
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef USING_CXX
#ifndef new
#define new(p,t) (p = ((t)malloc((unsigned)sizeof(*p))))
#ifndef __STDC__
extern char *malloc();
#endif
#endif
#endif
#ifndef newblock
#define newblock(t) ((t *)malloc((unsigned)sizeof(t)))
#define newblocks(n,t) ((t *)calloc((unsigned)(n),(unsigned)sizeof(t)))
#ifndef __STDC__
extern char *calloc();
#endif
#endif
#ifndef streq
#define streq(s,t) (strcmp(s,t) == 0)
#define strneq(s,t,n) (strncmp(s,t,n) == 0)
#define lmatch(s,prefix) (strncmp(s,prefix,strlen(prefix)) == 0)
#endif
#ifndef strsave
/* strsave copies its argument */
#define strsave(s) strcpy((char *)malloc((unsigned)(strlen(s)+1)),s)
#ifndef __STDC__
extern char *strcpy();
#endif
#endif
#ifndef strnsave
/* strnsave copies its second argument too */
#define strnsave(s,n) strncpy((char *)malloc((unsigned)((n)+1)),s,(n))
#ifndef __STDC__
extern char *strncpy();
#endif
#endif
#ifndef str2cat
#define str2len(s,t) (strlen(s)+strlen(t))
#define str3len(s,t,u) (strlen(s)+strlen(t)+strlen(u))
#define str2cat(s,t) strcat(strcpy((char*)malloc((unsigned)(str2len(s,t)+1)),s),t)
#define str3cat(s,t,u)	\
	strcat(strcat(strcpy((char*)malloc((unsigned)(str3len(s,t,u)+1)),s),t),u)
#ifndef __STDC__
extern char *strcpy(), *strcat();
#endif
#endif
#ifndef assert
#ifndef _crash
#include <stdlib.h>
#define _crash() (abort(),0)
#endif
#ifndef _assert
#define _assert(ex) \
	((ex) || \
		(fprintf(stderr,"Assertion \"ex\" failed: file %s, line %d\n",\
			__FILE__, __LINE__),\
		_crash()))
#endif
#ifndef NDEBUG
#define assert(ex) _assert(ex)
#else
#define assert(ex)
#endif 
#endif 
#define isy(c) ((c) == 'Y' || (c) == 'y')
#define isn(c) ((c) == 'N' || (c) == 'n')
#define isyn(c) (isy(c) || isn(c))

#ifdef hpux
#define random() lrand48()
#define srandom(seed) srand48(seed)
#endif
