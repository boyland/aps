#include "aps-impl.h"
#include "basic.h"

string C_TYPE::v_string(Node *n)
{
  return Module::v_string(n);
}

bool v_true = true;
bool v_false = false;
C_BOOLEAN* t_Boolean = new C_BOOLEAN;

string C_INTEGER::v_string(int x)
{
  if (x == 0) return "0";
  if (x < 0) return "-" + v_string(-x);
  string rev;
  while (x != 0) {
    rev += ('0' + (x % 10));
    x /= 10;
  }
  string s;
  for (string::reverse_iterator i=rev.rbegin(); i != rev.rend(); ++i)
    s += *i;
  return s;
}

C_INTEGER::C_INTEGER() : t_Result(this), v_zero(0), v_one(1) {}


void C_PHYLUM::v_assert(Node *n) { assert(n || n->type == this); }
string C_PHYLUM::v_string(Node *n) {
  return Module::v_string(n);
}

C_PHYLUM::C_PHYLUM() : v_nil(0) {}

// Sequence functions:
template <>
Node* C_SEQUENCE<Node*>::v_nth(int i, Node* l) {
  return COLL<C_SEQUENCE<Node*>,Node*>::nth(i,l);
}

template <>
Node* C_SEQUENCE<Node*>::v_nth_from_end(int i, Node* l) {
  return COLL<C_SEQUENCE<Node*>,Node*>::nth_from_end(i,l);
}

template<>
int C_SEQUENCE<Node*>::v_position(Node *x, Node* l) {
  return COLL<C_SEQUENCE<Node*>,Node*>(this,t_ElemType).position(x,l);
}

template<>
int C_SEQUENCE<Node*>::v_position_from_end(Node *x, Node* l) {
  return COLL<C_SEQUENCE<Node*>,Node*>(this,t_ElemType).position_from_end(x,l);
}

template<>
bool C_SEQUENCE<Node*>::v_member(Node *x, Node* l) {
  return COLL<C_SEQUENCE<Node*>,Node*>(this,t_ElemType).member(x,l);
}


// The remaining STRING functions:

string C_STRING::v_subseq_from_end(string l, int s, int f) {
  int n = (int)l.size();
  return l.substr(n-f-1,n-s-1);
}
string C_STRING::v_butsubseq(string l, int start, int finish) {
  return l.substr(0,start) + l.substr(finish,-1);
}
string C_STRING::v_butsubseq_from_end(string l, int start, int finish) {
  int n = l.size();
  return l.substr(0,n-finish-1) + l.substr(n-start-1,-1);
}

string C_STRING::v__op_AC(char c,...)
{
  string s(1,c);
  va_list ap;
  va_start(ap,c);
  while (char c = va_arg(ap,char))
    s += c;
  return s;
}

C_STRING* t_String = new C_STRING();
