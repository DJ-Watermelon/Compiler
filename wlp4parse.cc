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
    string data;
    vector<Tree*> children;
    Tree(string data) : data(data) {}
    
    // explicit destructor; could also use smart pointers
    ~Tree() {
        for( auto &c : children ) { delete c; }
    }
    void print() {
        // prints a representation of the tree, with each line indented by the given prefix
        istringstream iss(data);
        string s;
        int i = 0;
        while (iss >> s) {
          ++i;
        }
        if (i == 1) {
          cout << data << " .EMPTY\n";
        } else {
          cout << data << "\n";
        }
        for (auto &c : children) {
          c->print() ;
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

void tokensToTrees(deque<token> & program, vector<Rule> & cfgRule, SLR & slr) {
  map <pair<int, string>, int> &transition = slr.transition ;
  map <pair<int, string>, int> &reduction = slr.reduction ;
  deque<Tree*> treeStack ;
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
  for (auto &t: treeStack) {
    t->print() ;
  }
  for (auto &t: treeStack) {
    delete t;
  }
  
}

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
      try {
        tokensToTrees(program, cfgRules, slr) ;
      } catch (runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
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
            ("Expected " + STATES + ", but found end of input.");
        }
        s = squish(s);
        if (s == STATES) {
          break;
        }
        if (!s.empty()) {
          throw runtime_error
            ("Expected " + STATES + ", but found: " + s);
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