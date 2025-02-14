#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <bitset>
#include "dfa.h"

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
    void simplMaxMunch(string &s, vector<token> & program) ;
    void DFAbuild(istream & in) ;

};
void intToBytes(int paramInt);

void translate(vector<token> &program);
    
    int main() {
      DFA dfa;
      try {
        stringstream s (DFAstring);
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
      vector<token> program ;
      try {
        dfa.simplMaxMunch(input, program) ;
      } catch (runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
      
      try {
        translate(program) ;
      } catch (runtime_error &e) {
        cerr << "ERROR: " << e.what() << "\n";
        return 1;
      }
      return 0;
    }

long regToInt (string reg) {
  reg = reg.substr(1, reg.size() - 1);
  return stoll(reg) ;
}

void translate(vector<token> &program) {
    map<string, int> instr;
    instr["add"] = 0;
    instr["sub"] = 0;
    instr["slt"] = 0;
    instr["sltu"] = 0;

    instr["mult"] = 1;
    instr["multu"] = 1;
    instr["div"] = 1;
    instr["divu"] = 1;

    instr["mfhi"] = 2;
    instr["mflo"] = 2;
    instr["lis"] = 2;
    instr["jr"] = 2;
    instr["jalr"] = 2;

    instr["bne"] = 3;
    instr["beq"] = 3;

    instr["lw"] = 4;
    instr["sw"] = 4;

    map<string, int> opcode;
    opcode["beq"] = 4;
    opcode["bne"] = 5;
    opcode["lw"] = 35;
    opcode["sw"] = 43;

    map<string, int>func_code;
    func_code["add"] = 32;
    func_code["sub"] = 34;
    func_code["slt"] = 42;
    func_code["sltu"] = 43;

    func_code["mult"] = 24;
    func_code["multu"] = 25;
    func_code["div"] = 26;
    func_code["divu"] = 27;

    func_code["mfhi"] = 16;
    func_code["mflo"] = 18;
    func_code["lis"] = 20;
    func_code["jr"] = 8;
    func_code["jalr"] = 9;

     //add sub mult multu div divu mfhi mflo lis slt sltu jr jalr beq bne lw sw
    int pc = 0;
    map <string, long> labels; // (label, address)

    // checks new labels and duplicates 
    // check proper format ignoring whether label values are valid

    int length = program.size() ;
    string type;
    string lexeme; 
    for (int i = 0; i < length ; ++i) { // process one line at a time
        type = program[i].type ;
        lexeme = program[i].lexeme ;
        // space for one word (label, comment)
        
        while (type == "LABELDEF") { // get all labels
            string label = lexeme.substr(0, lexeme.size() - 1);
            if (labels.find(label) == labels.end()) {
                labels[label] = pc;
                ++i ;
                if (i == length) { // end of program
                    break;
                } else {
                    type = program[i].type ;
                    lexeme = program[i].lexeme ;
                }
            } else {
                throw runtime_error("ERROR: Duplicate label definition");                 
            }
        }

        if (type == "ID" || type == "DOTID") {
            if (instr.find(lexeme) == instr.end()) {
              if (type == "DOTID" && lexeme == ".word") {
                if (i < length - 1) { // check appropriate length
                    ++i ;
                    type = program[i].type ;
                    if (type != "DECINT" && type != "HEXINT" && type != "ID") {             
                        throw runtime_error("ERROR: Invalid DOTID value");
                    }
                } else {
                throw runtime_error("ERROR: No .word value defined");
                }
              } else {
                throw runtime_error("ERROR: Invalid instruction ID/DOTID");
              }  
            } else {
                switch (instr[lexeme]) {
                    case 0 : // ID REGISTER COMMA REGISTER COMMA REGISTER
                        if (i < length - 5) {
                            if (program[i+1].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing $d");
                            if (program[i+2].type != "COMMA") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $d");
                            if (program[i+3].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing $s");
                            if (program[i+4].type != "COMMA") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $s");
                            if (program[i+5].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing $t");
                            i += 5 ;
                        } else {
                            throw runtime_error("ERROR: incomplete case 0 instruction");
                        }
                        break;
                    case 1 : // ID REGISTER COMMA REGISTER
                        if (i < length - 3) {
                            if (program[i+1].type != "REGISTER") throw runtime_error("ERROR: invalid case 1 instruction: missing $s");
                            if (program[i+2].type != "COMMA") throw runtime_error("ERROR: invalid case 1 instruction: missing comma after $s");
                            if (program[i+3].type != "REGISTER") throw runtime_error("ERROR: invalid case 1 instruction: missing $t");
                            i += 3;
                        } else {
                            throw runtime_error("ERROR: incomplete case 1 instruction");
                        }
                        break;
                    case 2 : // ID REGISTER
                        if (i < length - 1) {
                            if (program[i+1].type != "REGISTER") throw runtime_error("ERROR: invalid case 2 instruction: missing $d/$s");
                            i += 1;
                        } else {
                            throw runtime_error("ERROR: incomplete case 2 instruction");
                        }
                        break;
                    case 3 : // ID REGISTER COMMA REGISTER COMMA [DECINT || HEXINT || ID]
                        if (i < length - 5) {
                            if (program[i+1].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing $s");
                            if (program[i+2].type != "COMMA") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $s");
                            if (program[i+3].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing $t");
                            if (program[i+4].type != "COMMA") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $t");
                            if (program[i+5].type != "DECINT" && program[i+5].type != "HEXINT" && program[i+5].type != "ID") throw runtime_error("ERROR: invalid case 0 instruction: missing i");
                            i += 5;
                        } else {
                            throw runtime_error("ERROR: incomplete case 3 instruction");
                        }
                        break;
                    case 4 : // ID REGISTER COMMA [DECINT or HEXINT] LPAREN REGISTER RPAREN
                        if (i < length - 6) {
                            if (program[i+1].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing $t");
                            if (program[i+2].type != "COMMA") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $t");
                            if (program[i+3].type != "DECINT" && program[i+3].type != "HEXINT") throw runtime_error("ERROR: invalid case 0 instruction: missing i");
                            if (program[i+4].type != "LPAREN") throw runtime_error("ERROR: invalid case 0 instruction: missing $t");
                            if (program[i+5].type != "REGISTER") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $t");
                            if (program[i+6].type != "RPAREN") throw runtime_error("ERROR: invalid case 0 instruction: missing comma after $t");
                            i += 6;
                        } else {
                            throw runtime_error("ERROR: incomplete case 4 instruction");
                        }
                        break;
                }
            }
          // determine if newline needed or not
          if (i < length - 1) {
              ++i;
              if (program[i].type != "NEWLINE") { // must be NEWLINE
                  throw runtime_error("ERROR: missing NEWLINE");
              } else {
                  pc += 4;
              }
          }
        } else if (i < length && type != "NEWLINE") { // only other option after labels that is not ID or DOTID
          //cout << "i: " << length - i << "\n";
          //cout << "type: " << type << "\n";
          //cout << "val: " << lexeme << "\n";
          throw runtime_error("ERROR: Invalid instruction line");
        } 
    }
    // check existence
    // calculate labels
    // print out appropriate machine code
    long binary;
    long immediate ;
    vector <unsigned char> byteArray ;
    pc = 0 ;
    for (int i = 0; i < length ; ++i) {
        type = program[i].type ;
        lexeme = program[i].lexeme ;
        binary = 0;
        // space for one word (label, comment)

        if (type == "ID" || type == "DOTID") {
            if (instr.find(lexeme) == instr.end()) { // already checked so must be .word
                ++i;
                type = program[i].type ;
                lexeme = program[i].lexeme ;
                if (type == "ID") {
                  // check valid label
                  if (labels.find(lexeme) != labels.end()) {
                    binary = labels[lexeme] ;
                  } else {
                    throw runtime_error("ERROR: Label \""+ lexeme + "\" not found");
                  }
                } else { // hexint, decintconvert to integer
                  // cout << lexeme ;
                  if (type == "HEXINT") {
                    binary = stoll(lexeme,0,16);
                  } else {
                    binary = stoll(lexeme);
                  }
                }
                // cout << "binary: " << bitset<32>(binary) << "\n";
            } else {
                switch (instr[lexeme]) {
                    case 0 : // ID REGISTER COMMA REGISTER COMMA REGISTER
                        binary += regToInt(program[i + 3].lexeme) ; // $s
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 5].lexeme) ; // $t
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 1].lexeme) ; // $d
                        binary <<= 11 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary +=  func_code[lexeme];
                        i += 5;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        break;
                    case 1 : // ID REGISTER COMMA REGISTER
                        binary += regToInt(program[i + 1].lexeme) ; // $s
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 3].lexeme) ; // $t
                        binary <<= 16 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += func_code[lexeme];
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        i += 3;
                        break;
                    case 2 : // ID REGISTER
                        if (lexeme == "jr" || lexeme == "jalr") {
                          binary += regToInt(program[i+1].lexeme) ;
                          binary <<= 21 ;
                          // cout << "binary: " << bitset<32>(binary) << "\n";
                        } else { // mflo, mfhi, lis
                          binary += regToInt(program[i+1].lexeme) ;
                          binary <<= 11 ;
                          // cout << "binary: " << bitset<32>(binary) << "\n";
                        }
                        binary += func_code[lexeme];
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        i += 1;
                        break;
                    case 3 : // ID REGISTER COMMA REGISTER COMMA [DECINT || HEXINT || ID]
                        binary += opcode[lexeme] ;
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 1].lexeme) ; // $s
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 3].lexeme) ; // $t
                        binary <<= 16 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        lexeme = program[i + 5].lexeme ; // i
                        type = program[i + 5].type ;
                        if (type == "ID") {
                          // check valid label
                          if (labels.find(lexeme) != labels.end()) {
                            immediate = (labels[lexeme] - pc - 4)/4 ;
                            if (immediate < -32768 || immediate > 32767) throw runtime_error("ERROR: immediate not in range") ;
                            if (immediate < 0) {
                              immediate = immediate & 0xFFFF ;
                            }
                          } else {
                            throw runtime_error("ERROR: Label "+ lexeme + " not found");
                          }
                        } else { // hexint, decintconvert to integer 
                          if (type == "HEXINT") {
                            immediate = stoll(lexeme, 0, 16) ; 
                            if (immediate > 65535) throw runtime_error("ERROR: immediate not in range") ;
                          } else { // DECINT
                            immediate = stoll(lexeme);
                            if (immediate < -32768 || immediate > 32767) throw runtime_error("ERROR: immediate not in range") ;
                            if (immediate < 0) {
                              immediate = immediate & 0xFFFF ;
                            }
                          } 
                        }
                        
                        binary  += immediate ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        i += 5;
                        break ;
                    case 4 : // ID REGISTER COMMA [DECINT or HEXINT] LPAREN REGISTER RPAREN
                        binary += opcode[lexeme] ;
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 5].lexeme) ; // $s
                        binary <<= 5 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        binary += regToInt(program[i + 1].lexeme) ; // $t
                        binary <<= 16 ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        if (program[i+3].type == "HEXINT") {
                          immediate = stoll(program[i + 3].lexeme, 0, 16) ; 
                          if (immediate > 65535) throw runtime_error("ERROR: immediate not in range") ;
                        } else { // DECINT
                          immediate = stoll(program[i + 3].lexeme);
                          if (immediate < -32768 || immediate > 32767) throw runtime_error("ERROR: immediate not in range") ;
                          if (immediate < 0) {
                            immediate = immediate & 0xFFFF ;
                          }
                        } 
                        binary  += immediate ;
                        // cout << "binary: " << bitset<32>(binary) << "\n";
                        i += 6;
                        break;
                }
            }
            
            intToBytes(binary) ;
            pc += 4;
        }
    }
}

void intToBytes(int paramInt) {
    vector<unsigned char> arrayOfByte(4);
    for (int i = 0; i < 4; i++) {
      arrayOfByte[3 - i] = (paramInt >> (i * 8));
    }
    for (int i = 0; i < 4; i++) {
      cout << arrayOfByte[i];
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
    void DFA::simplMaxMunch(string &s, vector<token> & program) {
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
                        program.push_back(token(state, lexeme));
                        
                      } else {
                        throw runtime_error("ERROR: Invalid register value!");
                      }
                      
                    } else if (state == "DECINT" || state == "ZERO") { // integer in range
                      val = stoll(lexeme);
                      //cout << val ;
                      if ((val > 4294967295) || (val < -2147483648)) {
                        throw runtime_error("ERROR: Integer value out of range!");
                      } else {
                        program.push_back(token("DECINT", lexeme));
                      }
                    } else if (state == "HEXINT") { // hex in range
                      val = stoll(lexeme,0,16);
                      if (val >  4294967295) {
                        throw runtime_error("ERROR: Hexadecimal value out of range!");
                      } else {
                        program.push_back(token(state, lexeme)) ;
                      }
                    } else if (state[0] != '?') {
                      if (state == "NEWLINE") {
                        program.push_back(token(state,"\n")) ;
                      } else {
                        program.push_back(token(state, lexeme)) ;
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