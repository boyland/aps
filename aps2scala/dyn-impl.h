#include <iostream>

void implement_local_attributes(vector<Declaration>& local_attributes,
				ostream& oss);

void implement_attributes(const vector<Declaration>& attrs,
			  const vector<Declaration>& tlms,
			  ostream& oss);

void implement_var_value_decls(const vector<Declaration>& vvds,
			       const vector<Declaration>& tlms,
			       ostream& oss);                