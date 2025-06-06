%option noyywrap

%{
#include<bits/stdc++.h>
using namespace std;

ofstream yyoutlog; //output filestream for log file

string loglist = ""; //list of tokens along with line numbers

// define necessary c++ variables and functions here (if any)
int lines = 1;

%}

/* Regular Definitions */

delim	 [ \t\v\r]

/* write regular expressions for whitespace and newline */
ws		 [ \t\v\r]+

newline  [\n]

letter_	 [A-Za-z_]
digit	 [0-9]

/* write regular expressions for id, float and integers */
id          {letter_}({letter_}|{digit})*
CONST_INT   {digit}+
CONST_FLOAT {digit}*"."{digit}+([eE][+-]?{digit}+)?

%%

{ws}            { /* Ignore whitespace */ }

{newline}       { lines++; }

if           { loglist = "Line no " + to_string(lines) + ": Token <IF> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
else         { loglist = "Line no " + to_string(lines) + ": Token <ELSE> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
while        { loglist = "Line no " + to_string(lines) + ": Token <WHILE> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
int          { loglist = "Line no " + to_string(lines) + ": Token <INT> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
float         { loglist = "Line no " + to_string(lines) + ": Token <FLOAT> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
do            { loglist = "Line no " + to_string(lines) + ": Token <DO> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
break         { loglist = "Line no " + to_string(lines) + ": Token <BREAK> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
char          { loglist = "Line no " + to_string(lines) + ": Token <CHAR> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
double        { loglist = "Line no " + to_string(lines) + ": Token <DOUBLE> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
void          { loglist = "Line no " + to_string(lines) + ": Token <VOID> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
return        { loglist = "Line no " + to_string(lines) + ": Token <RETURN> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
switch        { loglist = "Line no " + to_string(lines) + ": Token <SWITCH> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
case          { loglist = "Line no " + to_string(lines) + ": Token <CASE> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
default       { loglist = "Line no " + to_string(lines) + ": Token <DEFAULT> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
continue      { loglist = "Line no " + to_string(lines) + ": Token <CONTINUE> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
goto          { loglist = "Line no " + to_string(lines) + ": Token <GOTO> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
printf        { loglist = "Line no " + to_string(lines) + ": Token <PRINTF> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }


{CONST_INT}     { loglist = "Line no " + to_string(lines) + ": Token <CONST_INT> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
{CONST_FLOAT}   { loglist = "Line no " + to_string(lines) + ": Token <CONST_FLOAT> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }
{id}            { loglist = "Line no " + to_string(lines) + ": Token <ID> Lexeme " + yytext + " found" + "\n" + "\n"; yyoutlog << loglist; }


"++"|"--" {
    loglist = "Line no " + to_string(lines) + ": Token <INCOP> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}

"<"|">"|"==" {
        loglist = "Line no " + to_string(lines) + ": Token <RELOP> Lexeme " + yytext + " found" + "\n" + "\n";
        yyoutlog << loglist;
}

";"             { 
    loglist = "Line no " + to_string(lines) + ": Token <SEMICOLON> Lexeme " + yytext + " found" + "\n" + "\n"; 
    yyoutlog << loglist;
     }

"=" {
    loglist = "Line no " + to_string(lines) + ": Token <ASSIGNOP> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}

"&&"|"||" {
    loglist = "Line no " + to_string(lines) + ": Token <LOGICOP> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"!" {
    loglist = "Line no " + to_string(lines) + ": Token <NOT> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"(" {
    loglist = "Line no " + to_string(lines) + ": Token <LPAREN> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
")" {
    loglist = "Line no " + to_string(lines) + ": Token <RPAREN> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"{" {
    loglist = "Line no " + to_string(lines) + ": Token <LCURL> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"}" {
    loglist = "Line no " + to_string(lines) + ": Token <RCURL> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"[" {
    loglist = "Line no " + to_string(lines) + ": Token <LTHIRD> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"]" {
    loglist = "Line no " + to_string(lines) + ": Token <RTHIRD> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
"," {
    loglist = "Line no " + to_string(lines) + ": Token <COMMA> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}
":" {
    loglist = "Line no " + to_string(lines) + ": Token <COLON> Lexeme " + yytext + " found" + "\n" + "\n";
    yyoutlog << loglist;
}

%%


int main(int argc, char *argv[])
{

	if(argc != 2) //check if file name is given
	{
		cout<<"Please input file name"<<endl;
		return 0;
	}

	yyin = fopen(argv[1], "r"); //the file to be read must be assigned to yyin

	yyoutlog.open("my_log.txt", ios::trunc); // remember to rename the log file as mentioned in the spec
	
	if(yyin == NULL) // file does not exist
	{
		// print error message and exit
	}
	
	yylex(); // start reading the input file in a loop
	
	//print total number of lines found in the input file
	string totalLine = "Total lines: "+to_string(lines);
    yyoutlog<<totalLine;

	yyoutlog.close();  // close the log file
	
	fclose(yyin); // close the input file
}
