#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include "dfa.h"

using namespace std;

struct DFA {
    string initial;
    map <string, bool> stateAccept ;
    map<pair<string, char>, string> transition ;

    string nextState(string state, char nextChar) {
    
        if (transition.find(make_pair(state, nextChar)) == transition.end()) { // no mapping for char
            return "" ;
        } 
        return transition[make_pair(state, nextChar)] ;
        
    }

    bool accepting(string state) {
        if (stateAccept.find(state) != stateAccept.end()) {
            return stateAccept[state] ;
        }
        return false;
    }

    DFA() : initial{}, stateAccept{}, transition{} {} ;



    const string STATES      = ".STATES";
    const string TRANSITIONS = ".TRANSITIONS";
    const string INPUT       = ".INPUT";
  

    // applies simplMaxMunch
    void simplMaxMunch(string s) {
        string state = initial;
        string lexeme;
        long val ;
        int length = s.length();
        for (int i = 0 ; i < length; ++i) {
          //cout << i << " " << s[i] << " " << lexeme << " " << state << "\n" ; 
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
                    if (state == "REGISTER") { // register is in range
                      val = stoll(lexeme.substr(1));
                      if (val <= 31 && val >= 0) {
                        cout << state << " " << lexeme << "\n";
                        
                      } else {
                        throw runtime_error("ERROR: Invalid register value!");
                      }
                      
                    } else if (state == "DECINT" || state == "ZERO") { // integer in range
                      val = stoll(lexeme);
                      //cout << val ;
                      if ((val > 4294967295) || (val < -2147483648)) {
                        throw runtime_error("ERROR: Integer value out of range!");
                      } else {
                        cout << "DECINT " << lexeme << "\n";
                      }
                    } else if (state == "HEXINT") { // hex in range
                      val = stoll(lexeme,0,16);
                      if (val >  4294967295) {
                        throw runtime_error("ERROR: Hexadecimal value out of range!");
                      } else {
                        cout << state << " " << lexeme << "\n";
                      }
                    } else if (state[0] != '?') {
                      if (state == "NEWLINE") {
                        cout << state << "\n" ;
                      } else {
                        cout << state << " " << lexeme << "\n";
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
              //cout << "newState: " << state << ", newLexeme: " << lexeme << "\n";
            }
            
        }

    }

    void DFAbuild(istream &in) {
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
        if(lineVec.empty()) {
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

};

    int main() {
      DFA dfa;
      try {
        stringstream s(DFAstring);
        dfa.DFAbuild(s);
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
      //cout << input << "\n";
      try {
        dfa.simplMaxMunch(input) ;
      } catch (runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
        
      return 0;
    }

