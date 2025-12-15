#ifndef APS_BIND_H
#define APS_BIND_H

extern void bind_Program(Program);
extern Unit first_Unit(Units);
extern Declaration first_Declaration(Declarations);
extern Expression first_Actual(Actuals);
extern Expression first_Expression(Expressions);
extern Pattern first_PatternActual(PatternActuals);
extern Pattern first_Pattern(Patterns);
extern Type first_TypeActual(TypeActuals);
extern Match first_Match(Matches);

struct TypeContour {
  struct TypeContour *outer; /*  nested type environment */
  Declaration source; /* module, class, polymorphic */
  Declarations type_formals; /* numbered for polyrmorphic */
  Declaration result; /* type declaration */
  union {
    TypeActuals type_actuals; // for module/class
    Type* inferred; // array of Types (initialized to NULL in aps-bind)
  } u;
};
  
typedef struct TypeContour *TypeEnvironment;

#define NAME_SIGNATURE 1
#define NAME_TYPE 2
#define NAME_PATTERN 4
#define NAME_VALUE 8
extern int decl_namespaces(Declaration d);

extern int bind_debug;
#define PRAGMA_ACTIVATION 1

extern Declaration module_TYPE;
extern Declaration module_PHYLUM;

#endif
