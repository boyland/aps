#include "aps-impl.h"
#include "basic.h"

using namespace std;

int Debug::depth = 0;

static ostream* make_null_output()
{
  return &cout;
  // return new ofstream("/dev/null");
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

Node::Node(Constructor*c)
  : type(c->get_type()), cons(c), index(type->install(this))
{}

string Node::to_string() {
  if (dynamic_cast<Phylum*>(cons->get_type()))
    return cons->get_name() + "#" + t_Integer->v_string(index);
  else
    return cons->get_name();
}

ostream& operator<<(ostream&os,Node*n)
{
  if (n) {
    os << n->to_string();
  } else {
    os << "<null>";
  }
  return os;
}

string operator+(const string&s,Node*n)
{
  if (n == 0) return s + "<null>";
  return s + n->to_string();
}

Module::Module() : complete(false), name("<anonymous>"), t_Result(this) {}
Module::Module(string n) : complete(false), name(n), t_Result(this) {}

static print_depth = 1;
class Depth {
public:
  Depth() { --print_depth; }
  ~Depth() { ++print_depth; }
  get() { return print_depth; }
};

string Module::v_string(Node *n) {
  if (!n) return "<null>";
  Depth d;
  if (d.get() < 0) return ("#");
  return n->to_string();
}
  
void Module::finish() { complete = true; }

Type::Type() : name("anonymous") {}

Type::Type(string s) : name(s) {}

string Type::get_name() const { return name; }

int Type::install(Constructor* c)
{
  constructors.push_back(c);
  return constructors.size()-1;
}

int Type::install(Node* n)
{
  return 0; // always
}

void Type::finish() {}

int Type::size()
{
  throw domain_error("types not not keep track of nodes");
}

Phylum::Phylum() : complete(false) {}

Phylum::Phylum(string s) : Type(s), complete(false) {}


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

Node* Phylum::node(int i) const
{
  if (i < 0) throw invalid_argument("negative index to Phylum::node(int)");
  if (i >= int(nodes.size())) throw out_of_range("index too large to Phylum::node(int)");
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



