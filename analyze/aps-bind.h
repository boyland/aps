extern void bind_Program(Program);
extern Declaration first_Declaration(Declarations);
extern Expression first_Actual(Actuals);
extern Expression first_Expression(Expressions);
extern Pattern first_PatternActual(PatternActuals);
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

extern int bind_debug;
#define PRAGMA_ACTIVATION 1
