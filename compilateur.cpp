//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin
//
//  Build with "make compilateur"

#include <string>
#include <iostream>
#include <cstdlib>
#include <set>
#include <FlexLexer.h>
#include "tokeniser.h"
#include <cstring>

using namespace std;

enum OPREL {EQU, DIFF, INF, SUP, INFE, SUPE, WTFR};
enum OPADD {ADD, SUB, OR, WTFA};
enum OPMUL {MUL, DIV, MOD, AND, WTFM};
enum TYPE {UNSIGNED_INT, BOOLEAN};

TOKEN current;
FlexLexer* lexer = new yyFlexLexer;
set<string> DeclaredVariables;
unsigned long TagNumber=0;

bool IsDeclared(const char *id){
	return DeclaredVariables.find(id)!=DeclaredVariables.end();
}

void Error(string s){
	cerr << "Ligne n°"<<lexer->lineno()<<", lu : '"<<lexer->YYText()<<"'("<<current<<"), mais ";
	cerr<< s << endl;
	exit(-1);
}

TYPE Expression(void);
void Statement(void);

// Identifier retourne UNSIGNED_INT (pour l'instant)
TYPE Identifier(void){
	cout << "\tpush "<<lexer->YYText()<<"(%rip)"<<endl;
	current=(TOKEN) lexer->yylex();
	return UNSIGNED_INT;
}

// Number retourne UNSIGNED_INT
TYPE Number(void){
	cout <<"\tpush $"<<atoi(lexer->YYText())<<endl;
	current=(TOKEN) lexer->yylex();
	return UNSIGNED_INT;
}

// Factor retourne le type de ce qu'il a analysé
TYPE Factor(void){
	TYPE type;
	if(current==RPARENT){
		current=(TOKEN) lexer->yylex();
		type=Expression();
		if(current!=LPARENT)
			Error("')' etait attendu");
		else
			current=(TOKEN) lexer->yylex();
	}
	else if(current==NUMBER)
		type=Number();
	else if(current==ID)
		type=Identifier();
	else
		Error("'(' ou chiffre ou lettre attendue");
	return type;
}

OPMUL MultiplicativeOperator(void){
	OPMUL opmul;
	if(strcmp(lexer->YYText(),"*")==0)
		opmul=MUL;
	else if(strcmp(lexer->YYText(),"/")==0)
		opmul=DIV;
	else if(strcmp(lexer->YYText(),"%")==0)
		opmul=MOD;
	else if(strcmp(lexer->YYText(),"&&")==0)
		opmul=AND;
	else opmul=WTFM;
	current=(TOKEN) lexer->yylex();
	return opmul;
}

// Term retourne le type commun de tous les Factor
TYPE Term(void){
	OPMUL mulop;
	TYPE type1, type2;
	type1=Factor();
	while(current==MULOP){
		mulop=MultiplicativeOperator();
		type2=Factor();
		if(type1 != type2)
			Error("Types incompatibles dans Term");
		cout << "\tpop %rbx"<<endl;
		cout << "\tpop %rax"<<endl;
		switch(mulop){
			case AND:
				cout << "\tmulq\t%rbx"<<endl;
				cout << "\tpush %rax\t# AND"<<endl;
				break;
			case MUL:
				cout << "\tmulq\t%rbx"<<endl;
				cout << "\tpush %rax\t# MUL"<<endl;
				break;
			case DIV:
				cout << "\tmovq $0, %rdx"<<endl;
				cout << "\tdiv %rbx"<<endl;
				cout << "\tpush %rax\t# DIV"<<endl;
				break;
			case MOD:
				cout << "\tmovq $0, %rdx"<<endl;
				cout << "\tdiv %rbx"<<endl;
				cout << "\tpush %rdx\t# MOD"<<endl;
				break;
			default:
				Error("operateur multiplicatif attendu");
		}
	}
	return type1;
}

OPADD AdditiveOperator(void){
	OPADD opadd;
	if(strcmp(lexer->YYText(),"+")==0)
		opadd=ADD;
	else if(strcmp(lexer->YYText(),"-")==0)
		opadd=SUB;
	else if(strcmp(lexer->YYText(),"||")==0)
		opadd=OR;
	else opadd=WTFA;
	current=(TOKEN) lexer->yylex();
	return opadd;
}

// SimpleExpression retourne le type commun de tous les Term
TYPE SimpleExpression(void){
	OPADD adop;
	TYPE type1, type2;
	type1=Term();
	while(current==ADDOP){
		adop=AdditiveOperator();
		type2=Term();
		if(type1 != type2)
			Error("Types incompatibles dans SimpleExpression");
		cout << "\tpop %rbx"<<endl;
		cout << "\tpop %rax"<<endl;
		switch(adop){
			case OR:
				cout << "\taddq\t%rbx, %rax\t# OR"<<endl;
				break;
			case ADD:
				cout << "\taddq\t%rbx, %rax\t# ADD"<<endl;
				break;
			case SUB:
				cout << "\tsubq\t%rbx, %rax\t# SUB"<<endl;
				break;
			default:
				Error("operateur additif inconnu");
		}
		cout << "\tpush %rax"<<endl;
	}
	return type1;
}

void DeclarationPart(void){
	if(current!=RBRACKET)
		Error("caractere '[' attendu");
	cout << "\t.data"<<endl;
	cout << "\t.align 8"<<endl;
	current=(TOKEN) lexer->yylex();
	if(current!=ID)
		Error("Un identificateur etait attendu");
	cout << lexer->YYText() << ":\t.quad 0"<<endl;
	DeclaredVariables.insert(lexer->YYText());
	current=(TOKEN) lexer->yylex();
	while(current==COMMA){
		current=(TOKEN) lexer->yylex();
		if(current!=ID)
			Error("Un identificateur etait attendu");
		cout << lexer->YYText() << ":\t.quad 0"<<endl;
		DeclaredVariables.insert(lexer->YYText());
		current=(TOKEN) lexer->yylex();
	}
	if(current!=LBRACKET)
		Error("caractere ']' attendu");
	current=(TOKEN) lexer->yylex();
}

OPREL RelationalOperator(void){
	OPREL oprel;
	if(strcmp(lexer->YYText(),"==")==0)
		oprel=EQU;
	else if(strcmp(lexer->YYText(),"!=")==0)
		oprel=DIFF;
	else if(strcmp(lexer->YYText(),"<")==0)
		oprel=INF;
	else if(strcmp(lexer->YYText(),">")==0)
		oprel=SUP;
	else if(strcmp(lexer->YYText(),"<=")==0)
		oprel=INFE;
	else if(strcmp(lexer->YYText(),">=")==0)
		oprel=SUPE;
	else oprel=WTFR;
	current=(TOKEN) lexer->yylex();
	return oprel;
}

// Expression retourne BOOLEAN si opérateur relationnel, sinon le type de SimpleExpression
TYPE Expression(void){
	OPREL oprel;
	TYPE type1, type2;
	type1=SimpleExpression();
	if(current==RELOP){
		oprel=RelationalOperator();
		type2=SimpleExpression();
		if(type1 != type2)
			Error("Types incompatibles dans Expression");
		cout << "\tpop %rax"<<endl;
		cout << "\tpop %rbx"<<endl;
		cout << "\tcmpq %rax, %rbx"<<endl;
		switch(oprel){
			case EQU:
				cout << "\tje Vrai"<<++TagNumber<<"\t# If equal"<<endl;
				break;
			case DIFF:
				cout << "\tjne Vrai"<<++TagNumber<<"\t# If different"<<endl;
				break;
			case SUPE:
				cout << "\tjae Vrai"<<++TagNumber<<"\t# If above or equal"<<endl;
				break;
			case INFE:
				cout << "\tjbe Vrai"<<++TagNumber<<"\t# If below or equal"<<endl;
				break;
			case INF:
				cout << "\tjb Vrai"<<++TagNumber<<"\t# If below"<<endl;
				break;
			case SUP:
				cout << "\tja Vrai"<<++TagNumber<<"\t# If above"<<endl;
				break;
			default:
				Error("Operateur de comparaison inconnu");
		}
		cout << "\tpush $0\t\t# False"<<endl;
		cout << "\tjmp Suite"<<TagNumber<<endl;
		cout << "Vrai"<<TagNumber<<":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True"<<endl;
		cout << "Suite"<<TagNumber<<":"<<endl;
		return BOOLEAN;  // operateur relationnel -> BOOLEAN
	}
	return type1;  // pas d'operateur relationnel -> type de SimpleExpression
}

// AssignementStatement verifie que variable et expression sont du meme type
void AssignementStatement(void){
	string variable;
	TYPE type;
	if(current!=ID)
		Error("Identificateur attendu");
	if(!IsDeclared(lexer->YYText())){
		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non declaree"<<endl;
		exit(-1);
	}
	variable=lexer->YYText();
	current=(TOKEN) lexer->yylex();
	if(current!=ASSIGN)
		Error("caracteres ':=' attendus");
	current=(TOKEN) lexer->yylex();
	type=Expression();
	// Pour l'instant toutes les variables sont UNSIGNED_INT
	if(type != UNSIGNED_INT)
		Error("Type incompatible dans l'affectation : UNSIGNED_INT attendu");
	cout << "\tpop "<<variable<<"(%rip)"<<endl;
	cout << "\tpush "<<variable<<"(%rip)"<<endl;
	cout << "\tpop %rax"<<endl;
}

// IfStatement verifie que l'expression est BOOLEAN
void IfStatement(void){
	unsigned long tag = ++TagNumber;
	TYPE type;
	current = (TOKEN) lexer->yylex();
	type=Expression();
	if(type != BOOLEAN)
		Error("Expression booleenne attendue apres IF");
	cout << "\tpop %rax" << endl;
	cout << "\tcmpq $0, %rax" << endl;
	cout << "\tje Faux" << tag << endl;
	if(current != KEYWORD_THEN)
		Error("'THEN' attendu");
	current = (TOKEN) lexer->yylex();
	Statement();
	if(current == KEYWORD_ELSE){
		cout << "\tjmp Suite" << tag << endl;
		cout << "Faux" << tag << ":" << endl;
		current = (TOKEN) lexer->yylex();
		Statement();
		cout << "Suite" << tag << ":" << endl;
	}
	else{
		cout << "Faux" << tag << ":" << endl;
	}
}

// WhileStatement verifie que l'expression est BOOLEAN
void WhileStatement(void){
	unsigned long tag = ++TagNumber;
	TYPE type;
	cout << "While" << tag << ":" << endl;
	current = (TOKEN) lexer->yylex();
	type=Expression();
	if(type != BOOLEAN)
		Error("Expression booleenne attendue apres WHILE");
	cout << "\tpop %rax" << endl;
	cout << "\tcmpq $0, %rax" << endl;
	cout << "\tje EndWhile" << tag << endl;
	if(current != KEYWORD_DO)
		Error("'DO' attendu");
	current = (TOKEN) lexer->yylex();
	Statement();
	cout << "\tjmp While" << tag << endl;
	cout << "EndWhile" << tag << ":" << endl;
}

void BlockStatement(void){
	current = (TOKEN) lexer->yylex();
	Statement();
	while(current == SEMICOLON){
		current = (TOKEN) lexer->yylex();
		Statement();
	}
	if(current != KEYWORD_END)
		Error("'END' attendu");
	current = (TOKEN) lexer->yylex();
}

void Statement(void){
	if(current == KEYWORD_IF)
		IfStatement();
	else if(current == KEYWORD_WHILE)
		WhileStatement();
	else if(current == KEYWORD_BEGIN)
		BlockStatement();
	else
		AssignementStatement();
}

void StatementPart(void){
	cout << "\t.text\t\t# The following lines contain the program"<<endl;
	cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
	cout << "main:\t\t\t# The main function body :"<<endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;
	Statement();
	while(current==SEMICOLON){
		current=(TOKEN) lexer->yylex();
		Statement();
	}
	if(current!=DOT)
		Error("caractere '.' attendu");
	current=(TOKEN) lexer->yylex();
}

void Program(void){
	if(current==RBRACKET)
		DeclarationPart();
	StatementPart();
}

int main(void){
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	current=(TOKEN) lexer->yylex();
	Program();
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(current!=FEOF){
		cerr <<"Caracteres en trop a la fin du programme : ["<<current<<"]";
		Error(".");
	}
}