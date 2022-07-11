#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jbb-alloc.h"
#include "jbb-string.h"

/*
 * We store regular constant strings as
 * character pointers (unless they have high bits set)
 * Otherwise we use the first character as a type code:
 *   � (left european quote) for constant strings
 *   � (copyright sign) for concatenated strings
 *   � (Yen sign) for integer strings
 */

struct jbb_string {
  unsigned char type;
};

struct special_string {
  char type;
  char null; /* always '\0'; prevents segfaults if misused */
  char ignore; /* was a cached length */
  char special;
};

#define AS_SPECIAL(p,x) struct special_string *p=(struct special_string *)(x)

#define CONSTANT_STRING (128+'+')
struct constant_string {
  struct special_string header;
  char *value;
};
#define AS_CONSTANT(p,x) \
  struct constant_string *p=(struct constant_string *)(x)

#define CONC_STRING (128+')')
struct conc_string {
  struct special_string header;
  STRING str1, str2;
};
#define AS_CONC(p,x) struct conc_string *p=(struct conc_string *)(x)

#define INTEGER_STRING (128+'%')
struct integer_string {
  struct special_string header;
  int integer;
};
#define AS_INTEGER(p,x) struct integer_string *p=(struct integer_string *)(x)

/*************************** GLOBALS ***************************************/

#define def_global(name,l,string) \
  static struct constant_string S_##name = \
  {{CONSTANT_STRING,'\0',l,0},string}; STRING name = (STRING)&S_##name

def_global(empty_string,0,"");
def_global(error_string,12,"STRING-ERROR");

/*************************** FUNCTIONS *************************************/

#define TALLOC(t) (t *)HALLOC(sizeof(t))

int min(int x, int top) {
  return (x > top) ? top : x;
}
#define chop(x) min(x,255)

/* extern int strlen(char *); */

static STRING force_make_string (char *s) {
  struct constant_string *p = TALLOC(struct constant_string);
  if (s == NULL) s = "<null>";
  p->header.type = CONSTANT_STRING;
  p->header.null = '\0';
  /* p->header.length = chop(strlen(s)); */
  p->value = s;
  return (STRING)p;
}

STRING make_string(char *s) {
  if (s == NULL) return (STRING)"<null>";
  if ((unsigned int)*s > 128) {
    return force_make_string(s);
  } else {
    return (STRING)s;
  }
}

STRING make_saved_string(char *s) {
  return make_string(strcpy((char *)HALLOC(strlen(s)+1),s));
}

static int digits(int n, int base) {
  int digits = 0;
  if (n < 0) {
    ++digits;
    n = -n;
  }
  while (n > 0) {
    ++digits;
    n /= base;
  }
  return digits;
}

STRING make_integer_string(int n, int base) {
  if (2 <= base && base <= 36) {
    struct integer_string *p = TALLOC(struct integer_string);
    p->header.type = CONSTANT_STRING;
    p->header.null = '\0';
    /* p->header.length = chop(digits(n,base)); */
    p->header.special = base;
    p->integer = n;
    return (STRING)p;
  } else {
    fprintf(stderr,"make_integer_string(%d,%d): base out of range: %d\n",
	    n,base,base);
    string_error();
    return error_string;
  }
}

#define AUDIT_STRING(s,name,result) \
    if (s != NULL && \
	(s->type < 128 || \
	 s->type == CONSTANT_STRING || \
	 s->type == INTEGER_STRING || \
	 s->type == CONC_STRING)) { \
    } else { \
      fprintf(stderr,"%s: bad string argument (type = 0x%02x): 0x%08lx\n", \
	      name,(s == NULL) ? '0' : s->type,(unsigned long)s); \
      string_error(); return result; }

STRING conc_string(STRING str1, STRING str2) {
  AUDIT_STRING(str1,"conc_string",error_string);
  AUDIT_STRING(str2,"conc_string",error_string);

  if (str1->type == 0) return str2;
  if (str2->type == 0) return str1;
  {
    struct conc_string *p = TALLOC(struct conc_string);
    p->header.type = CONC_STRING;
    p->header.null = '\0';
    /* p->header.length = chop(str1->length+str2->length); */
    p->str1 = str1;
    p->str2 = str2;
    return (STRING)p;
  }
}

int string_length(STRING str) {
  AUDIT_STRING(str,"string_length",0);
  /* under old method: */
  /* if (str->length < 255) return str->length; */
  if (str->type < 128) return strlen((char *)str);
  switch (str->type) {
  case CONSTANT_STRING :
    return strlen(((struct constant_string *)str)->value);
  case INTEGER_STRING:
    { AS_INTEGER(p,str);
      return digits(p->integer,p->header.special); }
  case CONC_STRING:
    return string_length(((struct conc_string *)str)->str1) +
           string_length(((struct conc_string *)str)->str2);
  default:
    fprintf(stderr,"string_length: internal error\n");
    exit(1);
  }
}

static char *realize_constant_string(char *s, struct constant_string *p) {
  int l = strlen(p->value);
/*
  if (l < 255 && l != p->header.length) {
    fprintf(stderr,"realize_string: noticed changed string: %s\n",p->value);
    string_error();
    p->value[l] = '\0';
  }
 */
  strcpy(s,p->value);
  return s+l;
}

static char *realize_integer_string(char *s, struct integer_string *p) {
/*  int l = p->header.length; */
  int base = p->header.special;
  int n = p->integer;

  if (/* l > 0 && */ n < 0) {
    /* --l; */
    *s++ = '-';
    n = -n;
  }
  while (/* l > 0 && */ n > 0) {
    int digit = n % base;
    /* --l; */
    *s++ = (digit <= 9) ? (digit + '0') : (digit + 'a');
    n /= base;
  }
  if (n != 0) {
    fprintf(stderr,"realize_integer_string: internal error\n");
    exit(1);
  }
  return s;
}

char *realize_string_aux(char *s,STRING str) {
  if (str->type < 128) {
    strcpy(s,(char *)str);
    return s + strlen(s);
  }
  switch (str->type) {
  case CONSTANT_STRING :
    return realize_constant_string(s,(struct constant_string *)str);
  case INTEGER_STRING :
    return realize_integer_string(s,(struct integer_string *)str);
  case CONC_STRING :
    { struct conc_string *p = (struct conc_string *)str;
      return realize_string_aux(realize_string_aux(s,p->str1),p->str2); }
  default:
    fprintf(stderr,"realize_string_aux: internal error\n");
    exit(1);
  }
}

void realize_string(char *s,STRING str) {
  *s = '\0';
  AUDIT_STRING(str,"realize_string",);

  *realize_string_aux(s,str) = '\0';
}

void print_string_aux(FILE *f, STRING str) {
  if (str->type < 128) {
    fputs((char *)str,f);
  } else switch (str->type) {
  case CONSTANT_STRING :
    { AS_CONSTANT(p,str);
      fputs(p->value,f);
    } break;
  case INTEGER_STRING : /* do the easy thing: */
    { char *stack_string = (char *)SALLOC(string_length(str)+1);
      realize_string(stack_string,str);
      fputs(stack_string,f);
      release(stack_string);
    } break;
  case CONC_STRING :
    { AS_CONC(p,str);
      print_string_aux(f,p->str1);
      print_string_aux(f,p->str2);
    } break;
  default:
    fprintf(stderr,"print_string: internal error\n");
  }
}

void print_string(FILE *f, STRING str) {
  AUDIT_STRING(str,"print_string",);

  print_string_aux(f,str);
}

int dump_level = 0;
void dump_string(STRING str) {
  {int i; for (i=0; i < dump_level; ++i) putc(' ',stderr); }
  ++dump_level;
  switch (str->type) {
  case CONSTANT_STRING :
    fprintf(stderr,"\"%s\"\n",((struct constant_string *)str)->value); break;
  case INTEGER_STRING :
    { AS_INTEGER(p,str);
      fprintf(stderr,"%d (base %d)\n",p->integer,p->header.special);
    } break;
  case CONC_STRING :
    { AS_CONC(p,str);
      fprintf(stderr,"conc\n");
      dump_string(p->str1); dump_string(p->str2);
    } break;
  default:
    fprintf(stderr,"dump_string: internal error\n");
  }
  --dump_level;
}

/* Stub strings and substitute currently use the above mechanisms,
 *  Later we may introduce a new type
 */

STRING make_stub_string(char *default_text) {
  STRING str = force_make_string(default_text);
  /* if we had a special SUBST_STRING form, and we still had a length cache,
     we would need to set length = 255 to show the size could change. */
  return str;
}

static STRING subst_string (STRING sub, STRING stub, STRING subject) {
  if (subject == stub) {
    return sub;
  } else switch (subject->type) {
  case CONC_STRING :
    { struct conc_string *p = (struct conc_string *)subject;
      STRING str1 = subst_string(sub,stub,p->str1);
      STRING str2 = subst_string(sub,stub,p->str2);
      if (str1 == p->str1 && str2 == p->str2) {
	return subject; /* no change */
      } else {
	return conc_string(str1,str2);
      }
    }
  default:
    return subject; /* no substitution */
  }
}

STRING substitute_string(STRING sub, STRING stub, STRING subject) {
  AUDIT_STRING(sub,"substitute_string sub",error_string);
  AUDIT_STRING(stub,"substitute_string stub",error_string);
  AUDIT_STRING(subject,"substitute_string subject",error_string);

  return subst_string(sub,stub,subject);
}

/* don't inline ! */  
void string_error() {}
