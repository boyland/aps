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
  Declarations type_formals; /* numbered for polymorphic */
  union
  {
    Declaration result_decl; /* type declaration when use is not available */
    Type result_type; /* type of type_inst*/
  } u;
  int num_type_actuals; /* count of type actuals */
  Type type_actuals[]; /* array of type actuals */
};
  
typedef struct TypeContour *TypeEnvironment;

#define NAME_SIGNATURE 1
#define NAME_TYPE 2
#define NAME_PATTERN 4
#define NAME_VALUE 8
extern int decl_namespaces(Declaration d);

extern int bind_debug;
#define PRAGMA_ACTIVATION 1

void load_type_actuals(TypeActuals type_actuals, TypeEnvironment te);
