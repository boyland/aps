#ifndef APS_IMPL_H
#define APS_IMPL_H

#include <string>
#include <vector>
#include <stdexcept>

class Debug {
 public:
  Debug(); // increase indentation
  Debug(const Debug&); // increase indentation
  ~Debug(); // decrease indentation
  // there are no private instance data members so we don't need to overload =

  Debug(const std::string&); // increase indentation and print entry string
  void returns(const std::string&); // return value

  std::ostream& out(); // print a debugging comment
  static std::ostream& out(std::ostream&); // change the debugging stream
  static int print_depth(int); // change print depth
private:
  static int depth;
  static std::ostream* output;
};

class Type;
class Phylum;
class Constructor;

typedef std::string T_String;

class Module {
 protected:
  bool complete;
 public:
  Module();
  virtual ~Module() {}
  virtual void finish();
};

class C_NULL_TYPE : public Module {
  Type* type;
 public:
  C_NULL_TYPE();

  struct Node {
    Type* type;
    Constructor* cons;
    Node(Constructor*);
    virtual ~Node() {}
    virtual T_String to_string();
  };

  typedef Node *T_Result;

  Type* get_type();

  virtual T_String v_string(Node *n);
};

std::ostream& operator<<(std::ostream&,C_NULL_TYPE::Node*);
std::string operator+(const std::string&,C_NULL_TYPE::Node*);
std::string operator+(const std::string&,int);
std::string operator+(const std::string&,bool);

extern int aps_impl_lineno;

class C_NULL_PHYLUM : public Module {
  Phylum* phylum;
 public:
  C_NULL_PHYLUM();

  struct Node : public C_NULL_TYPE::Node {
    int index;
    Node* parent;
    int lineno;
    void set_parent(Node* n) { parent = n; }
    Node(Constructor*);
    virtual T_String to_string();
  };

  typedef Node *T_Result;
  
  Phylum* get_phylum();
  Type* get_type() { return (Type*)get_phylum(); }

  virtual T_String v_string(Node *n);
};

class Type {
  std::vector<Constructor*> constructors;
 public:
  Type();
  int install(Constructor*);
};

class Phylum : public Type {
  typedef C_NULL_PHYLUM::Node Node;
  std::vector<Node*> nodes;
  bool complete;
 public:
  Phylum();
  int install(Node*);
  void finish();
  int size();
  Node* node(int i) const;
};

class Constructor {
  Type* type;
  std::string name;
  int index;
 public:
  Constructor(Type*,std::string name,int tag);
  std::string get_name() const;
  Type* get_type() const;
  int get_index() const;
};

enum EvalStatus { UNINITIALIZED, UNEVALUATED, CYCLE, EVALUATED, ASSIGNED };

class too_late_error : public std::runtime_error {
 public:
  too_late_error() : std::runtime_error("too late") {}
  too_late_error(const std::string& what) : std::runtime_error(what) {}
};

class null_constructor : public std::runtime_error {
 public:
  null_constructor() : std::runtime_error("null constructor") {}
};

class UndefinedAttributeException : public std::runtime_error {
 public:
  UndefinedAttributeException() : std::runtime_error("Undefined attribute") {}
  UndefinedAttributeException(const std::string& w) : std::runtime_error("Undefined Attribute: "+w) {}
};

class CyclicAttributeException : public std::runtime_error {
 public:
  CyclicAttributeException() : std::runtime_error("Cyclic attribute") {}
  CyclicAttributeException(const std::string& w) : std::runtime_error("Cyclic Attribute: "+w) {}
};

class stub_error : public std::runtime_error {
 public:
  stub_error() : std::runtime_error("stub error") {}
  stub_error(const std::string& s) : std::runtime_error(s) {}
};

class assertion_error : public std::logic_error {
 public:
  assertion_error(const std::string& s) : std::logic_error(s) {}
};

template<class _C_P, class _C_V> 
class Attribute {
 public:
  typedef _C_P C_P;
  typedef _C_V C_V;
  typedef typename C_P::T_Result node_type;
  typedef typename C_V::T_Result value_type;
 protected:
  C_P* nodes;
  C_V* values;
  Phylum* phylum;
 private:
  bool evaluation_started;
  bool complete;
  std::string name;
  std::vector<value_type> value_array;
  std::vector<EvalStatus> status_array;
 public:
  Attribute(C_P*nt, C_V*vt, std::string n) :
    nodes(nt), values(vt), phylum(nt->get_phylum()),
    evaluation_started(false), complete(false) , name(n)
    {}
  virtual ~Attribute() {}

  void assign(node_type n,value_type v) {
    Debug d(nodes->v_string(n) + "." + name + ":=" + values->v_string(v));
    if (evaluation_started) throw too_late_error("cannot assign to attribute once evaluation has started");
    set(n,v);
  }
    
  void finish() {
    if (!complete) {
      int n = phylum->size();
      value_array.resize(n);
      status_array.resize(n);
      for (int i=0; i < n; ++i) {
	(void)evaluate((node_type)(phylum->node(i)));
      }
    }
    complete = true;
  }

  virtual value_type evaluate(node_type n) {
    Debug d(nodes->v_string(n) + std::string(".") + name);
    check_phylum(n);
    evaluation_started = true;
    switch (status_array[n->index]) {
    case CYCLE: 
      {
	value_type v=cycle_evaluate(n);
	d.returns(values->v_string(v));
	return v;
      }
    case UNINITIALIZED:
    case UNEVALUATED:
      status_array[n->index] = CYCLE;
      {
        value_type v = compute(n);
        value_array[n->index] = v;
      }
      status_array[n->index] = EVALUATED;
      /* fall through */
    default:
      {
	value_type v = get(n);
	d.returns(values->v_string(v));
	return v;
      }
    }
  }

  virtual value_type cycle_evaluate(node_type n) {
    throw CyclicAttributeException(name+"."+n);
  }

  void check_phylum(node_type n) {
    if (n->cons->get_type() != phylum)
      throw std::invalid_argument("node not of correct phylum");
    if (n->index >= (int)value_array.size()) {
      value_array.resize(n->index+1);
      status_array.resize(n->index+1);
    }
  }
  virtual void set(node_type n, value_type v) {
    // Debug d(nodes->v_string(n) + "." + name + ":=" + values->v_string(v));
    check_phylum(n);
    value_array[n->index] = v;
    status_array[n->index] = ASSIGNED;
  }
  virtual value_type get(node_type n) {
    check_phylum(n); // paranoia
    if (status_array[n->index] == UNINITIALIZED) {
      value_array[n->index] = get_default(n);
      status_array[n->index] = UNEVALUATED;
    }
    return value_array[n->index];
  }

 protected:
  virtual value_type get_default(node_type n) {
    throw UndefinedAttributeException(std::string("") + n + "." + name);
  }

 public:
  virtual value_type compute(node_type n)
  {
    throw UndefinedAttributeException(std::string("") + n + "." + name);
  }
};

class Circular {
  typedef std::vector<Circular*> Pending;
  static Pending pending;

 protected:
  virtual ~Circular() {}
  virtual bool eval_changed() = 0;

 public:

  static bool any_pending();
  static void add_pending(Circular *c);
  static void clear_pending();

  class CheckPending {
    static int num_checks;
  public:
    CheckPending() {
      ++num_checks;
    }
    ~CheckPending() {
      --num_checks;
    }

    bool operator!() {
      return num_checks == 1;
    }
  };
};

template <class C_V>
class TypedCircular : public Circular {
 protected:
  C_V *values;
 public:
  typedef typename C_V::T_Result value_type;

  TypedCircular(C_V *vt) : values(vt) {}

  bool eval_changed() {
    value_type old_val = get();
    value_type new_val = values->v_join(old_val,eval());
    if (values->v_equal(old_val,new_val)) {
      return false;
    } else {
      set(new_val);
      return true;
    }
  }

  virtual value_type get() = 0;
  virtual value_type eval() = 0;
  virtual void set(value_type) = 0;
};

template <class C_P, class C_V>
class CircularAttributeHelper : public TypedCircular<C_V> {
 public:
  typedef typename C_P::T_Result node_type;
  typedef typename C_V::T_Result value_type;

 private:
  Attribute<C_P,C_V> *attr;
  node_type node;
  
 public:
  CircularAttributeHelper(C_P*,C_V*vt, Attribute<C_P,C_V> *a, node_type n)
    : TypedCircular<C_V>(vt), attr(a), node(n) {  }

  value_type get() { return attr->get(node); }
  value_type eval() { return attr->compute(node); }
  void set(value_type val) { attr->set(node,val); }
};

//! This definition of circular attributes is incorrect because
//! it assumes all dependencies are monotone.
template <class C_P, class C_V>
class CircularAttribute : public Attribute<C_P,C_V> {
 public:
  typedef typename C_P::T_Result node_type;
  typedef typename C_V::T_Result value_type;

  CircularAttribute(C_P*nt, C_V*vt, std::string n)
    : Attribute<C_P,C_V>(nt,vt,n) {}

  virtual value_type cycle_evaluate(node_type n) {
    CircularAttributeHelper<C_P,C_V> *cah = 
	new CircularAttributeHelper<C_P,C_V>(this->nodes,this->values,this,n);
    Circular::add_pending(cah);
    Attribute<C_P,C_V>::set(n,this->values->v_bottom);
    return this->values->v_bottom;
  }

  virtual value_type evaluate(node_type n) {
    Circular::CheckPending cp;
    value_type val = Attribute<C_P,C_V>::evaluate(n);
    if (!cp) {
      Circular::clear_pending();
      val = get(n);
    }
    return val;
  }
};

template <class A>
class Collection : public A {
  typedef typename A::C_P C_P;
  typedef typename A::C_V C_V;
  typedef typename A::node_type node_type;
  typedef typename A::value_type value_type;
 protected:
  value_type initial;

 public:
  Collection(C_P*nt, C_V*vt, std::string n, value_type init)
    : A(nt,vt,n), initial(init) {}

  virtual void set(node_type n, value_type v) {
    Attribute<C_P,C_V>::set(n,combine(this->get(n),v));
  }

 protected:
  virtual value_type get_default(node_type n) {
    return initial;
  }

  virtual value_type combine(value_type v1, value_type v2) = 0;
};

class C_STRING : public C_NULL_TYPE
{
 public:
  typedef std::string T_Result;
  void v_assert(T_Result) {}
  bool v_equal(T_Result l1,T_Result l2) { return l1 == l2; }
  T_Result v_concatenate(T_Result l1,T_Result l2) { return l1 + l2; }
  T_Result v_append(T_Result l1, T_Result l2) { return l1 + l2; }
  T_Result v_single(char c) { return T_Result(1,c); }
  T_Result v_none() { return ""; }
  bool v_member(char x,T_Result l) { return l.find(x) < l.size(); }
  char v_nth(int i,T_Result l) { return l.at(i); }
  char v_nth_from_end(int i,T_Result l) { return l.at(l.size()-i-1); }
  int v_position(char x,T_Result l) { return l.find(x); }
  int v_position_from_end(char x,T_Result l) { return l.rfind(x); }
  T_Result v_subseq(T_Result l,int s,int f) { return l.substr(s,f); }
  T_Result v_subseq_from_end(T_Result l,int s,int f);
  T_Result v_butsubseq(T_Result l,int start,int finish);
  T_Result v_butsubseq_from_end(T_Result l,int start,int finish);
  bool v_less(T_Result x,T_Result y) { return x < y; }
  bool v_less_equal(T_Result x,T_Result y) { return x <= y; }
  T_Result v_string(T_Result x) { return x; }

  void finish() {}
};

typedef C_STRING C_String;
// already mentioned above:
// typedef std::string T_String;
extern C_STRING *t_String;
extern C_STRING *get_String();

typedef class C_BOOLEAN C_Boolean;
typedef bool T_Boolean;
extern C_Boolean *t_Boolean;
extern C_Boolean *get_Boolean();
extern T_Boolean v_not(T_Boolean v__27);

template <class T_T>
std::string s_string(T_T n);

#endif
