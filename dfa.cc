#include "dfa.h"

std::string DFAstring = R"(
.STATES
start
ID!
NUM0!
NUM!
ZERO!
lead!
i!
in!
INT!
w!
wa!
wai!
WAIN!
IF!
e!
el!
els!
ELSE!
wh!
whi!
whil!
WHILE!
p!
pr!
pri!
prin!
print!
printl!
PRINTLN!
r!
re!
ret!
retu!
retur!
RETURN!
n!
ne!
NEW!
d!
de!
del!
dele!
delet!
DELETE!
N!
NU!
NUL!
NULL!
LPAREN!
RPAREN!
LBRACE!
RBRACE!
LBRACK!
RBRACK!
BECOMES!
PLUS!
MINUS!
STAR!
SLASH!
PCT!
AMP!
COMMA!
SEMI!
LT!
GT!
LE!
GE!
EQ!
neg
NE!
?WHITESPACE!
?COMMENT!
.TRANSITIONS
start d d
d e de
d a-d f-z A-Z 0-9 ID
de l del
de a-k m-z A-Z 0-9 ID
del e dele
del a-d f-z A-Z 0-9 ID
dele t delet
dele a-s u-z A-Z 0-9 ID
delet e DELETE
delet a-d f-z A-Z 0-9 ID
start i i
i f IF
IF a-z A-Z 0-9 ID
i n in
i a-e g-m o-z A-Z 0-9 ID
in t INT
in a-s u-z A-Z 0-9 ID
INT a-z A-Z 0-9 ID
start p p
p r pr
p a-q s-z A-Z 0-9 ID
pr i pri
pr a-h j-z A-Z 0-9 ID
pri n prin
pri a-m o-z A-Z 0-9 ID
prin t print
prin a-s u-z A-Z 0-9 ID
print l printl
print a-k m-z A-Z 0-9 ID
printl n PRINTLN
printl a-m o-z A-Z 0-9 ID
PRINTLN a-z A-Z 0-9 ID
start N N
N U NU 
N a-z A-T V-Z 0-9 ID
NU L NUL
NU a-z A-K M-Z 0-9 ID
NUL L NULL
NUL a-z A-K M-Z 0-9 ID
NULL a-z A-Z 0-9 ID
start e e
e l el
e a-k m-z A-Z 0-9 ID
el s els
el a-r t-z A-Z 0-9 ID
els e ELSE
els a-d f-z A-Z 0-9 ID
ELSE a-z A-Z 0-9 ID
start n n
n e ne
n a-d f-z A-Z 0-9 ID
ne w NEW
ne a-v y-z A-Z 0-9 ID
NEW a-z A-Z 0-9 ID
start r r
r e re
r a-d f-z A-Z 0-9 ID
re t ret
re a-s u-z A-Z 0-9 ID
ret u retu
ret a-t v-z A-Z 0-9 ID
retu r retur
retu a-q s-z A-Z 0-9 ID
retur n RETURN
retur a-m o-z A-Z 0-9 ID
RETURN a-z A-Z 0-9 ID
start w w
w a wa
w h wh
w b-g i-z A-Z 0-9 ID
wa i wai
wa a-h j-z A-Z 0-9 ID
wai n WAIN
wai a-m o-z A-Z 0-9 ID
WAIN a-z A-Z 0-9 ID
wh  i whi
wh a-h j-z A-Z 0-9 ID
whi l whil
whi a-k m-z A-Z 0-9 ID
whil e WHILE
whil a-d f-z A-Z 0-9 ID
WHILE a-z A-Z 0-9 ID
start a-c f-h j-m o q s-v x-z A-M O-Z ID
ID    a-z A-Z 0-9 ID
start  0   ZERO
ZERO 0-9 lead
lead 0-9 lead
start  1-9 NUM0
NUM0 0-9 NUM
NUM 0-9 NUM
start ( LPAREN
start ) RPAREN
start { LBRACE
start } RBRACE
start [ LBRACK
start ] RBRACK
start = BECOMES
start + PLUS
start - MINUS
start * STAR
start / SLASH
start % PCT
start & AMP
start , COMMA 
start ; SEMI
start < LT
start > GT
LT = LE
GT = GE
BECOMES = EQ
start ! neg
neg = NE
start       \s \t \n \r ?WHITESPACE
?WHITESPACE \s \t \n \r ?WHITESPACE
SLASH / ?COMMENT
?COMMENT \x00-\x09 \x0B \x0C \x0E-\x7F ?COMMENT
)";