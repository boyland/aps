#ifndef BASIC_HANDCODE_H
#define BASIC_HANDCODE_H

#include "stdarg.h"

// inline versions of TYPE functions
inline void C_TYPE::v_assert(Node *x) { assert (x != 0); }
inline bool C_TYPE::v_node_equivalent(Node *x, Node *y) { return x == y; }

// inline versions of BOOLEAN functions
inline void C_BOOLEAN::v_assert(bool x) { assert(x == 0 || x == 1); }
inline bool C_BOOLEAN::v_equal(bool x, bool y) { return x == y; }
inline string C_BOOLEAN::v_string(bool x) { return x ? "true" : "false"; }

// outside functions:
inline bool v_and(bool x, bool y) { return x & y; }
inline bool v_or(bool x, bool y) { return x | y; }
inline bool v_not(bool x) { return !x; }

// Inline versions of INTEGER functions
inline void C_INTEGER::v_assert(int x) {}
inline bool C_INTEGER::v_equal(int x, int y) { return x == y; }
inline bool C_INTEGER::v_less(int x, int y) { return x < y; }
inline bool C_INTEGER::v_less_equal(int x, int y) { return x <= y; }
inline int  C_INTEGER::v_plus(int x, int y) { return x + y; }
inline int  C_INTEGER::v_minus(int x, int y) { return x - y; }
inline int  C_INTEGER::v_times(int x, int y) { return x * y; }
inline int  C_INTEGER::v_divide(int x, int y) { return x / y; }
inline int  C_INTEGER::v_unary_minus(int x) { return -x; }
inline int  C_INTEGER::v_unary_divide(int x) { return 1/x; }

// Integer$string is too big to inline

// need to implement the lognot  -> ash functions too
inline bool v_odd(int x) { return x & 1; }

inline void C_CHARACTER::v_assert(char x) {}
inline bool C_CHARACTER::v_equal(char x, char y) { return x == y; }
inline bool C_CHARACTER::v_less(char x, char y) { return x < y; }
inline bool C_CHARACTER::v_less_equal(char x, char y) { return x <= y; }
inline string C_CHARACTER::v_string(char x) { return string(1,x); }

inline int v_char_code(char x) { return x; }
inline char v_int_char(int x) { return x; }

inline bool C_PHYLUM::v_identical(Node *x, Node *y) { return x == y; }
inline int C_PHYLUM::v_object_id(Node *x) { return x->index; }
inline bool C_PHYLUM::v_object_id_less(Node *x, Node*y) { return x->index < y->index; }

template<class E>
inline void C_BAG<E>::v_assert(C_BAG::T_Result) { }

template <>
inline Node* C_BAG<Node*>::v__op_AC(Node* f,...) {
  //!! buggy (can't put 0 in a list)
  Node *l = v_single(f);
  va_list ap;
  va_start(ap,f);
  while ((f = va_arg(ap,Node*)) != 0)
    l = v_append(l,v_single(f));
  return l;
}

//!! People won't be able to call it...
template <>
inline Node* C_BAG<string>::v__op_AC(string f,...) {
  throw stub_error("BAG[String]${}");
}

class C_STRING : virtual public Module, virtual public C_ORDERED< string >, 
		 virtual public C_PRINTABLE< string > ,
		 virtual public C_CONCATENATING< string >
{
 public:
  typedef string T_Result;
  C_STRING *t_Result;
  void v_assert(string) {}
  bool v_equal(string l1,string l2) { return l1 == l2; }
  string v_concatenate(string l1,string l2) { return l1 + l2; }
  bool v_member(char x,string l) { return l.find(x) < l.size(); }
  char v_nth(int i,string l) { return l.at(i); }
  char v_nth_from_end(int i,string l) { return l.at(l.size()-i-1); }
  int v_position(char x,string l) { return l.find(x); }
  int v_position_from_end(char x,string l) { return l.rfind(x); }
  string v_subseq(string l,int s,int f) { return l.substr(s,f); }
  string v_subseq_from_end(string l,int s,int f);
  string v_butsubseq(string l,int start,int finish);
  string v_butsubseq_from_end(string l,int start,int finish);
  string v__op_AC(...);
  bool v_less(string x,string y) { return x < y; }
  bool v_less_equal(string x,string y) { return x <= y; }
  string v_string(string x) { return x; }

  C_STRING() : t_Result(this) {}
  C_STRING(const C_STRING& from) : t_Result(from.t_Result) {}
  void finish() {}
};


// Useful things for collection functions:
template <class C, class E> 
struct COLL {
  typedef typename C::T_Result Coll;
  typedef typename C::V_append V_append;
  typedef typename C::V_single V_single;
  typedef typename C::V_none V_none;

  C* collection_type;
  C_BASIC<E>* element_type;
  COLL(C* ct, C_BASIC<E>* et) : collection_type(ct), element_type(et) {}

  bool equal(Coll l1, Coll l2) {
    if (V_append *l1 = dynamic_cast<V_append*>(l1)) {
      int n1 = length(l1->v_l1);
      return equal(l1->v_l1,subseq(l2,0,n1)) &&
	equal(l1->v_l2,butsubseq(l2,0,n1));
    } else if (V_single *l1 = dynamic_cast<V_single*>(l1)) {
      int n2 = length(l2);
      return n2 == 1 && element_type->v_equal(l1->v_x,nth(0,l2));
    } else if (dynamic_cast<V_none*>(l1)) {
      return length(l2) == 0;
    }
    throw stub_error("COLLECTION[...]$equal isn't working");
  }
  static E nth(int i, Coll l) {
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      int n1 = length(n->v_l1);
      if (n1 > i) {
	return nth(i,n->v_l1);
      } else {
	return nth(i-n1,n->v_l2);
      }
    } else if (V_single * n = dynamic_cast<V_single*>(l)) {
      if (i == 0) {
	return n->v_x;
      } else {
	throw out_of_range("COLLECTION[...]$nth");
      }
    } else {
      throw out_of_range("COLLECTION[...]$nth");
    }
  }
  static E nth_from_end(int i, Coll l) {
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      int n2 = length(n->v_l2);
      if (n2 > i) {
	return nth_from_end(i,n->v_l2);
      } else {
	return nth_from_end(i-n2,n->v_l1);
      }
    } else if (V_single * n = dynamic_cast<V_single*>(l)) {
      if (i == 0) {
	return n->v_x;
      } else {
	throw out_of_range("COLLECTION[...]$nth_from_end");
      }
    } else {
      throw out_of_range("COLLECTION[...]$nth_from_end");
    }
  }
  int position(E x, Coll l) {
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      int p1 = position(x,n->v_l1);
      if (p1 >= 0) return p1;
      int p2 = position(x,n->v_l2);
      if (p2 >= 0) return p2 + length(n->v_l1);
    } else if (V_single * n = dynamic_cast<V_single*>(l)) {
      if (element_type->v_equal(x,n->v_x)) return 0;
    }
    return -1;
  }
  int position_from_end(E x, Coll l) {
    throw stub_error("COLLECTION[...]$position_from_end");
  }
  bool member(E x, Node *l) {
    return position(x,l) >= 0;
  }
  Coll subseq(Coll l, int start, int finish) {
    // cout << "[" << start << "," << finish << ")" << endl;
    if (start == finish) return collection_type->v_none();
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      int n1 = length(n->v_l1);
      if (start >= n1) {
	return subseq(n->v_l2,start-n1,finish-n1);
      } else if (finish <= n1) {
	if (finish == n1) {
	  return butsubseq(n->v_l1,0,start);
	}
	return subseq(n->v_l1,start,finish);
      } else {
	Coll rem1 = butsubseq(n->v_l1,0,start);
	Coll rem2 = subseq(n->v_l2,0,finish-n1);
	if (rem1 == n->v_l1 && rem2 == n->v_l2) return n;
	return collection_type->v_append(rem1,rem2);
      }
    } else {
      if (start == 0) return l;
      return collection_type->v_none();
    }
  }
  Coll subseq_from_end(Coll l, int start, int finish) {
    throw stub_error("COLLECTION[...]$subseq_from_end");
  }
  Coll butsubseq(Coll l, int start, int finish) {
    // cout << "~[" << start << "," << finish << ")" << endl;
    if (start >= finish) return l;
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      int n1 = length(n->v_l1);
      if (start >= n1) {
	Coll rem = butsubseq(n->v_l2,start-n1,finish-n1);
	return collection_type->v_append(n->v_l1,rem);
      } else if (finish <= n1) {
	if (start == 0 && finish == n1) {
	  return n->v_l2;
	} else {
	  Coll rem = butsubseq(n->v_l1,start,finish);
	  return collection_type->v_append(rem,n->v_l2);
	}
      } else {
	Coll rem1 = subseq(n->v_l1,0,start);
	Coll rem2 = butsubseq(n->v_l2,0,finish-n1);
	return collection_type->v_append(rem1,rem2);
      }
    } else if (V_single * n = dynamic_cast<V_single*>(l)) {
      if (start == 0) return collection_type->v_none();
    }
    return l;
  }
  Coll butsubseq_from_end(Coll l, int start, int finish) {
    throw stub_error("COLLECTION[...]$butsubseq_from_end");
  }

  static int length(Coll l) {
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      return length(n->v_l1) + length(n->v_l2);
    } else if (dynamic_cast<V_single*>(l)) {
      return 1;
    } else if (dynamic_cast<V_none*>(l)) {
      return 0;
    } else {
      throw stub_error("COLLECTION$length isn't working");
    }
  }
};

template <class E>
inline bool C_LIST<E>::v_equal(Node *l1, Node* l2) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).equal(l1,l2);
}

template <class E>
inline E C_LIST<E>::v_nth(int i, Node* l) {
  return COLL<C_LIST<E>,E>::nth(i,l);
}

template <class E>
inline E C_LIST<E>::v_nth_from_end(int i, Node* l) {
  return COLL<C_LIST<E>,E>::nth_from_end(i,l);
}

template <class E>
inline int C_LIST<E>::v_position(E x, Node* l) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).position(x,l);
}

template <class E>
inline int C_LIST<E>::v_position_from_end(E x, Node* l) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).position_from_end(x,l);
}

template <class E>
inline bool C_LIST<E>::v_member(E x, Node* l) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).member(x,l);
}

template <class E>
inline Node* C_LIST<E>::v_subseq(Node *l, int s, int f) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).subseq(l,s,f);
}

template <class E>
inline Node* C_LIST<E>::v_subseq_from_end(Node *l, int s, int f) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).subseq_from_end(l,s,f);
}

template <class E>
inline Node* C_LIST<E>::v_butsubseq(Node *l, int s, int f) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).butsubseq(l,s,f);
}

template <class E>
inline Node* C_LIST<E>::v_butsubseq_from_end(Node *l, int s, int f) {
  return COLL<C_LIST<E>,E>(this,t_ElemType).butsubseq_from_end(l,s,f);
}

// The following gets a compiler error if 
// I try to have it generic over all list element types
template <>
inline Node* C_LIST<Node*>::v__op_AC(Node* f,...) {
  //!! buggy (can't put 0 in a list)
  Node *l = v_single(f);
  va_list ap;
  va_start(ap,f);
  while ((f = va_arg(ap,Node*)) != 0)
    l = v_append(l,v_single(f));
  return l;
}

template <>
inline Node* C_LIST<int>::v__op_AC(int f,...) {
  //!! buggy (can't put 0 in a list)
  Node *l = v_single(f);
  va_list ap;
  va_start(ap,f);
  while ((f = va_arg(ap,int)) != 0)
    l = v_append(l,v_single(f));
  return l;
}

template <>
inline Node* C_LIST<string>::v__op_AC(string,...) {
  throw stub_error("List[...]${}");
}

template<class E>
inline bool C_SET<E>::v_equal(T_Result s1, T_Result s2) 
{ return v_less_equal(s1,s2) && v_less_equal(s2,s1); }

template<class E>
inline bool C_SET<E>::v_less(T_Result s1, T_Result s2) 
{ return v_less_equal(s1,s2) && !v_less_equal(s2,s1); }

template<class E>
inline bool C_SET<E>::v_less_equal(T_Result s1, T_Result s2)
{
  if(s1->cons == this->c_append) {
    C_SET<E>::V_append * n = (C_SET<E>::V_append *) s1;
    return v_less_equal(n->v_l1,s2) && v_less_equal(n->v_l2, s2);
  } else if(s1->cons == this->c_single){
    return v_member( ((C_SET<E>::V_single *)s1)->v_x, s2);
  } else if(s1->cons == this->c_none){
    return true;
  } else {
    cout << "Really strange error!!" << endl;
    return false;
  }
}

template<class E>
inline C_SET<E>::T_Result C_SET<E>::v_none()
{ return C_BAG<E>::v_none(); }

template<class E>
inline C_SET<E>::T_Result C_SET<E>::v_single(E x)
{ return C_BAG<E>::v_single(x); }

template<class E>
inline C_SET<E>::T_Result C_SET<E>::v__op_AC(E,...)
{ throw stub_error("SET[...]${}"); }

template<>
inline C_SET<Node*>::T_Result C_SET<Node*>::v__op_AC(T_Result f, ...) {
  //!! buggy (can't put 0 in a list)
  Node *l = v_single(f);
  va_list ap;
  va_start(ap,f);
  while ((f = va_arg(ap,T_Result)) != 0)
    l = v_union(l,v_single(f));
  return l;
}

template<class E>
inline bool C_SET<E>::v_member(E x, T_Result s) 
{ return C_BAG<E>::v_member(x,s); }

template<class E>
inline C_SET<E>::T_Result C_SET<E>::v_union(T_Result s1, T_Result s2) 
{ 
  if(s1->cons == this->c_append) {
    C_SET<E>::V_append * n = (C_SET<E>::V_append *) s1;
    return v_union(n->v_l1,v_union(n->v_l1, s2));
  } else if(s1->cons == this->c_single){
    C_SET<E>::V_single * n = (C_SET<E>::V_single *) s1;
    if(v_member(n->v_x, s2)){
      if(s2->cons == this->c_append){
	C_SET<E>::V_append * m = (C_SET<E>::V_append *) s2;
	if(v_member(n->v_x,m->v_l1)){
	  return new V_append(this->c_append,v_union(s1,m->v_l1),
			      v_union(new V_none(this->c_none),m->v_l2));
	}else{
	  return new V_append(this->c_append,v_union(s1,m->v_l1),
			      v_union(new V_none(this->c_none),m->v_l1));
	}
      }else if(s2->cons == this->c_single){
	return new V_single(this->c_single,n->v_x);
      }else{
	cout << "Really strange error!!" << endl;
	return new V_none(this->c_none);
      }
    }else
      return new V_append(this->c_append,s1,s2);
  } else if(s1->cons == this->c_none){
    if(s2->cons == this->c_append){
      C_SET<E>::V_append * m = (C_SET<E>::V_append *) s2;
      return new V_append(this->c_append,v_union(s1,m->v_l1),
			  v_union(s1,m->v_l2));
    }else if(s2->cons == this->c_single){
	return new V_single(this->c_single,((V_single *)s2)->v_x);
    }else if(s2->cons == this->c_none){
	return new V_none(this->c_none);
    }else{
      cout << "Really strange error!!" << endl;
      return new V_none(this->c_none);
    }
  } else {
    cout << "Really strange error!!" << endl;
    return new V_none(this->c_none);
  }
}

template<class E>
inline C_SET<E>::T_Result C_SET<E>::v_intersect(T_Result s1, T_Result s2) 
{
  if(s1->cons == this->c_append) {
    C_SET<E>::V_append * n = (C_SET<E>::V_append *) s1;
    return v_union(v_intersect(n->v_l1,s2),v_intersect(n->v_l1, s2));
  } else if(s1->cons == this->c_single){
    C_SET<E>::V_single * n = (C_SET<E>::V_single *) s1;
    if(v_member(n->v_x, s2)){
      return new V_single(this->c_single,n->v_x);
    }else
      return new V_none(this->c_none);
  } else if(s1->cons == this->c_none){
    return new V_none(this->c_none);
  } else {
    cout << "Really strange error!!" << endl;
    return new V_none(this->c_none);
  }
}

template<class E>
inline C_SET<E>::T_Result C_SET<E>::v_difference(T_Result s1, T_Result s2) 
{
  if(s2->cons == this->c_append) {
    C_SET<E>::V_append * n = (C_SET<E>::V_append *) s2;
    return v_difference(v_difference(s1, n->v_l1), n->v_l2);
  } else if(s2->cons == this->c_single){
    C_SET<E>::V_single * n = (C_SET<E>::V_single *) s2;
    if(s1->cons == this->c_append){
      C_SET<E>::V_append * m = (C_SET<E>::V_append *) s1;
      return new V_append(this->c_append,v_difference(m->v_l1,s2),
			  v_difference(m->v_l2,s2));
    }else if (s1->cons == this->c_single){
      if(v_member(n->v_x,s2))
	return new V_none(this->c_none);
      else
	return new V_single(this->c_single,((V_single*)s2)->v_x);
    }else if(s1->cons == this->c_none){
      return new V_none(this->c_none);
    }else{
      cout << "Really strange error!!" << endl;
      return new V_none(this->c_none);
    }
  } else if(s2->cons == this->c_none){
    if(s1->cons == this->c_append){
      C_SET<E>::V_append * m = (C_SET<E>::V_append *) s1;
      return new V_append(this->c_append,v_difference(m->v_l1,s2),
			  v_difference(m->v_l2,s2));
    }else if (s1->cons == this->c_single){
      return new V_single(this->c_single,((V_single*)s2)->v_x);
    }else if(s1->cons == this->c_none){
      return new V_none(this->c_none);
    }else{
      cout << "Really strange error!!" << endl;
      return new V_none(this->c_none);
    }
  } else {
    cout << "Really strange error!!" << endl;
    return new V_none(this->c_none);
  }
}

template <class E, class T>
class C__basic_22 {
 public:
  C__basic_22(Module *,C_READ_ONLY_COLLECTION<T,E>*) {}

  inline int v_length(typename T::T_Result l) {
    return COLL<T,E>::length(l);
  }
};

template<>
class C__basic_22<T_Character,T_String> {
 public:
  C__basic_22(Module *,C_READ_ONLY_COLLECTION<T_String,T_Character>*) {}

  inline int v_length(string l) {
    return l.size();
  }
};

#endif
