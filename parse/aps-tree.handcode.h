#include "jbb-string.h"
#include "jbb-symbol.h"

#include "aps-tree.gen.xcons.h"

typedef STRING String;
typedef SYMBOL Symbol;
typedef int Boolean;

#define TRUE 1
#define FALSE 0

extern Symbol copy_Symbol(Symbol);
extern String copy_String(String);
extern Boolean copy_Boolean(Boolean);

extern void assert_Symbol(Symbol);
extern void assert_String(String);
extern void assert_Boolean(Boolean);

/* forward compatability */

/* backward compatability */

#define Formal Declaration
#define Formals Declarations
#define nil_Formals nil_Declarations
#define KEYnil_Formals KEYnil_Declarations
#define list_Formals list_Declarations
#define KEYlist_Formals KEYlist_Declarations
#define append_Formals append_Declarations
#define KEYappend_Formals KEYappend_Declarations

#define TypeFormal Declaration
#define TypeFormals Declarations
#define nil_TypeFormals nil_Declarations
#define KEYnil_TypeFormals KEYnil_Declarations
#define list_TypeFormals list_Declarations
#define KEYlist_TypeFormals KEYlist_Declarations
#define append_TypeFormals append_Declarations
#define KEYappend_TypeFormals KEYappend_Declarations



