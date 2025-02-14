#include <iostream>
#include <sstream>
#include <deque>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <bitset>
#include "dfa.h"
#include "wlp4data.h"

using namespace std;

const string STATES      = ".STATES";
const string TRANSITIONS = ".TRANSITIONS";
const string INPUT       = ".INPUT";

struct token {
    string type;
    string lexeme;

    token(string type, string lexeme) : type{type}, lexeme{lexeme} {}
};

struct DFA {
    string initial;
    map <string, bool> stateAccept ;
    map<pair<string, char>, string> transition ;
    
    DFA() : initial{}, stateAccept{}, transition{} {};
    string nextState(string state, char nextChar) ;
    bool accepting(string state) ;
    void simplMaxMunch(string &s, deque<token> & program) ;
    void DFAbuild(istream & in) ;

};

struct Tree{
    string LH ;
    vector<string> RH;
    string type;
    vector<Tree*> children;
    Tree(string data) {
        istringstream iss{data};
        string s;
        iss >> LH;
        while (iss >> s) {
            RH.push_back(s) ;
        }
    }
    
    // explicit destructor; could also use smart pointers
    ~Tree() {
        for( auto &c : children ) { delete c; }
    }
    void print() {
        // prints a representation of the tree, with each line indented by the given prefix
        if (RH.empty()) {
          cout << LH << " .EMPTY\n";
        } else {
          cout << LH ;
          for (auto &s: RH) {
            cout << " " << s;
          } 
          cout << "\n";
        }

        for (auto &c : children) {
          c->print() ;
        }
             
    }

    Tree* getChild(string type, int i) {
        for (int j = 0; j < RH.size(); ++j) {
            if (RH[j] == type) {
                --i;
            }
            if (i == 0) return children[j];
        }
    }
};

struct Rule {
  string LHS;
  vector<string> RHS ;
  Rule(string LHS, vector<string> RHS) : LHS{LHS}, RHS{RHS} {};
  string getRule() {
    string rule = LHS;
    for (auto & sym : RHS) {
      rule += " " + sym ;
    }
    return rule ;
  } 
};
// reads in CFG and returns as a rule vector
vector<Rule> CFGbuild(istream &in) {
  vector <Rule> cfg;
  string line;
  string symbol ;
  string LHS;
  
  getline(in, line); // skip .CFG
  while (getline(in, line)) {
    istringstream iss(line);
    iss >> LHS ;    
    vector<string> RHS;
    while (iss >> symbol) {
          // terminal symbol 
          if (symbol != ".EMPTY") RHS.push_back(symbol);
    }
    cfg.push_back(Rule(LHS, RHS)) ;
  }
  return cfg ;
}

struct SLR {
  map <pair<int, string>, int> transition ;
  map <pair<int, string>, int> reduction ;
  
  SLR() : transition{}, reduction{} {};
  void SLRbuild(istream &transitions, istream &reductions) ;

};

void reduceTree(Rule &rule, deque<Tree*> &treeStack) {
  // cout << "TREE REDUCTION: " << rule.getRule() << "\n";
  int iterate = (rule.RHS).size() ;
  vector<Tree*> newChildren = vector<Tree*>(iterate);
  --iterate;
  while (iterate >= 0) {
    newChildren[iterate] = treeStack.back();
    treeStack.pop_back() ;
    --iterate;
    // cout << "TreeStack: \n" ;
    // for (auto & t: treeStack) { t->print(); }
  }
  Tree* reducedTree = new Tree(rule.getRule()) ;
  reducedTree->children = newChildren ;
  treeStack.push_back(reducedTree) ;
  
}

void reduceStack(Rule &rule, deque<int> &stateStack, map <pair<int, string>, int> &transition) {
  
  // cout << "STACK REDUCTION: " << rule.getRule() << "\n";
  int iterate = (rule.RHS).size() ;
  while (iterate > 0) {
    stateStack.pop_back();
    --iterate;
  }
  stateStack.push_back(transition[make_pair(stateStack.back(), rule.LHS)]);
}

void shift(deque<token> &input, deque<Tree*> &treeStack, deque<int> &stateStack, map <pair<int, string>, int> &transition){
  token symbol = input.front() ;
  input.pop_front() ;
  Tree* newNode = new Tree(symbol.type + " " + symbol.lexeme) ;
  treeStack.push_back(newNode) ;
  // cout << "SHIFT Node:" << symbol.type + " " + symbol.lexeme << "\n" ;
  if (transition.find(make_pair(stateStack.back(), symbol.type)) == transition.end()) {
    for (auto &c: treeStack) {
      delete c ;
    }
    throw runtime_error("ERROR: Invalid input in slr") ;
  } 
  stateStack.push_back(transition[make_pair(stateStack.back(), symbol.type)]) ;
  // cout << "new state:" << stateStack.back() << "\n";
}

void tokensToTrees(deque<token> & program, vector<Rule> & cfgRule, SLR & slr, deque<Tree*> & treeStack) {
  map <pair<int, string>, int> &transition = slr.transition ;
  map <pair<int, string>, int> &reduction = slr.reduction ;
  deque<int> stateStack ;
  stateStack.push_back(0) ;
  while (program.size() > 0) {
    while (true) {
      int state = stateStack.back() ;
      string nextSymbol = (program.front()).type ;
      // cout << "state: " << state << "\n";
      // cout << "symbol: " << nextSymbol << "\n";
      
      if (reduction.find(make_pair(state, nextSymbol)) != reduction.end()) {
        Rule &rule = cfgRule[reduction[make_pair(state, nextSymbol)]] ;
        // cout << "REDUCTION: " << rule.getRule() << "\n";
        reduceTree(rule, treeStack);
        reduceStack(rule, stateStack, transition);
      } else {
        break ;
      }
    }
    shift(program, treeStack, stateStack, transition) ;
  }
  reduceTree(cfgRule[0], treeStack);
  //treeStack[1]->print();
  return ;
  
}

struct Variable {
    string name;
    string type;
    Variable() {}
    Variable(Tree * dcl) {
        name = (dcl->getChild("ID", 1))->RH[0] ;
        type = "int";
        if ((dcl->getChild("type",1))->children.size() > 1) {
            type = type + "*" ;
        }
    }
};

struct VariableTable {
    map<string,Variable> varTable;
    void Add(Variable &var) {
        if (varTable.find(var.name) == varTable.end()) {
            varTable[var.name] = var ;
        } else {
            throw runtime_error("ERROR: Duplicate variable definition!");
        }
    }
    Variable & Get(string var) {
        if (varTable.find(var) != varTable.end()) {
            return varTable[var];
        } else {
            throw runtime_error("ERROR: Undeclared Variable!");
        }
    }
};

struct Procedure {
    string name;
    vector<string> signature;
    VariableTable symTable; 
    Procedure() {}
    Procedure(Tree * proc) {
        if (proc->LH == "main") { // main
            //name
            name = "main" ;
            // param # 1
            Variable dcl = Variable(proc->getChild("dcl", 1));
            symTable.Add(dcl);
            signature.push_back(dcl.type);
            // param # 2
            dcl = Variable(proc->getChild("dcl", 2));
            if (dcl.type == "int") {
                symTable.Add(dcl);
            } else {
                throw runtime_error("ERROR: main param #2 not int!");
            }
            signature.push_back("int");
        } else { // procedure
            //name
            name = (proc->getChild("ID",1)->RH)[0];
            // params
            Tree *params = proc->getChild("params",1) ;
            if (!(params->children).empty()) {
                params = params->getChild("paramlist",1) ; 
                while (true) {
                    Variable param = Variable(params->getChild("dcl",1));
                    symTable.Add(param);
                    signature.push_back(param.type);
                    if ((params->children).size() > 1) {
                        params = params->getChild("paramlist",1);
                    } else {
                      break;
                    }
                }
            }
        }
        Tree *dcls = proc->getChild("dcls", 1);
        while (true) {
            if ((dcls->children).empty()) {
                break;
            } else {
                Variable var = Variable(dcls->getChild("dcl",1)) ;
                string value = dcls->RH[3];
                if (var.type == "int" && value == "NULL")  throw runtime_error("ERROR: wrong type for declaration") ;
                if (var.type == "int*" && value == "NUM")  throw runtime_error("ERROR: wrong type for declaration") ;
                symTable.Add(var);
                dcls = dcls->getChild("dcls",1);
            }
        }
    }
};

struct ProcedureTable {
    map<string, Procedure> procTable;
    void Add(Procedure &proc) {
        if (procTable.find(proc.name) == procTable.end()) {
            procTable[proc.name] = proc ;
        } else {
            throw runtime_error("ERROR: Duplicate procedure definition!");
        }
    }
    Procedure &Get(string proc) {
        if (procTable.find(proc) != procTable.end()) {
            return procTable[proc];
        } else {
            throw runtime_error("ERROR: Undeclared Procedure!");
        }
    }
};

void checkStatements(Tree* subTree) {
    if (!(subTree->RH).empty()) {
        // statements
        checkStatements(subTree->getChild("statements",1)) ;
        // statement
        Tree *statement = subTree->getChild("statement",1) ;
        // cout << statement->LH ;
        // for (auto &c : statement->RH) {
        //   cout << " " << c;
        // } cout << endl;
        
        int n = statement->RH.size() ;
        if (n == 4) { // lvalue BECOMES expr SEMI
            if (statement->getChild("lvalue",1)->type != statement->getChild("expr",1)->type) throw runtime_error("ERROR: lvalue BECOMES expr types are not the same in test!") ;
        } else if (n == 5) { 
            if (statement->RH[0] == "PRINTLN") { // PRINTLN
                if (statement->getChild("expr", 1)->type != "int") throw runtime_error("ERROR: PRINTLN expr type not int!") ;
            } else { // DELETE
                if (statement->getChild("expr",1)->type != "int*") throw runtime_error("ERROR: DELETE expr type not int*!") ;
            }
        } else { // WHILE or IF
            // test
            Tree* test = statement->getChild("test", 1) ;
            if ((test->getChild("expr",1))->type != (test->getChild("expr",2))->type) throw runtime_error("ERROR: expr types are not the same in test!") ;
            if (statement->children[0]->LH == "IF") { // IF
                checkStatements(statement->getChild("statements",1));
                checkStatements(statement->getChild("statements",2));
            } else { // WHILE
                checkStatements(statement->getChild("statements",1));
            }
        }
    }
}

void annoteTypes(Tree* subTree, VariableTable & vars, ProcedureTable & procs) {
    // cout << subTree->LH ;
    // for (auto &c : subTree->RH) {
    //   cout << " " << c;
    // } cout << endl;
    for (auto &c: subTree->children) {
      annoteTypes(c, vars, procs) ;
    }
    if (subTree->LH == "NUM") {
        subTree->type = "int";
    } else if (subTree->LH == "NULL") {
        subTree->type = "int*";
    } else if (subTree->LH == "factor") {
        string id;
        switch ((subTree->RH).size()) {
            case 1:
                if (subTree->RH[0] == "ID") { // ID
                    subTree->type = (vars.Get((subTree->getChild("ID",1)->RH)[0])).type ;
                    
                } else { // NUM NULL
                    subTree->type = ((subTree->children)[0])->type ;
                }
                break;
            case 2:
                if (subTree->RH[0] == "AMP") { // AMP lvalue
                    if (subTree->getChild("lvalue", 1)->type == "int") {
                        subTree->type = "int*" ;
                    } else {
                        throw runtime_error("ERROR: factor -> AMP lvalue where lvalue type is not int") ;
                    }
                } else { // STAR factor
                    if (subTree->getChild("factor",1)->type == "int*") {
                        subTree->type = "int" ;
                    } else {
                        throw runtime_error("ERROR: factor -> STAR factor where factor type is not int*") ;
                    }
                }
                break;
            case 3:
                if (subTree->RH[0] == "ID") { // ID LPAREN RPAREN
                    id = (subTree->getChild("ID", 1))->RH[0] ;
                    if (id == "main") throw runtime_error("ERROR: function wain cannot be called recursively!") ;
                    // cout << "id: " << id << endl;
                    if (vars.varTable.find(id) == vars.varTable.end()) { // check
                        if (procs.Get(id).signature.size() > 0) throw runtime_error("ERROR: wrong number of variables in empty procedure " + id) ;
                        subTree->type = "int" ;
                    } else {
                        throw runtime_error("ERROR: variable " + id + " called as procedure");
                    }
                } else { // LPAREN expr RPAREN
                  subTree->type = (subTree->getChild("expr",1))->type;
                }
                break;
            case 4: // ID LPAREN arglist RPAREN      
                id = (subTree->getChild("ID", 1))->RH[0] ;
                if (id == "main") throw runtime_error("ERROR: function wain cannot be called recursively!") ;
                if (vars.varTable.find(id) == vars.varTable.end()) { // check
                    Tree *args = subTree->getChild("arglist",1) ; 
                    vector<string> sign = procs.Get(id).signature ;
                    int n = sign.size() ;
                    if (n == 0) throw runtime_error("ERROR: too many parameters");
                    for (int i = 0; i < n ; ++i) {
                        if (sign[i] != (args->getChild("expr",1))->type) throw runtime_error("ERROR: arglist does not match procedure " + id + " signature") ; 
                        if (i != n - 1) {
                            if ((args->children).size() > 1) {
                                args = args->getChild("arglist",1) ;     
                            } else {
                                throw runtime_error("ERROR: not enough arguments");
                            }
                        } else {
                            if ((args->children).size() > 1) throw runtime_error("ERROR: too many arguments");
                        }
                    }   
                    subTree->type = "int" ;
                } else {
                    throw runtime_error("ERROR: variable " + id + " called as procedure");
                }
                break;
            case 5:
                if (subTree->getChild("expr",1)->type == "int") {
                        subTree->type = "int*" ;
                } else {
                    throw runtime_error("ERROR: factor -> NEW INT LBRACK expr RBRACK where expr type is not int") ;
                }
                break;
        }
    } else if (subTree->LH == "lvalue") {
        switch ((subTree->RH).size()) {
            case 1: // ID
                subTree->type = (vars.Get((subTree->getChild("ID",1)->RH)[0])).type ;
                break;
            case 2: // STAR factor
                if (subTree->getChild("factor", 1)->type == "int*") {
                    subTree->type = "int" ;
                } else {
                    throw runtime_error("ERROR: factor -> AMP lvalue where lvalue type is not int") ;
                }
                break ;
            case 3: // ( lvalue )
                subTree->type = (subTree->getChild("lvalue", 1))->type ;
                break ;
        }
    } else if (subTree->LH == "term") {
        if ((subTree->RH).size() == 1) {// term -> factor
            subTree->type = (subTree->getChild("factor", 1))->type ;
        } else { // term -> term [] factor
            if (subTree->getChild("term", 1)->type != "int" || subTree->getChild("factor", 1)->type != "int") throw runtime_error("ERROR: term -> term factor not both type int") ;
            subTree->type = "int" ;
        }
    } else if (subTree->LH == "expr") {
        if ((subTree->RH).size() == 1) {
            subTree->type = (subTree->getChild("term", 1))->type ;
        } else {
            string exprType = subTree->getChild("expr",1)->type ;
            string termType = subTree->getChild("term",1)->type ;
            if (subTree->children[1]->LH == "PLUS") {
                if (exprType == "int*" && termType == "int*") throw runtime_error("ERROR: expr -> expr PLUS term both type int*") ;
                subTree->type = (exprType == "int" && termType == "int") ? "int" : "int*" ;
            } else {
                if (exprType == "int" && termType == "int*") throw runtime_error("ERROR: expr -> expr PLUS term both type int") ;
                subTree->type = (exprType == "int*" && termType == "int") ? "int*" : "int" ;
            }
        } 
    }  // cout << "--" << subTree->LH << "-- type:" << subTree->type << endl;
}

void collectProcedures(Tree *procTree) {
    ProcedureTable procs = ProcedureTable() ;
    Tree * proc ;
    while (true) {
        if (procTree->children.size() > 1) {
            proc = procTree->getChild("procedure",1) ;
            Procedure newProc = Procedure(proc);
            // for (auto it = newProc.symTable.varTable.begin(); it != newProc.symTable.varTable.end(); it++) {
            //   cout << "String: " << it->first << endl;
            //   cout << "Variable: " << (it->second).name << endl;
            //   cout << "Type: " << (it->second).type << endl;
            // }
            procs.Add(newProc) ;
            annoteTypes(proc, newProc.symTable, procs) ;
            checkStatements(proc->getChild("statements",1));
            if ((proc->getChild("expr", 1))->type != "int") throw runtime_error("ERROR: expr type is not int!") ;
            procTree = procTree->getChild("procedures",1) ;
        } else { // main
            proc = procTree->getChild("main",1) ;
            // proc->print();
            Procedure newProc = Procedure(proc); //
            procs.Add(newProc) ;
            Procedure procedure = procs.Get("main") ;
            annoteTypes(proc, procedure.symTable, procs) ;
            //cout << "HMMMM\n";
            checkStatements(proc->getChild("statements",1));
            //cout << "HMMMMmmm\n";
            if ((proc->getChild("expr", 1))->type != "int") throw runtime_error("ERROR: expr type is not int!") ;
            break;
        }
    } 
};

int main() {
      DFA dfa;
      try {
        stringstream s (DFAstring);
        dfa.DFAbuild(s);
      } catch(runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
      
      vector<Rule> cfgRules;
      try {
        stringstream CFG (WLP4_CFG);
        cfgRules = CFGbuild(CFG) ;
      } catch(runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
      SLR slr;
      try {
        stringstream transitions (WLP4_TRANSITIONS);
        stringstream reductions (WLP4_REDUCTIONS);
        slr.SLRbuild(transitions,reductions);
      } catch(runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
      string input;
      char c = getchar() ;
      while (c != EOF) {
        input += c;
        c = getchar();
      }
      input += "\n" ;
      deque<token> program ;
      program.push_back(token("BOF","BOF")) ;
      try {
        dfa.simplMaxMunch(input, program) ;
      } catch (runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
      program.push_back(token("EOF","EOF"));
      
      // been tokenized
      deque<Tree*> treeStack; 
      try {
        tokensToTrees(program, cfgRules, slr, treeStack) ;
        // cout << treeStack.size();
        // treeStack[0]->print();
        collectProcedures(treeStack[0]->getChild("procedures",1));
        for (auto &t: treeStack) {
          delete t;
        }
      } catch (runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        for (auto &t: treeStack) {
          delete t;
        }
        return 1;
      }

      return 0;
    }


void SLR::SLRbuild(istream &transitions, istream &reductions) {
  string line;
  string symbol ;
  int state1;
  int state2;

  getline(transitions, line); // skip .TRANSITION
  while (getline(transitions, line)) {
    istringstream iss(line);
    iss >> state1 ;
    iss >> symbol ;
    iss >> state2 ;    
    transition[make_pair(state1, symbol)] = state2 ;
  }
  getline(reductions, line); // skip .REDUCTION
  while (getline(reductions, line)) {
    istringstream iss(line);
    iss >> state1 ;
    iss >> state2 ;
    iss >> symbol ;    
    reduction[make_pair(state1, symbol)] = state2 ;
  }
}


string DFA::nextState(string state, char nextChar) {
    
    if (transition.find(make_pair(state, nextChar)) == transition.end()) { // no mapping for char
        return "" ;
    } 
    return transition[make_pair(state, nextChar)] ;
        
}

bool DFA::accepting(string state) {
    if (stateAccept.find(state) != stateAccept.end()) {
        return stateAccept[state] ;
    }
    return false;
}

    // applies simplMaxMunch
    // applies simplMaxMunch
    void DFA::simplMaxMunch(string &s, deque<token> & program) {
        string state = initial;
        string lexeme;
        long val ;
        string subcom[31] = {"d","de","del","dele","delet",
                            "e","el","els",
                            "i", "n","ne",
                            "p","pr","pri","prin","print","printl",
                            "r","re","ret","retu","retur", 
                            "w","wa","wai", 
                            "wh","whi","whil",
                            "N","NU","NUL"};
        int length = s.length();
        for (int i = 0 ; i < length; ++i) {
          // cout << i << " " << s[i] << " " << lexeme << " " << state << "\n" ; 
            if ((nextState(state, s[i]) == "") || (i == length - 1)) {
                //cout << "stop"  << "\n";
                
                // last token?
                if ((i == length - 1) && (nextState(state,s[i])!= "")) {
                  state = nextState(state, s[i]);
                  lexeme += s[i] ;
                }

                if (accepting(state)) {
                    //cout << "accepting"  << "\n";
                    // check that it satisfies restrictions
                    if (state == "NUM" || state == "ZERO" || state == "NUM0") { // integer in range
                      val = stoll(lexeme);
                      //cout << val ;
                      if (val > 2147483647) {
                        throw runtime_error("ERROR: Integer value out of range!");
                      } else {
                        program.push_back(token("NUM", lexeme));
                        // cout << "NUM" << " " << lexeme << "\n" ;
                      }
                    } else if (state == "lead") {
                      throw runtime_error("ERROR: leading zeroes") ;
                    } else if (state[0] != '?') {
                        bool incomplete = true;
                        for (auto i: subcom) {
                          if (i == lexeme) {
                            program.push_back(token("ID", lexeme));
                            // cout << "ID" << " " << lexeme << "\n" ;
                            incomplete = !incomplete;
                            break;
                          }
                        } if (incomplete) {
                          program.push_back(token(state, lexeme));
                          // cout << state << " " << lexeme << "\n";
                        }
                    } 
                    lexeme = "";
                    state = initial;
                    if (i != length - 1) --i;
                } else {
                  throw runtime_error("ERROR: Invalid string!!!");
                }
            } else {
              state = nextState(state, s[i]) ;
              lexeme += s[i] ;
              // cout << "newState: " << state << ", newLexeme: " << lexeme << "\n";
            }
            
        }

    }

//// Function that takes a DFA file (passed as a stream) and prints information about it.
void DFAprint(std::istream &in);

//// These helper functions are defined at the bottom of the file:
// Check if a string is a single character.
bool isChar(std::string s);
// Check if a string represents a character range.
bool isRange(std::string s);
// Remove leading and trailing whitespace from a string, and
// squish intermediate whitespace sequences down to one space each.
std::string squish(std::string s);
// Convert hex digit character to corresponding number.
int hexToNum(char c);
// Convert number to corresponding hex digit character.
char numToHex(int d);
// Replace all escape sequences in a string with corresponding characters.
std::string escape(std::string s);
// Convert non-printing characters or spaces in a string into escape sequences.
std::string unescape(std::string s);

    void DFA::DFAbuild(istream &in) {
      string s;
      // Skip blank lines at the start of the file
      while(true) {
        if (!(getline(in, s))) {
          throw runtime_error
            ("ERROR: Expected " + STATES + ", but found end of input.");
        }
        s = squish(s);
        if (s == STATES) {
          break;
        }
        if (!s.empty()) {
          throw runtime_error
            ("ERROR: Expected " + STATES + ", but found: " + s);
        }
      }
      // States
      bool start  = true;
      while(true) {
        if (!(in >> s)) {
          throw runtime_error
            ("Unexpected end of input while reading state set: " 
            + TRANSITIONS + "not found.");
        }
        if (s == TRANSITIONS) {
          break;
        } 
        // Process an individual state
        bool accepting = false;
        if (s.back() == '!' && s.length() > 1) {
          accepting = true;
          s.pop_back();
        }
        stateAccept[s] = accepting ? true : false ; 
        if (start) {
          initial = s ;
          start = false;
        }
      }
      // Print transitions
      getline(in, s); // Skip .TRANSITIONS header
      while(true) {
        if (!(getline(in, s))) {
          // We reached the end of the file
          break;
        }
        s = squish(s);
        if (s == INPUT) {
          break;
        } 
        // Split the line into parts
        string lineStr = s;
        stringstream line(lineStr);
        vector<string> lineVec;
        while(line >> s) {
          lineVec.push_back(s);
        }
        if (lineVec.empty()) {
          // Skip blank lines
          continue;
        }
        if (lineVec.size() < 3) {
          throw runtime_error
            ("Incomplete transition line: " + lineStr);
        }
        // Extract state information from the line
        string fromState = lineVec.front();
        string toState = lineVec.back();
        // Extract character and range information from the line
        vector<char> charVec;
        for(int i = 1; i < lineVec.size()-1; ++i) {
          string charOrRange = escape(lineVec[i]);
          if (isChar(charOrRange)) {
            char c = charOrRange[0];
            if (c < 0 || c > 127) {
              throw runtime_error
                ("Invalid (non-ASCII) character in transition line: " + lineStr + "\n"
                + "Character " + unescape(string(1,c)) + " is outside ASCII range");
            }
            charVec.push_back(c);
          } else if (isRange(charOrRange)) {
            for(char c = charOrRange[0]; charOrRange[0] <= c && c <= charOrRange[2]; ++c) {
              charVec.push_back(c);
            }
          } else {
            throw runtime_error
              ("Expected character or range, but found "
              + charOrRange + " in transition line: " + lineStr);
          }
        }
        // Print a representation of the transition line
        for (char c : charVec) {
          
          transition[make_pair(fromState, c)] = toState ;
        
        }
      }
      // We ignore .INPUT sections, so we're done
    
    }

    //// Helper functions

    bool isChar(string s) {
      return s.length() == 1;
    }

    bool isRange(string s) {
      return s.length() == 3 && s[1] == '-';
    }

    string squish(string s) {
      stringstream ss(s);
      string token;
      string result;
      string space = "";
      while(ss >> token) {
        result += space;
        result += token;
        space = " ";
      }
      return result;
    }

    int hexToNum(char c) {
      if ('0' <= c && c <= '9') {
        return c - '0';
      } else if ('a' <= c && c <= 'f') {
        return 10 + (c - 'a');
      } else if ('A' <= c && c <= 'F') {
        return 10 + (c - 'A');
      }
      // This should never happen....
      throw runtime_error("Invalid hex digit!");
    }

    char numToHex(int d) {
      return (d < 10 ? d + '0' : d - 10 + 'A');
    }

    string escape(string s) {
      string p;
      for(int i=0; i<s.length(); ++i) {
        if (s[i] == '\\' && i+1 < s.length()) {
          char c = s[i+1]; 
          i = i+1;
          if (c == 's') {
            p += ' ';            
          } else
          if (c == 'n') {
            p += '\n';            
          } else
          if (c == 'r') {
            p += '\r';            
          } else
          if (c == 't') {
            p += '\t';            
          } else
          if (c == 'x') {
            if(i+2 < s.length() && isxdigit(s[i+1]) && isxdigit(s[i+2])) {
              if (hexToNum(s[i+1]) > 8) {
                throw runtime_error(
                    "Invalid escape sequence \\x"
                    + string(1, s[i+1])
                    + string(1, s[i+2])
                    +": not in ASCII range (0x00 to 0x7F)");
              }
              char code = hexToNum(s[i+1])*16 + hexToNum(s[i+2]);
              p += code;
              i = i+2;
            } else {
              p += c;
            }
          } else
          if (isgraph(c)) {
            p += c;            
          } else {
            p += s[i];
          }
        } else {
          p += s[i];
        }
      }  
      return p;
    }

    string unescape(string s) {
      string p;
      for(int i=0; i<s.length(); ++i) {
        char c = s[i];
        if (c == ' ') {
          p += "\\s";
        } else
        if (c == '\n') {
          p += "\\n";
        } else
        if (c == '\r') {
          p += "\\r";
        } else
        if (c == '\t') {
          p += "\\t";
        } else
        if (!isgraph(c)) {
          string hex = "\\x";
          p += hex + numToHex((unsigned char)c/16) + numToHex((unsigned char)c%16);
        } else {
          p += c;
        }
      }
      return p;
    } 