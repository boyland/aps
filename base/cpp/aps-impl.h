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

  Debug(const string&); // increase indendation and print entry string
  void returns(const string&); // return value

  ostream& out(); // print a debugging comment
  static ostream& out(ostream&); // change the debugging stream
 private:
  static int depth;
  static ostream* output;
};

class Phylum;
class Type;
class Constructor;

struct Node {
  Type* type;
  Constructor* cons;
  int index;
  Node* parent;
  void set_parent(Node* n) { parent = n; }
  Node(Constructor*);
  virtual ~Node() {}
  virtual string to_string();
};

ostream& operator<<(ostream&,Node*);
string operator+(const string&,Node*);

class Module {
 protected:
  bool complete;
  string name;
 public:
  Module *t_Result;
  Module();
  Module(string name);
  virtual ~Module() {}
  virtual void finish();
  virtual string v_string(Node *n);
  virtual string v_string(bool) { return "stub_error"; }
  virtual string v_string(int) { return "stub_error"; }
  virtual string v_string(string) { return "stub_error"; }
};

class Type : virtual public Module {
  std::string name;
  std::vector<Constructor*> constructors;
 public:
  Type();
  Type(std::string);
  std::string get_name() const;
  virtual int install(Constructor*);
  virtual int install(Node*);
  void finish();
  int size();
};

class Phylum : virtual public Type {
  std::vector<Node*> nodes;
  bool complete;
 public:
  Phylum();
  Phylum(std::string);
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

class too_late_error : public runtime_error {
 public:
  too_late_error() : runtime_error("too late") {}
  too_late_error(const string& what) : runtime_error(what) {}
};

class UndefinedAttributeException : public runtime_error {
 public:
  UndefinedAttributeException() : runtime_error("Undefined attribute") {}
  UndefinedAttributeException(const string& w) : runtime_error("Undefined Attribute: "+w) {}
};

template<class P,class V> 
class Attribute {
  Phylum* phylum;
  Module* values;
  bool evaluation_started;
  bool complete;
  string name;
 public:
  Attribute(Phylum* p, Module*vt, string n)
    : phylum(p), values(vt), evaluation_started(false),complete(false) , name(n)
    {}
  virtual ~Attribute() {}
  typedef P node_type;
  typedef V value_type;

  void assign(node_type n,value_type v) {
    Debug d(phylum->v_string(n) + "." + name + ":=" + values->v_string(v));
    check_phylum(n);
    if (evaluation_started) throw too_late_error("cannot assign to attribute once evaluation has started");
    value_array[n->index] = v;
    status_array[n->index] = ASSIGNED;
  }
    
  void finish() {
    if (!complete) {
      int n = phylum->size();
      value_array.resize(n);
      status_array.resize(n);
      for (int i=0; i < n; ++i) {
	(void)evaluate((P)phylum->node(i));
      }
    }
    complete = true;
  }
  value_type evaluate(node_type n) {
    Debug d(phylum->v_string(n) + string(".") + name);
    check_phylum(n);
    evaluation_started = true;
    switch (status_array[n->index]) {
    case CYCLE: 
      {
	throw runtime_error("cycle in attribute computation for "+name+"."+n);
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
	value_type v = value_array[n->index];
	d.returns(values->v_string(v));
	return v;
      }
    }
  }
 protected:
  void check_phylum(node_type n) {
    if (n->cons->get_type() != phylum)
      throw invalid_argument("node not of correct phylum");
    if (n->index >= (int)value_array.size()) {
      value_array.resize(n->index+1);
      status_array.resize(n->index+1);
    }
  }
  virtual value_type compute(node_type n) = 0;
#ifdef UNDEF
  {
    throw UndefinedAttributeException(string("") + n + "." + name);
  }
#endif
 private:
  vector<value_type> value_array;
  vector<EvalStatus> status_array;
};

template <class T>
struct TypeTraits {};

template <class T>
struct TypeTraits<T*> {
  typedef T ModuleType;
};

template <>
struct TypeTraits<void> {
  typedef Module ModuleType;
};

class C_NULL_TYPE : virtual public Type { typedef Node* T_Result; };
class C_NULL_PHYLUM : virtual public Phylum { typedef Node* T_Result; };

class C_BOOLEAN;
template<>
struct TypeTraits<bool> {
  typedef C_BOOLEAN ModuleType;
};
typedef bool T_Boolean;
extern C_BOOLEAN *t_Boolean;

class C_INTEGER;
template<>
struct TypeTraits<int> {
  typedef C_INTEGER ModuleType;
};
// typedef int T_Integer;
// extern C_INTEGER *t_Integer;

class C_IEEE;
template<>
struct TypeTraits<double> {
  typedef C_IEEE ModuleType;
};

class C_CHARACTER;
template<>
struct TypeTraits<char> {
  typedef C_CHARACTER ModuleType;
};

class C_STRING;
template<>
struct TypeTraits<string> {
  typedef C_STRING ModuleType;
};
typedef string T_String;
extern C_STRING *t_String;

class stub_error : public runtime_error {
 public:
  stub_error(){}
  stub_error(const string& s) : runtime_error(s) {}
};

#endif
