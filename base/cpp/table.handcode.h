// Handcoded tables for APS
// John Boyland
// March 2003

#include <map>

template <class C_T>
struct APS_less {
  typedef typename C_T::T_Result T_T;
  C_T *t_T;
  APS_less(C_T *_t_T) : t_T(_t_T) {}
  bool operator()(T_T x, T_T y) const { return t_T->v_less(x,y); }
};

template <class C_KeyType, class C_ValueType>
class C_TABLE : public C_TYPE {
  typedef typename C_KeyType::T_Result T_KeyType;
  typedef typename C_ValueType::T_Result T_ValueType;
  C_KeyType *t_KeyType;
  C_ValueType *t_ValueType;

public:
  struct V_full_table;

private:
  struct V_Table : public C_TYPE::Node {
    V_Table(Constructor *c) : C_TYPE::Node(c) {}
    virtual ~V_Table() {}
    virtual void add_to(V_full_table*) = 0;
  };

 public:
  typedef C_TABLE C_Result;
  typedef V_Table* T_Result;
  C_Result* const t_Result;
  enum P_Result{ p_full_table, p_empty_table, p_table_entry };

  struct V_full_table : public V_Table {
    typedef APS_less<C_KeyType> Key_Less;
    typedef std::map<T_KeyType,T_ValueType,Key_Less> Entries;
    C_KeyType *t_KeyType;
    C_ValueType *t_ValueType;
    Entries entries;
    V_full_table(Constructor* c, C_KeyType *_t_KeyType, C_ValueType *_vt) :
      V_Table(c),
      t_KeyType(_t_KeyType),
      t_ValueType(_vt),
      entries(Key_Less(_t_KeyType)) {}
    V_full_table *as_full() { return this; }
    void add_to(V_full_table *table) {
      typename Entries::const_iterator
	begin = entries.begin(),
	end = entries.end(),
	i;
      for (i=begin; i != end; ++i) {
	table->add((*i).first,(*i).second);
      }
    }
    T_ValueType get(T_KeyType key) {
      Debug debug(this->to_string()+".get(" + t_KeyType->v_string(key) + ")");
      typename Entries::iterator i = entries.find(key);
      T_ValueType result;
      if (i != entries.end()) {
	result = (*i).second;
      } else {
	result = entries[key] = t_ValueType->v_initial;
      }
      debug.returns(t_ValueType->v_string(result));
      return result;
    }
    void add(T_KeyType key, T_ValueType value) {
      Debug debug(this->to_string()+".add(" + t_KeyType->v_string(key) +
		  "," + t_ValueType->v_string(value) + ")");
      T_ValueType old = get(key);
      entries[key] = t_ValueType->v_combine(old,value);
    }
  };

  Constructor* c_full_table;
  V_full_table* v_full_table() {
    return new V_full_table(c_full_table,t_KeyType,t_ValueType);
  }

  struct V_empty_table : public V_Table {
    V_empty_table(Constructor *c) : V_Table(c){}
    std::string to_string() {
      return std::string("empty_table(")+")";
    }
    V_full_table *as_full() { return V_full_table(); }
    void add_to(V_full_table *table) { }
  };

  Constructor* c_empty_table;
  T_Result v_empty_table() {
    return new V_empty_table(c_empty_table);
  }

  struct V_table_entry : public V_Table {
    T_KeyType v_key;
    T_ValueType v_val;
    V_table_entry(Constructor *c, T_KeyType _key, T_ValueType _val)
  : V_Table(c), v_key(_key), v_val(_val){}
    std::string to_string() {
      return std::string("table_entry(")
          +s_string(v_key)+","
          +s_string(v_val)+")";
    }
    V_full_table* as_full() {
      V_full_table* result = v_full_table();
      result->entries[v_key] = v_val;
      return result;
    }
    void add_to(V_full_table* table) {
      table->add(v_key,v_val);
    }
  };

  Constructor* c_table_entry;
  T_Result v_table_entry(T_KeyType v_key,T_ValueType v_val) {
    Debug debug(std::string("table_entry(") +
		t_KeyType->v_string(v_key) + "," +
		t_ValueType->v_string(v_val) + ")");
    return new V_table_entry(c_table_entry,v_key,v_val);
  }

  T_Result v_initial;
  T_Result v_combine(T_Result v_t1,T_Result v_t2) {
    V_full_table *table;
    if ((table = dynamic_cast<V_full_table*>(v_t1)) == 0) {
      table = v_full_table();
      v_t1->add_to(table);
    }
    v_t2->add_to(table);
    return table;
  }
  T_Result v_select(T_Result v_table,T_KeyType v_key) {
    if (V_full_table* table = dynamic_cast<V_full_table*>(v_table)) {
      return v_table_entry(v_key,table->get(v_key));
    } else if (V_table_entry* table = dynamic_cast<V_table_entry*>(v_table)) {
      if (t_KeyType->v_equal(v_key,table->v_key)) {
	return table;
      }
    } 
    return v_table_entry(v_key,t_ValueType->v_initial);
  }

  void finish() { }


  C_TABLE(C_KeyType* _t_KeyType,C_ValueType* _t_ValueType) : C_TYPE(),
    t_KeyType(_t_KeyType),
    t_ValueType(_t_ValueType),
    t_Result(this),
    c_full_table(new Constructor(get_type(),"full_table",p_full_table)),
    c_empty_table(new Constructor(get_type(),"empty_table",p_empty_table)),
    c_table_entry(new Constructor(get_type(),"table_entry",p_table_entry)),
    v_initial(v_empty_table()) {}
};
