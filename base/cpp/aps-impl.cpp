#include <iostream>
#include <fstream>
#include "aps-impl.h"
#include "basic.h"

using namespace std;

int Debug::depth = 0;

static ostream* make_null_output()
{
  // return &cout;
  return new ofstream("/dev/null");
}

ostream* Debug::output = make_null_output();

Debug::Debug() {
  ++depth;
}
Debug::Debug(const Debug&) {
  ++depth;
}
Debug::~Debug() {
  --depth;
}

Debug::Debug(const string& initial) {
  ++depth;
  out() << initial << endl;
}

void Debug::returns(const string& value) {
  out() << "=> " << value << endl;
}

ostream& Debug::out() {
  for (int i=0; i < depth; ++i)
    output->put(' ');
  return *output;
}

ostream& Debug::out(ostream& new_stream) {
  ostream* old = output;
  output = &new_stream;
  return *old;
}

ostream& operator<<(ostream&os,C_NULL_TYPE::Node*n)
{
  if (n) {
    os << n->to_string();
  } else {
    os << "<null>";
  }
  return os;
}

string operator+(const string&s,C_NULL_TYPE::Node*n)
{
  if (n == 0) return s + "<null>";
  return s + n->to_string();
}

string operator+(const string&s,int i)
{
  return s + t_Integer->v_string(i);
}

Module::Module() : complete(false) {}

void Module::finish() { complete = true; }

static int print_depth = 1;
class Depth {
public:
  Depth() { --print_depth; }
  ~Depth() { ++print_depth; }
  int get() { return print_depth; }
};

template <>
std::string s_string<int>(int n) 
{
  return t_Integer->v_string(n);
}

template <>
std::string s_string<bool>(bool b) 
{
  return t_Boolean->v_string(b);
}

template <>
std::string s_string<std::string>(std::string s) 
{
  return s;
}

template <>
std::string s_string<C_NULL_TYPE::Node *>(C_NULL_TYPE::Node* n) 
{
  if (!n) return "<null>";
  Depth d;
  if (d.get() < 0) return ("#");
  return n->to_string();
}

template <>
std::string s_string<C_NULL_PHYLUM::Node *>(C_NULL_PHYLUM::Node* n)
{
  if (!n) return "<null>";
  Depth d;
  if (d.get() < 0) return ("#");
  return n->to_string();
}

C_NULL_TYPE::C_NULL_TYPE() : type(0) {}

C_NULL_TYPE::Node::Node(Constructor*c)
  : type(c->get_type()), cons(c)
{}

T_String C_NULL_TYPE::Node::to_string() {
  return cons->get_name();
}

Type* C_NULL_TYPE::get_type() {
  if (type == 0) {
    // cout << (int)this << ": Creating Type" << endl;
    type = new Type();
  }
  return type;
}

T_String C_NULL_TYPE::v_string(Node *n) {
  return s_string<Node*>(n);
}

C_NULL_PHYLUM::C_NULL_PHYLUM() : phylum(0) {}
  
int aps_impl_lineno;

C_NULL_PHYLUM::Node::Node(Constructor*c)
  : C_NULL_TYPE::Node(c), index(((Phylum*)type)->install(this)),
    lineno(aps_impl_lineno) {}

T_String C_NULL_PHYLUM::Node::to_string() {
  return cons->get_name() + "#" + t_Integer->v_string(index);
}

Phylum* C_NULL_PHYLUM::get_phylum() {
  if (phylum == 0) phylum = new Phylum();
  return phylum;
}

T_String C_NULL_PHYLUM::v_string(Node *n) {
  return s_string<Node*>(n);
}
  
Type::Type() {}

int Type::install(Constructor* c)
{
  // cout << (int)this << ": Installing " << c->get_name() << " at " << constructors.size() << endl;
  constructors.push_back(c);
  return constructors.size()-1;
}

Phylum::Phylum() : complete(false) {}

int Phylum::install(Node* n)
{
  if (complete) throw too_late_error("cannot add node");
  nodes.push_back(n);
  return nodes.size()-1;
}

void Phylum::finish()
{
  complete = true;
}

int Phylum::size()
{
  finish();
  return nodes.size();
}

C_NULL_PHYLUM::Node* Phylum::node(int i) const
{
  if (i < 0) {
    throw invalid_argument("negative index to Phylum::node(int)");
  }
  if (i >= int(nodes.size())) {
    throw out_of_range("index too large to Phylum::node(int)");
  }
  return nodes[i];
}

Constructor::Constructor(Type *t, string n, int tag)
  : type(t), name(n), index(tag)
{
  if (tag != t->install(this))
    throw logic_error("tag doesn't match type constructor index");
}

string Constructor::get_name() const { return name; }
int Constructor::get_index() const { return index; }
Type* Constructor::get_type() const { return type; }

bool Circular::any_pending() {
  return !pending.empty();
}

void Circular::add_pending(Circular *c) {
  pending.push_back(c);
}

void Circular::clear_pending() {
  unsigned n;
  do {
    n = 0;
    for (Pending::iterator i = pending.begin(); i != pending.end(); ++i)
      if (!(*i)->eval_changed()) ++n;
  } while (n < pending.size());
  for (Pending::iterator i = pending.begin(); i != pending.end(); ++i)
    delete (*i);
  pending.clear();
}

Circular::Pending Circular::pending;
int Circular::CheckPending::num_checks = 0;



