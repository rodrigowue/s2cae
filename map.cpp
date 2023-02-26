#include "map.h"
#include "transistor.h"
#include <unordered_set>
#include <vector>
#include <math.h>
using namespace std;

void print_logo(){
cout << "======================================" << endl;
cout << " SPICE STD-CELL TIMING ARCS EXTRACTOR" << endl;
cout << "       [UNDER DEVELOPMENT]" << endl;
cout << "======================================" << endl;
};

vector<string> remove_pin(vector<string> pin_list, string pin){
	for (auto it_rm = begin(pin_list); it_rm != end(pin_list); ){
		if (*it_rm == pin){
			//cout << "deleting " << *it_rm << " from the list" << endl;
    		pin_list.erase(it_rm);
			}
		else{
			it_rm++;
			}
		}

	return pin_list;
}

void distribute_pins(vector<string>& common_nets, vector<string>& in_pins, vector<string>& out_pins){
	vector<string> in_pins_tmp;
	for (string common_net: common_nets){
		for (string pin : in_pins){
			if(common_net == pin){
				in_pins = remove_pin(in_pins, pin);
				out_pins.push_back(pin);
				if (in_pins.size() == 1){
					break;
				}
				else{
					distribute_pins(common_nets, in_pins, out_pins);
					break;
				}
			}
			
		}
	}
	return;
};

bool check_parallel(Transistor A, Transistor B){
	if ((( A.get_source() == B.get_source() ) && ( A.get_drain() == B.get_drain())) | (( A.get_drain() == B.get_source() ) && ( A.get_source() == B.get_drain()))){
		return true;
	}
	else{
		return false;
	}
}

bool check_pg_pin(string pin, vector<string>& power_pins, vector<string>& ground_pins){
		for(string vdd_pin: power_pins){
			if(pin == vdd_pin){
				return false;
			}
		}
		for(string gnd_pin: ground_pins){
			if(pin == gnd_pin){
				return false;
			}
		}
		return true;
}

bool check_series(Transistor A, Transistor B, vector<string>& power_pins, vector<string>& ground_pins, string& common_net){
	if (((( A.get_source() == B.get_source() ) & (A.get_source() != common_net))|
		 (( A.get_drain() == B.get_drain())    & (A.get_drain() != common_net )))|
		((( A.get_source() == B.get_drain())   & (A.get_source() != common_net))|
		 (( A.get_drain() == B.get_source())   & (A.get_drain() != common_net )))){
		return true;
	}
	else{
		return false;
	}
}

vector<Transistor> remove_two_items(vector<Transistor> PDN, Transistor A, Transistor B){
	for (auto it_rm = begin(PDN); it_rm != end(PDN); ){
		if (it_rm->get_alias() == B.get_alias()){
    		PDN.erase(it_rm);
			}
		else if(it_rm->get_alias() == A.get_alias()){
			PDN.erase(it_rm);
			}
		else{
			it_rm++;
			}
		}

	return PDN;
}

string find_expression_v2(int circuit_columns, string common_net, vector<Transistor> PDN, vector<string>& power_pins, vector<string>& ground_pins){
	vector<Transistor> PDN_TEMP = PDN;
    string alias = "";
    string source = "";
    string drain = "";
    string gate = "";
    string bulk = "";
    string type = "";
	double diff_width = 0.0;
    int fingers=0;
    double gate_lenght = 0.0;
    int stack=0;
	auto a_pointer = PDN_TEMP.begin();
	string expression = "";
	if (PDN.size() > circuit_columns){
		Transistor A = *a_pointer;
				for (Transistor B : PDN){
	  				if ((check_parallel(A,B) == true) & (A.get_alias() != B.get_alias()) & ((A.get_source() == common_net)|(A.get_drain() == common_net)|(B.get_source() == common_net)|(B.get_drain() == common_net))){
						expression.append(A.get_gate());
						expression.append("+");
						expression.append(B.get_gate());
						alias = "";
						alias.append("(");
						alias.append(A.get_gate());
						alias.append("+");
						alias.append(B.get_gate());
						alias.append(")");
						gate = alias;
						source = A.get_source();
						drain  = A.get_drain();
						Transistor group_transistor(alias, source, drain, gate, bulk, type, diff_width, fingers, gate_lenght, stack);
						PDN_TEMP.shrink_to_fit();
						PDN_TEMP.push_back(group_transistor); // insert the merged item
						PDN_TEMP = remove_two_items(PDN_TEMP, A, B); //remove the two items that were merged
						if(PDN_TEMP.size()==circuit_columns){
							return (PDN_TEMP.front()).get_alias();
							break;
						}
						else{
							expression = find_expression_v2(circuit_columns, common_net, PDN_TEMP, power_pins, ground_pins);
							break;
						}
						
	  				}
	  				else if ((check_series(A,B,power_pins,ground_pins,common_net) == true) & (A.get_alias() != B.get_alias()) & ((A.get_source() == common_net)|(A.get_drain() == common_net)|(B.get_source() == common_net)|(B.get_drain() == common_net))){
					expression.append(A.get_gate());
					expression.append("*");
					expression.append(B.get_gate());
					alias = "";
					alias.append("(");
					alias.append(A.get_gate());
					alias.append("*");
					alias.append(B.get_gate());
					alias.append(")");
					gate = alias;

            		//Find the connecting point and preserve the connection
            		if ( A.get_source() == B.get_source() & (check_pg_pin(A.get_source(),power_pins,ground_pins))){
            		    source = A.get_drain();
					    drain  = B.get_drain();
            		}
            		else if(A.get_drain() == B.get_drain() & (check_pg_pin(A.get_drain(),power_pins,ground_pins))){
            		    source = A.get_source();
					    drain  = B.get_source();
            		}
            		else if(A.get_source() == B.get_drain() & (check_pg_pin(A.get_source(),power_pins,ground_pins))){
            		    source = A.get_drain();
					    drain  = B.get_source();
            		}
            		else{
            		    source = A.get_source();
					    drain  = B.get_drain();
            		}
            
					Transistor group_transistor(alias, source, drain, gate, bulk, type, diff_width, fingers, gate_lenght, stack);
					PDN_TEMP.shrink_to_fit();
					PDN_TEMP.push_back(group_transistor); // insert the merged item
					PDN_TEMP = remove_two_items(PDN_TEMP, A, B); //remove the two items that were merged
					if(PDN_TEMP.size()==circuit_columns){
						return (PDN_TEMP.front()).get_alias();
						break;
					}
					else{
						expression = find_expression_v2(circuit_columns, common_net, PDN_TEMP, power_pins, ground_pins);
						break;
					}
	  			}
				else{
					a_pointer++;
				}
			}
	}
	if (expression==""){
		for(Transistor transistor: PDN){
			if((transistor.get_drain()==common_net)|(transistor.get_source()==common_net)){
				return transistor.get_gate();
			}
		}
	}
	return expression;
}

string find_expression(vector<Transistor> PDN){
	vector<Transistor> PDN_TEMP = PDN;
	vector<string> power_pins, ground_pins;
    string alias = "";
    string source = "";
    string drain = "";
    string gate = "";
    string bulk = "";
    string type = "";
	double diff_width = 0.0;
    int fingers=0;
    double gate_lenght = 0.0;
    int stack=0;
	string expression;
	string common_net = "";
	int size = PDN_TEMP.size();
	auto a_pointer = PDN_TEMP.begin();
	if (PDN_TEMP.size() > 1){
		Transistor A = *a_pointer;
		for (auto it = begin(PDN_TEMP)+1; (it != end(PDN_TEMP)); ++it){
	  		Transistor B = *it;
	  		if (check_parallel(A,B) == true){
				expression.append(A.get_gate());
				expression.append("+");
				expression.append(B.get_gate());
				alias = "";
				alias.append("(");
				alias.append(A.get_gate());
				alias.append("+");
				alias.append(B.get_gate());
				alias.append(")");
				gate = alias;
				source = A.get_source();
				drain  = A.get_drain();
				Transistor group_transistor(alias, source, drain, gate, bulk, type, diff_width, fingers, gate_lenght, stack);
				PDN_TEMP.shrink_to_fit();
				PDN_TEMP.push_back(group_transistor); // insert the merged item
				PDN_TEMP = remove_two_items(PDN_TEMP, A, B); //remove the two items that were merged
				if (PDN_TEMP.size() == 1){
					return (PDN_TEMP.front()).get_alias();
					break;
				}
				else{
					expression = find_expression(PDN_TEMP);
					break;
				}
	  		}
	  	else if (check_series(A,B,power_pins, ground_pins,common_net) == true){
			expression.append(A.get_gate());
			expression.append("*");
			expression.append(B.get_gate());
			alias = "";
			alias.append("(");
			alias.append(A.get_gate());
			alias.append("*");
			alias.append(B.get_gate());
			alias.append(")");
			gate = alias;

            //Find the connecting point and preserve the connection
            if ( A.get_source() == B.get_source() ){
                source = A.get_drain();
			    drain  = B.get_drain();
            }
            else if(A.get_drain() == B.get_drain()){
                source = A.get_source();
			    drain  = B.get_source();
            }
            else if(A.get_source() == B.get_drain()){
                source = A.get_drain();
			    drain  = B.get_source();
            }
            else{
                source = A.get_source();
			    drain  = B.get_drain();
            }
            
			Transistor group_transistor(alias, source, drain, gate, bulk, type, diff_width, fingers, gate_lenght, stack);
			PDN_TEMP.shrink_to_fit();
			PDN_TEMP.push_back(group_transistor);
			PDN_TEMP = remove_two_items(PDN_TEMP, A, B); //remove the two items that were merged
			if (PDN_TEMP.size() == 1){
				return (PDN_TEMP.front()).get_alias();
				break;
			}
			else{
				expression = find_expression(PDN_TEMP);
				break;
			}
	  	}
	  	else{
			cout << "not correlated items" << endl;
			a_pointer++;
	  	}
    	}
	}
	return expression;
}

string flatten_expression(vector<string> common_nets, vector<string> expressions){
	string expression = expressions.front();
	if (expressions.size()>1){
		for (int i = 0 ; i < common_nets.size(); i++){
			for (auto it2 = begin(expressions)+1; (it2 != end(expressions)); ++it2){
			if(it2->find(common_nets.at(i)) != string::npos){
				cout << "it2:" << *it2 << endl;
				cout << "exp:" << expressions.at(i) << endl;
				cout << "commo:" << common_nets.at(i) << endl;
				string what_it_is =  common_nets.at(i);
				string what_it_will_be = expressions.at(i);
				replace_all(*it2, what_it_is, what_it_will_be);
				cout << "Result:" << *it2 << endl;

				for (auto it = begin(expressions)+1; (it != end(expressions)); ++it){
					if(*it == expressions.at(i)){
						expressions.erase(it);	
					};
				}
				expression = flatten_expression(common_nets, expressions);
			}
			}
		}	
	}
	return expression;
}



int solve_boolean_expression(string expression){
	if (expression.size() > 1){
		replace_all(expression, "(0+0)", "0");
		replace_all(expression, "(0+1)", "1");
		replace_all(expression, "(1+0)", "1");
		replace_all(expression, "(1+1)", "1");

		replace_all(expression, "(0*0)", "0");
		replace_all(expression, "(0*1)", "0");
		replace_all(expression, "(1*0)", "0");
		replace_all(expression, "(1*1)", "1");

		replace_all(expression, "!1", "0");
		replace_all(expression, "!0", "1");
		return solve_boolean_expression(expression);
		}
	else{
		return atoi(expression.c_str());
	}
}

void replace_all(
    std::string& s,
    std::string const& toReplace,
    std::string const& replaceWith
) {
    std::string buf;
    std::size_t pos = 0;
    std::size_t prevPos;

    // Reserves rough estimate of final size of string.
    buf.reserve(s.size());

    while (true) {
        prevPos = pos;
        pos = s.find(toReplace, pos);
        if (pos == std::string::npos)
            break;
        buf.append(s, prevPos, pos - prevPos);
        buf += replaceWith;
        pos += toReplace.size();
    }

    buf.append(s, prevPos, s.size() - prevPos);
    s.swap(buf);
}

vector<string> find_arcs(vector<string> in_pins, string expression){
	vector<string> arcs;
	string expr = expression;
  	unordered_set<char> literals;
  
  for (char c : expr) {
    if (isalpha(c)) {
      literals.insert(c);
    }
  }

  int numLiterals = literals.size();
  cout << "Number of literals: " << numLiterals << endl;


  int maxVal = 1 << numLiterals;
  bool values[numLiterals];

  for (int i = 0; i < maxVal; i++) {
    // Set the values of the literals
    int t = i;
    int j = 0;
    for (char c : literals) {
      values[j++] = t & 1;
      t >>= 1;
    }

    // Evaluate the boolean expression
    string local_expression = expr;
    
    for (size_t i = 0; i < local_expression.size(); ++i) {
        int it = 0;
        for (char c : literals) {
            if (local_expression[i] == char(c)) {
                local_expression.replace(i, 1, to_string(values[it]));
            }
            it++;
        }
    }
    
    bool result = solve_boolean_expression(local_expression);
    //bool result = !((values[0] + values[1]) * (values[0] * values[1]));
    
    // Check for transition arcs
    int k = 0;
    for (char c : literals) {
      int t = i ^ (1 << k);
      int l = 0;
      for (char d : literals) {
        values[l++] = t & 1;
        t >>= 1;
      }
      string local_expression = expr;
      for (size_t i = 0; i < local_expression.size(); ++i) {
        int it = 0;
        for (char c : literals) {
            if (local_expression[i] == char(c)) {
                local_expression.replace(i, 1, to_string(values[it]));
            }
            it++;
        }
    }
    
      //cout << local_expression << endl;
      bool newResult = solve_boolean_expression(local_expression);

      if (newResult != result) {
		int counter = 0;
		string arc = "";
        for(char c1: literals){
			if(c1 == c){
			if (values[k]) {
          		cout << "F";
				arc += "F";
        	} else {
          		cout << "R";
				arc += "R";
        	}
			}
			else{
				cout << values[counter];
				if (values[counter] == 1){
					arc += "1";
				}
				else{
					arc += "0";
				}
			}
			counter++;
			cout << " " ;
		}
        cout << " | ";
        if (newResult) {
          cout << "Fall";
		  arc += "F";
        } else {
          cout << "Rise";
		  arc += "R";
        }
		arcs.push_back(arc);
        cout << endl;
      }

      k++;
    }
  }
	return arcs;

}

vector<string> truth_table(vector<string> in_pins, string expression){
	vector<string> arcs;
	int counter = 0;
	int amount_of_inputs = pow(2,(in_pins.size()));
	while (counter < amount_of_inputs){
		string local_expression = expression;
		for (auto it = begin(in_pins); (it != end(in_pins)); ++it){
			int index = it - in_pins.begin();
			int teste = (counter >> index) & 1;
			replace_all(local_expression, *it, to_string(teste));
			cout << teste;
		}
		if (solve_boolean_expression(local_expression) == 1){
			cout << "|" << 1  << endl;
		}
		else{
			cout << "|" << 0  << endl;
		}
		counter++;
	}
	return arcs;

}