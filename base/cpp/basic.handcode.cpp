using namespace std;

T_String C_TYPE::v_string(Node *n)
{
  return C_NULL_TYPE::v_string(n);
}

bool v_true = true;
bool v_false = false;
C_BOOLEAN* t_Boolean = get_Boolean();
C_BOOLEAN* get_Boolean() {
  static C_BOOLEAN* t_Boolean = new C_BOOLEAN();
  return t_Boolean;
}

T_String C_INTEGER::v_string(int x)
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


void C_PHYLUM::v_assert(Node *n) { assert(n || n->type == get_phylum()); }
string C_PHYLUM::v_string(Node *n) {
  return C_NULL_PHYLUM::v_string(n);
}

C_PHYLUM::C_PHYLUM() : v_nil(0) {}



// The remaining STRING functions:

string C_STRING::v_subseq_from_end(string l, int s, int f) {
  int n = (int)l.size();
  return l.substr(n-f-1,n-s-1);
}
string C_STRING::v_butsubseq(string l, int start, int finish) {
  return l.substr(0,start) + l.substr(finish,l.size());
}
string C_STRING::v_butsubseq_from_end(string l, int start, int finish) {
  int n = l.size();
  return l.substr(0,n-finish-1) + l.substr(n-start-1,n);
}

C_STRING* t_String = get_String();
C_STRING* get_String() {
  static C_STRING *t_String = new C_STRING();
  return t_String;
}
