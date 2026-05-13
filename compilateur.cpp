#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

char current;
char nextcar;

void ReadChar(void){
    if(!cin.get(current)){
        current = ' ';
        return;
    }
    while((current == ' ' || current == '\t' || current == '\n') && cin.get(current));
}

void Error(string s){
    cerr << s << endl;
    exit(-1);
}

void AdditiveOperator(void){
    if(current == '+' || current == '-')
        ReadChar();
    else
        Error("Opérateur additif attendu");
}

void Digit(void){
    if(current < '0' || current > '9')
        Error("Chiffre attendu");
    else{
        cout << "\tpush $" << current << endl;
        ReadChar();
    }
}

void ArithmeticExpression(void);

void Term(void){
    if(current == '('){
        ReadChar();
        ArithmeticExpression();
        if(current != ')')
            Error("')' était attendu");
        else
            ReadChar();
    }
    else if(current >= '0' && current <= '9')
        Digit();
    else
        Error("'(' ou chiffre attendu");
}

void ArithmeticExpression(void){
    char adop;
    Term();
    while(current == '+' || current == '-'){
        adop = current;
        AdditiveOperator();
        Term();
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        if(adop == '+')
            cout << "\taddq\t%rbx, %rax" << endl;
        else
            cout << "\tsubq\t%rbx, %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
}

void RelationalOperator(string &op){
    if(current == '='){
        op = "==";
        ReadChar();
    }
    else if(current == '<'){
        cin.get(nextcar);
        if(nextcar == '>'){
            op = "<>";
            ReadChar();
        }
        else if(nextcar == '='){
            op = "<=";
            ReadChar();
        }
        else{
            current = nextcar;
            op = "<";
        }
    }
    else if(current == '>'){
        cin.get(nextcar);
        if(nextcar == '='){
            op = ">=";
            ReadChar();
        }
        else{
            current = nextcar;
            op = ">";
        }
    }
    else
        Error("Opérateur relationnel attendu");
}

void Expression(void){
    string relop;
    ArithmeticExpression();
    if(current == '=' || current == '<' || current == '>'){
        RelationalOperator(relop);
        ArithmeticExpression();
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        cout << "\tcmpq\t%rbx, %rax" << endl;
        if(relop == "==")
            cout << "\tsete\t%al" << endl;
        else if(relop == "<>")
            cout << "\tsetne\t%al" << endl;
        else if(relop == "<")
            cout << "\tsetl\t%al" << endl;
        else if(relop == "<=")
            cout << "\tsetle\t%al" << endl;
        else if(relop == ">")
            cout << "\tsetg\t%al" << endl;
        else if(relop == ">=")
            cout << "\tsetge\t%al" << endl;
        cout << "\tmovzbq\t%al, %rax" << endl;
        cout << "\tpush %rax" << endl;
    }
}

int main(void){
    cout << "\t\t\t# This code was produced by the CERI Compiler" << endl;
    cout << "\t.text\t\t# The following lines contain the program" << endl;
    cout << "\t.globl main\t# The main function must be visible from outside" << endl;
    cout << "main:\t\t\t# The main function body :" << endl;
    cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top" << endl;
    ReadChar();
    Expression();
    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
    if(cin.get(current)){
        cerr << "Caractères en trop à la fin du programme : [" << current << "]";
        Error(".");
    }
}