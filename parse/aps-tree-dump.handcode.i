void dump_lisp_Symbol(Symbol s) {
  const char *name = symbol_name(s);
  printf(" aps-boot::|");
  while (*name != '\0') {
    switch (*name) {
    case '\\':
    case '|':
      putchar('\\');
      putchar(*name);
      break;
    default:
      putchar(*name);
      break;
    }
    ++name;
  }
  putchar('|');
}

void dump_lisp_String(String st) {
  /* a disgusting kludge */
  char *s = (char *)st;
  char end = '\0';
  /* fprintf(stderr,"Converting %s\n",s); */
  printf(" \"");
  if (*s== '"') { /* a string */
    ++s;
    end = '"';
  }
  for (; *s != end; ++s) {
    if (s[0] == '\\') {
      ++s;
      if (s[0] == 'n') {
	printf("\n");
      } else {
	printf("\\%c",*s);
      }
    } else if (*s == '"') {
      printf("\\\"");
    } else {
      printf("%c",*s);
    }
  }
  printf("\"");
}

void dump_lisp_Boolean(Boolean b) {
  if (b) printf(" t"); else printf(" nil");
}
