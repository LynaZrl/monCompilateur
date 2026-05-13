//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin

#include <string>
#include <iostream>
#include <cstdlib>
#include <set>

using namespace std;

char current, lookedAhead;
int NLookedAhead = 0;
set<char> declaredVariables;

void ReadChar(void){
    if(NLookedAhead > 0){
        current = lookedAhead;
        NLookedAhead--;
    }
    else
        while(cin.get(current) && (current==' '||current=='\t'||current=='\n'));
}

void LookAhead(void){
    while(cin.get(lookedAhead) && (lookedAhead==' '||lookedAhead=='\t'||lookedAhead=='\n'));
    NLookedAhead++;
}

void Error(string s){
    cerr << s << endl;
    exit(-1);
}

void Expression(void);

void Number(void){
    if(current < '0' || current > '9')
        Error("Chiffre attendu");
    string num = "";
    while(current >= '0' && current <= '9'){
        num += current;
        ReadChar();
    }
    cout << "\tpush $" << num << endl;
}

void Factor(void){
    if(current >= '0' && current <= '9'){
        Number();
    }
    else if(current >= 'a' && current <= 'z'){
        if(declaredVariables.find(current) == declaredVariables.end())
            Error(string("Variable non declaree : ") + current);
        cout << "\tpush " << current << "(%rip)" << endl;
        ReadChar();
    }
    else if(current == '('){
        ReadChar();
        Expression();
        if(current != ')')
            Error("')' attendu");
        ReadChar();
    }
    else if(current == '!'){
        ReadChar();
        Factor();
        cout << "\tpop %rax" << endl;
        cout << "\tnotq %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
    else
        Error("Facteur invalide");
}

string MultiplicativeOperator(void){
    string op;
    if(current == '*'){
        op = "*"; ReadChar();
    }
    else if(current == '/'){
        op = "/"; ReadChar();
    }
    else if(current == '%'){
        op = "%"; ReadChar();
    }
    else if(current == '&'){
        LookAhead();
        if(lookedAhead == '&'){
            op = "&&";
            ReadChar();
            ReadChar();
        }
        else
            Error("'&&' attendu");
    }
    else
        Error("Operateur multiplicatif attendu");
    return op;
}

void Term(void){
    Factor();
    while(current == '*' || current == '/' || current == '%' || current == '&'){
        if(current == '&'){
            LookAhead();
            if(lookedAhead != '&') break;
        }
        string op = MultiplicativeOperator();
        Factor();
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        if(op == "*")
            cout << "\timulq %rbx, %rax" << endl;
        else if(op == "/"){
            cout << "\tcqto" << endl;
            cout << "\tidivq %rbx" << endl;
        }
        else if(op == "%"){
            cout << "\tcqto" << endl;
            cout << "\tidivq %rbx" << endl;
            cout << "\tmovq %rdx, %rax" << endl;
        }
        else if(op == "&&")
            cout << "\tandq %rbx, %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
}

string AdditiveOperator(void){
    string op;
    if(current == '+'){
        op = "+"; ReadChar();
    }
    else if(current == '-'){
        op = "-"; ReadChar();
    }
    else if(current == '|'){
        LookAhead();
        if(lookedAhead == '|'){
            op = "||";
            ReadChar();
            ReadChar();
        }
        else
            Error("'||' attendu");
    }
    else
        Error("Operateur additif attendu");
    return op;
}

void SimpleExpression(void){
    Term();
    while(current == '+' || current == '-' || current == '|'){
        if(current == '|'){
            LookAhead();
            if(lookedAhead != '|') break;
        }
        string op = AdditiveOperator();
        Term();
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        if(op == "+")
            cout << "\taddq %rbx, %rax" << endl;
        else if(op == "-")
            cout << "\tsubq %rbx, %rax" << endl;
        else if(op == "||")
            cout << "\torq %rbx, %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
}

string RelationalOperator(void){
    string op;
    if(current == '=' && lookedAhead == '='){
        op = "==";
        ReadChar();
        ReadChar();
    }
    else if(current == '!' && lookedAhead == '='){
        op = "!=";
        ReadChar();
        ReadChar();
    }
    else if(current == '<'){
        LookAhead();
        if(lookedAhead == '='){
            op = "<=";
            ReadChar();
            ReadChar();
        }
        else{
            op = "<";
            current = lookedAhead;
            NLookedAhead = 0;
        }
    }
    else if(current == '>'){
        LookAhead();
        if(lookedAhead == '='){
            op = ">=";
            ReadChar();
            ReadChar();
        }
        else{
            op = ">";
            current = lookedAhead;
            NLookedAhead = 0;
        }
    }
    else
        Error("Operateur relationnel attendu");
    return op;
}

void Expression(void){
    SimpleExpression();
    if(current == '<' || current == '>'){
        string op = RelationalOperator();
        SimpleExpression();
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        cout << "\tcmpq %rbx, %rax" << endl;
        if(op == "<")       cout << "\tsetl %al" << endl;
        else if(op == "<=") cout << "\tsetle %al" << endl;
        else if(op == ">")  cout << "\tsetg %al" << endl;
        else if(op == ">=") cout << "\tsetge %al" << endl;
        cout << "\tmovzbq %al, %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
    else if(current == '=' || current == '!'){
        LookAhead();
        if(lookedAhead == '='){
            string op = RelationalOperator();
            SimpleExpression();
            cout << "\tpop %rbx" << endl;
            cout << "\tpop %rax" << endl;
            cout << "\tcmpq %rbx, %rax" << endl;
            if(op == "==")      cout << "\tsete %al" << endl;
            else if(op == "!=") cout << "\tsetne %al" << endl;
            cout << "\tmovzbq %al, %rax" << endl;
            cout << "\tpush %rax" << endl;
        }
    }
}

void AssignementStatement(void){
    if(current < 'a' || current > 'z')
        Error("Variable attendue");
    char varName = current;
    if(declaredVariables.find(varName) == declaredVariables.end())
        Error(string("Variable non declaree : ") + varName);
    ReadChar();
    if(current != '=')
        Error("'=' attendu pour l'affectation");
    ReadChar();
    Expression();
    cout << "\tpop %rax" << endl;
    cout << "\tmovq %rax, " << varName << "(%rip)" << endl;
}

void Statement(void){
    AssignementStatement();
}

void StatementPart(void){
    Statement();
    while(current == ';'){
        ReadChar();
        Statement();
    }
    if(current != '.')
        Error("'.' attendu en fin de programme");
    ReadChar();
}

void DeclarationPart(void){
    if(current != '[')
        Error("'[' attendu");
    ReadChar();
    if(current < 'a' || current > 'z')
        Error("Variable attendue apres '['");
    while(true){
        if(current < 'a' || current > 'z')
            Error("Lettre attendue");
        char varName = current;
        declaredVariables.insert(varName);
        ReadChar();
        if(current == ']') break;
        if(current != ',')
            Error("',' ou ']' attendu");
        ReadChar();
    }
    if(current != ']')
        Error("']' attendu");
    ReadChar();
    cout << "\t.data" << endl;
    for(char v : declaredVariables){
        cout << v << ":\t.quad 0" << endl;
    }
}

int main(void){
    ReadChar();
    if(current == '['){
        DeclarationPart();
    }
    cout << "\t\t\t# This code was produced by the CERI Compiler" << endl;
    cout << "\t.globl main\t# The main function must be visible from outside" << endl;
    cout << "\t.text\t\t# The following lines contain the program" << endl;
    cout << "main:\t\t\t# The main function body :" << endl;
    cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top" << endl;
    StatementPart();
    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
    return 0;
}