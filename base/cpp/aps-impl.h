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

  Debug(const std::string&); // increase indendation and print entry string
  void returns(const std::string&); // return value

  std::ostream& out(); // print a debugging comment
  static std::ostream& out(std::ostream&); // change the debugging stream
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

class C_NULL_PHYLUM : public Module {
  Phylum* phylum;
 public:
  C_NULL_PHYLUM();

  struct Node : public C_NULL_TYPE::Node {
    int index;
    Node* parent;
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

class UndefinedAttributeException : public std::runtime_error {
 public:
  UndefinedAttributeException() : std::runtime_error("Undefined attribute") {}
  UndefinedAttributeException(const std::string& w) : std::runtime_error("Undefined Attribute: "+w) {}
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

template<class C_P, class C_V> 
class Attribute {
 public:
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

  value_type evaluate(node_type n) {
    Debug d(nodes->v_string(n) + std::string(".") + name);
    check_phylum(n);
    evaluation_started = true;
    switch (status_array[n->index]) {
    case CYCLE: 
      {
	throw std::runtime_error("cycle in attribute computation for "+name+"."+n);
      }
      break;
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
 protected:
  void check_phylum(node_type n) {
    if (n->cons->get_type() != phylum)
      throw std::invalid_argument("node not of correct phylum");
    if (n->index >= (int)value_array.size()) {
      value_array.resize(n->index+1);
      status_array.resize(n->index+1);
    }
  }
  void set(node_type n, value_type v) {
    check_phylum(n);
    value_array[n->index] = v;
    status_array[n->index] = ASSIGNED;
  }
  value_type get(node_type n) {
    check_phylum(n); // paranoia
    if (status_array[n->index] < EVALUATED)
      throw assertion_error("get used illegally");
    return value_array[n->index];
  }

  virtual value_type compute(node_type n) = 0;
#ifdef UNDEF
  {
    throw UndefinedAttributeException(std::string("") + n + "." + name);
  }
#endif
};

class C_STRING;
typedef C_STRING C_String;
// already mentioned above:
// typedef std::string T_String;
extern C_STRING *t_String;

typedef class C_BOOLEAN C_Boolean;
typedef bool T_Boolean;
extern C_Boolean *t_Boolean;

template <class T_T>
std::string s_string(T_T n);

#endif
