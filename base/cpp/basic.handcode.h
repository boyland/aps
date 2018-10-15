#ifndef BASIC_HANDCODE_H
#define BASIC_HANDCODE_H

#include <iostream>
#include <stack>
#include "stdarg.h"
#include <assert.h>

// inline versions of TYPE functions
inline void C_TYPE::v_assert(Node *x) { assert (x != 0); }
inline bool C_TYPE::v_node_equivalent(Node *x, Node *y) { return x == y; }

// inline versions of BOOLEAN functions
inline void C_BOOLEAN::v_assert(bool x) { assert(x == 0 || x == 1); }
inline bool C_BOOLEAN::v_equal(bool x, bool y) { return x == y; }
inline T_String C_BOOLEAN::v_string(bool x) { return x ? "true" : "false"; }

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

// Inline versions of IEEE functions
inline void C_IEEE::v_assert(double x) {}
inline bool C_IEEE::v_equal(double x, double y) { return x == y; }
inline bool C_IEEE::v_less(double x, double y) { return x < y; }
inline bool C_IEEE::v_less_equal(double x, double y) { return x <= y; }
inline double C_IEEE::v_plus(double x, double y) { return x + y; }
inline double C_IEEE::v_minus(double x, double y) { return x - y; }
inline double C_IEEE::v_times(double x, double y) { return x * y; }
inline double C_IEEE::v_divide(double x, double y) { return x / y; }
inline double C_IEEE::v_unary_minus(double x) { return -x; }
inline double C_IEEE::v_unary_divide(double x) { return 1/x; }

// Integer$string is too big to inline

// need to implement the lognot  -> ash functions too
inline bool v_odd(int x) { return x & 1; }

inline void C_CHARACTER::v_assert(char x) {}
inline bool C_CHARACTER::v_equal(char x, char y) { return x == y; }
inline bool C_CHARACTER::v_less(char x, char y) { return x < y; }
inline bool C_CHARACTER::v_less_equal(char x, char y) { return x <= y; }
inline T_String C_CHARACTER::v_string(char x) { return std::string(1,x); }

inline int v_char_code(char x) { return x; }
inline char v_int_char(int x) { return x; }

inline bool C_PHYLUM::v_identical(Node *x, Node *y) { return x == y; }
inline int C_PHYLUM::v_object_id(Node *x) { return x->index; }
inline bool C_PHYLUM::v_object_id_less(Node *x, Node*y) { return x->index < y->index; }

template<class C_E>
inline void C_BAG<C_E>::v_assert(C_BAG::T_Result) { }

// Useful things for collection functions:
template <class C_C, class C_E> 
struct COLL {
  typedef typename C_C::T_Result Coll;
  typedef typename C_C::V_append V_append;
  typedef typename C_C::V_single V_single;
  typedef typename C_C::V_none V_none;

  typedef typename C_E::T_Result T_E;

  C_C* collection_type;
  C_E* element_type;
  COLL(C_C* ct, C_E* et) : collection_type(ct), element_type(et) {}
  
  T_String to_string(Coll l) {
    static const int max_print = 5;
    std::stack<Coll> stack;
    stack.push(l);
    int printed = 0;
    T_String res = "{";
    while (!stack.empty() && printed < max_print) {
      Coll l = stack.top();
      stack.pop();
      if (V_append *a1 = dynamic_cast<V_append*>(l)) {
	stack.push(a1->v_l2);
	stack.push(a1->v_l1);
      } else if (V_single *s1 = dynamic_cast<V_single*>(l)) {
	if (printed != 0) res += ',';
	res += element_type->v_string(s1->v_x);
	++printed;
      }
    }
    if (!stack.empty()) {
      res += "...";
    }
    res += '}';
    return res;
  }
    
  bool equal(Coll l1, Coll l2) {
    //std::cout << "equal(" << collection_type->v_string(l1) << ","
    //	      << collection_type->v_string(l2) << ")" << std::endl;
    if (V_append *a1 = dynamic_cast<V_append*>(l1)) {
      int n1 = length(a1->v_l1);
      return equal(a1->v_l1,subseq(l2,0,n1)) &&
	equal(a1->v_l2,butsubseq(l2,0,n1));
    } else if (V_single *s1 = dynamic_cast<V_single*>(l1)) {
      int n2 = length(l2);
      return n2 == 1 && element_type->v_equal(s1->v_x,nth(0,l2));
    } else if (dynamic_cast<V_none*>(l1)) {
      return length(l2) == 0;
    }
    throw stub_error("COLLECTION[...]$equal isn't working");
  }
  static T_E nth(int i, Coll l) {
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
	throw std::out_of_range("COLLECTION[...]$nth");
      }
    } else {
      throw std::out_of_range("COLLECTION[...]$nth");
    }
  }
  static T_E nth_from_end(int i, Coll l) {
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
	throw std::out_of_range("COLLECTION[...]$nth_from_end");
      }
    } else {
      throw std::out_of_range("COLLECTION[...]$nth_from_end");
    }
  }
  int position(T_E x, Coll l) {
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
  int position_from_end(T_E x, Coll l) {
    int i = position(x,l);
    if (i == -1) return -1;
    return length(l)-1-i;
  }
  bool member(T_E x, Coll l) {
    return position(x,l) >= 0;
  }
  Coll subseq(Coll l, int start, int finish) {
    // std::cout << "[" << start << "," << finish << ")" << endl;
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
    int len = length(l);
    return subseq(l,l-finish,l-start);
  }
  Coll butsubseq(Coll l, int start, int finish) {
    // std::cout << "~[" << start << "," << finish << ")" << endl;
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
    } else if (dynamic_cast<V_single*>(l)) {
      if (start == 0) return collection_type->v_none();
    }
    return l;
  }
  Coll butsubseq_from_end(Coll l, int start, int finish) {
    int n = length(l);
    return butsubseq(l,n-finish,n-start);
  }

  static int length(Coll l) {
    // std::cout << "length(" << l->to_string() << ")"<< std::endl;
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      return length(n->v_l1) + length(n->v_l2);
    } else if (dynamic_cast<V_single*>(l)) {
      return 1;
    } else if (dynamic_cast<V_none*>(l)) {
      return 0;
    } else {
      throw assertion_error("COLLECTION$length isn't working");
    }
  }

  static int empty(Coll l) {
    if (V_append * n = dynamic_cast<V_append*>(l)) {
      return empty(n->v_l1) && empty(n->v_l2);
    } else if (dynamic_cast<V_single*>(l)) {
      return false;
    } else if (dynamic_cast<V_none*>(l)) {
      return true;
    } else {
      throw assertion_error("COLLECTION$empty isn't working");
    }
  }
};

// We have to define nth and friends on BAG because
// of the way pattern matching works:
template <class C_E>
struct C__basic_16<C_E,C_BAG<C_E> > {
  typedef C_BAG<C_E> C_T;
  typedef typename C_E::T_Result T_E;
  typedef typename C_T::T_Result T_T;
  C_E *t_E;
  C_T *t_T;
  T_E v_first(T_T v_x){
    return COLL<C_T,C_E>::nth(0,v_x);
  }
  T_E v_last(T_T v_x){
    return COLL<C_T,C_E>::nth_from_end(0,v_x);
  }
  C__basic_16(C_E *_t_E,C_T *_t_T) : t_E(_t_E),
    t_T(_t_T)  {}
};
template <class C_E>
struct C__basic_17<C_E,C_BAG<C_E> > {
  typedef C_BAG<C_E> C_T;
  typedef typename C_E::T_Result T_E;
  typedef typename C_T::T_Result T_T;
  C_E *t_E;
  C_T *t_T;
  T_E v_butfirst(T_T v_x){
    return COLL<C_T,C_E>::butsubseq(v_x,0,1);
  }
  T_E v_butlast(T_T v_x){
    return COLL<C_T,C_E>::butsubseq_from_end(v_x,0,1);
  }
  C__basic_17(C_E *_t_E,C_T *_t_T) : t_E(_t_E),
    t_T(_t_T)  {}
};
 
// Sequence functions:
template <class C_E>
inline typename C_E::T_Result 
C_SEQUENCE<C_E>::v_nth(int i, T_Result l) {
  return COLL<C_SEQUENCE<C_E>,C_E>::nth(i,l);
}

template <class C_E>
inline typename C_E::T_Result 
C_SEQUENCE<C_E>::v_nth_from_end(int i, T_Result l) {
  return COLL<C_SEQUENCE<C_E>,C_E>::nth_from_end(i,l);
}

template <class C_E>
inline int C_SEQUENCE<C_E>::v_position(T_ElemType x, T_Result l) {
  return COLL<C_SEQUENCE<C_E>,C_E>(this,t_ElemType).position(x,l);
}

template <class C_E>
inline int C_SEQUENCE<C_E>::v_position_from_end(T_ElemType x, T_Result l) {
  return COLL<C_SEQUENCE<C_E>,C_E>(this,t_ElemType).position_from_end(x,l);
}

template <class C_E>
inline bool C_SEQUENCE<C_E>::v_member(T_ElemType x, T_Result l) {
  return COLL<C_SEQUENCE<C_E>,C_E>(this,t_ElemType).member(x,l);
}

// List functions 

template <class C_E>
inline bool C_LIST<C_E>::v_equal(T_Result l1, T_Result l2) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).equal(l1,l2);
}

template <class C_E>
inline typename C_E::T_Result
C_LIST<C_E>::v_nth(int i, T_Result l) {
  return COLL<C_LIST<C_E>,C_E>::nth(i,l);
}

template <class C_E>
inline typename C_E::T_Result
C_LIST<C_E>::v_nth_from_end(int i, T_Result l) {
  return COLL<C_LIST<C_E>,C_E>::nth_from_end(i,l);
}

template <class C_E>
inline int C_LIST<C_E>::v_position(T_ElemType x, T_Result l) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).position(x,l);
}

template <class C_E>
inline int C_LIST<C_E>::v_position_from_end(T_ElemType x, T_Result l) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).position_from_end(x,l);
}

template <class C_E>
inline bool C_LIST<C_E>::v_member(T_ElemType x, T_Result l) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).member(x,l);
}

template <class C_E>
inline typename C_LIST<C_E>::T_Result 
C_LIST<C_E>::v_subseq(T_Result l, int s, int f) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).subseq(l,s,f);
}

template <class C_E>
inline typename C_LIST<C_E>::T_Result 
C_LIST<C_E>::v_subseq_from_end(T_Result l, int s, int f) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).subseq_from_end(l,s,f);
}

template <class C_E>
inline typename C_LIST<C_E>::T_Result 
C_LIST<C_E>::v_butsubseq(T_Result l, int s, int f) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).butsubseq(l,s,f);
}

template <class C_E>
inline typename C_LIST<C_E>::T_Result 
C_LIST<C_E>::v_butsubseq_from_end(T_Result l, int s, int f) {
  return COLL<C_LIST<C_E>,C_E>(this,t_ElemType).butsubseq_from_end(l,s,f);
}

// SET

template<class C_E>
inline bool C_SET<C_E>::v_equal(T_Result s1, T_Result s2) 
{ return v_less_equal(s1,s2) && v_less_equal(s2,s1); }

template<class C_E>
inline bool C_SET<C_E>::v_less(T_Result s1, T_Result s2) 
{ return v_less_equal(s1,s2) && !v_less_equal(s2,s1); }

template<class E>
inline bool C_SET<E>::v_less_equal(T_Result s1, T_Result s2)
{
  if(s1->cons == this->c_append) {
    typename C_SET<E>::V_append * n = (typename C_SET<E>::V_append *) s1;
    return v_less_equal(n->v_l1,s2) && v_less_equal(n->v_l2, s2);
  } else if(s1->cons == this->c_single){
    return v_member( ((typename C_SET<E>::V_single *)s1)->v_x, s2);
  } else if(s1->cons == this->c_none){
    return true;
  } else {
    std::cout << "Really strange error!!" << std::endl;
    return false;
  }
}

template<class E>
inline typename C_SET<E>::T_Result C_SET<E>::v_none()
{ return C_BAG<E>::v_none(); }

template<class E>
inline typename C_SET<E>::T_Result C_SET<E>::v_single(T_ElemType x)
{ return C_BAG<E>::v_single(x); }

template<class E>
inline bool C_SET<E>::v_member(T_ElemType x, T_Result s) 
{ return C_BAG<E>::v_member(x,s); }

template<class E>
inline typename C_SET<E>::T_Result C_SET<E>::v_union(T_Result s1, T_Result s2) 
{
  // C++ can't handle this code without the followign repeated typedefs:
  typedef typename C_SET<E>::V_append V_append;
  typedef typename C_SET<E>::V_single V_single;
  typedef typename C_SET<E>::V_none V_none;

  if(s1->cons == this->c_append) {
    V_append * n = (V_append *) s1;
    return v_union(n->v_l1,v_union(n->v_l2, s2));
  } else if(s1->cons == this->c_single){
    V_single * n = (V_single *) s1;
    if(v_member(n->v_x, s2)){
      return s2;
    }else
      return new V_append(this->c_append,s1,s2);
  } else if(s1->cons == this->c_none){
    return s2;
  } else {
    std::cout << "Really strange error!!" << std::endl;
    return new V_none(this->c_none);
  }
}

template<class E>
inline typename C_SET<E>::T_Result C_SET<E>::v_intersect(T_Result s1, T_Result s2) 
{
  // C++ can't handle this code without the followign repeated typedefs:
  typedef typename C_SET<E>::V_append V_append;
  typedef typename C_SET<E>::V_single V_single;
  typedef typename C_SET<E>::V_none V_none;

  if(s1->cons == this->c_append) {
    V_append * n = (V_append *) s1;
    return new V_append(this->c_append,v_intersect(n->v_l1,s2),v_intersect(n->v_l2, s2));
  } else if(s1->cons == this->c_single){
    V_single * n = (V_single *) s1;
    if(v_member(n->v_x, s2)){
      return s1;
    }else
      return new V_none(this->c_none);
  } else if(s1->cons == this->c_none){
    return s1;
  } else {
    std::cout << "Really strange error!!" << std::endl;
    return new V_none(this->c_none);
  }
}

template<class E>
inline typename C_SET<E>::T_Result C_SET<E>::v_difference(T_Result s1, T_Result s2) 
{
  // C++ can't handle this code without the followign repeated typedefs:
  typedef typename C_SET<E>::V_append V_append;
  typedef typename C_SET<E>::V_single V_single;
  typedef typename C_SET<E>::V_none V_none;

  if(s1->cons == this->c_append) {
    V_append * n = (V_append *) s1;
    return new V_append(this->c_append,v_difference(n->v_l1,s2),v_difference(n->v_l2, s2));
  } else if(s1->cons == this->c_single){
    V_single * n = (V_single *) s1;
    if(v_member(n->v_x, s2)){
      return new V_none(this->c_none);
    }else
      return s1;
  } else if(s1->cons == this->c_none){
    return s1;
  } else {
    std::cout << "Really strange error!!" << std::endl;
    return new V_none(this->c_none);
  }
}

// We have to define nth and friends on SET because
// of the way pattern matching works:
template <class C_E>
struct C__basic_16<C_E,C_SET<C_E> > {
  typedef C_SET<C_E> C_T;
  typedef typename C_E::T_Result T_E;
  typedef typename C_T::T_Result T_T;
  C_E *t_E;
  C_T *t_T;
  T_E v_first(T_T v_x){
    return COLL<C_T,C_E>::nth(0,v_x);
  }
  T_E v_last(T_T v_x){
    return COLL<C_T,C_E>::nth_from_end(0,v_x);
  }
  C__basic_16(C_E *_t_E,C_T *_t_T) : t_E(_t_E),
    t_T(_t_T)  {}
};
template <class C_E>
struct C__basic_17<C_E,C_SET<C_E> > {
  typedef C_SET<C_E> C_T;
  typedef typename C_E::T_Result T_E;
  typedef typename C_T::T_Result T_T;
  C_E *t_E;
  C_T *t_T;
  T_E v_butfirst(T_T v_x){
    return COLL<C_T,C_E>::butsubseq(v_x,0,1);
  }
  T_E v_butlast(T_T v_x){
    return COLL<C_T,C_E>::butsubseq_from_end(v_x,0,1);
  }
  C__basic_17(C_E *_t_E,C_T *_t_T) : t_E(_t_E),
    t_T(_t_T)  {}
};
 
template <class E, class T>
class C__basic_22 {
 public:
  C__basic_22(E *, T *) {}

  inline int v_length(typename T::T_Result l) {
    return COLL<T,E>::length(l);
  }

  // useful, but not in the current basic.aps
  inline bool v_empty(typename T::T_Result l) {
    return COLL<T,E>::empty(l);
  }
};

template<>
class C__basic_22<C_Character,C_String> {
 public:
  C__basic_22(C_Character *,C_String *) {}

  inline int v_length(T_String l) {
    return l.size();
  }

  inline bool v_empty(T_String l) {
    return l.empty();
  }
};

template <class C_Node>
class C__basic_24 {
 public:
  C__basic_24(C_Node *) {}

  inline T_Integer v_lineno(typename C_Node::T_Result n) {
    return n->lineno;
  }
};

#endif
