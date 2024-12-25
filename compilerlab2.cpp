#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <stack>
// #include <strstream>
#include <regex>

#define VAR "[A-Za-z_]\\w*"
#define PRINTLN_INT "\\s*println_int\\s*\\(\\s*[A-Za-z_]\\w*\\s*\\)\\s*;"
#define SENTENCE "\\{[^\\}]*\\}|[^;\n\\{]+;|(\\s*int\\s*[A-Za-z_]\\w*\\s*\\(\\s*((int|string|char)\\s*[A-Za-z_]\\w*\\s*)?(,\\s*(int|string|char)\\s*[A-Za-z_]\\w*\\s*)*\\))"
#define ASSIGNMENT "\\s*[A-Za-z_]\\w*\\s*=\\s*[^;\n]+;"
#define SNIPPET "\\s*\\{[^\\}]*\\}"
#define VAR_DECLARATION "\\s*(int|string|char)\\s*[A-Za-z_]\\w*\\s*;"
#define FUNC_DECLARATION "\\s*int\\s*[A-Za-z_]\\w*\\s*\\(\\s*((int|string|(char\\s*\\*)|char)\\s*[A-Za-z_]\\w*\\s*)?(,\\s*(int|string|char|(char\\s*\\*))\\s*[A-Za-z_]\\w*\\s*)*\\)"
#define RETURN "\\s*return\\s*([A-Za-z_]\\w*|[1-9]\\d*|0)\\s*;"
#define KEYWORD "int|main|return|println_int"

using namespace std;

struct var{
    string name;
    int pos;
    // int size;
    string value;
};

class VarStack{
private:
    int sp;
public:
    VarStack(){
        sp=0;
    }

    string push(string reg){
        // string s = "sw $";
        // s += str + ", 0($sp)\n" + "addiu $sp, $sp, -4\n";
        string s = "push ";
        s += reg + "\n";
        sp -= 4;
        return s;
    }
    string pop(string reg){
        // pop需要给出存放的寄存器
        string s = "pop "; //"lw $";
        s += reg + "\n"; // + ", 4($sp)\n" + "addiu $sp, $sp, 4\n";
        sp += 4;
        return s;
    }
};

string CtoMips(string code);
// string int2str(int digital);
string findVarPos(string name);
string cal(string opt);
int priority(string s);
string expressionHandler(var& var, string expression);
string println_int(string var);
list<string> expression2token(string expression);

list<var> vars;
int var_pos = -4;

int main(int argc, char * argv[]) {
    string path = argv[1];
    // string path = "input.txt";
    ifstream file_in; 

    list<string> tokens;
    string token;
    string code;
    
    file_in.open(path, ios::in); 
    // list<string>::iterator nextBegin = tokens.begin();
    while(file_in >> token){
        // tokens.emplace_back(token);
        // if(tokens.back() == ";"){
        //     list<string> sentence(tokens.begin(), --tokens.end());
        //     tokens.erase(tokens.begin(), tokens.end());
        //     string mipsCode = CtoMips(sentence);
        //     cout << mipsCode << endl;
        // }
        // cout << tokens.back() << endl;
        code += token + " ";
    }
    // cout << code;
    string mips = CtoMips(code);
    mips += "leave\nret";
    cout << mips << endl;

    // for(auto &var: vars)
    //     cout << var.name << var.pos << var.value << endl;
    return 0;
}

string CtoMips(string code)
{   
    string mips;
    // 这里可能需要封装一下
    regex var_reg(VAR); // 匹配对应变量名
    regex sentence_reg(SENTENCE);
    regex snippet_reg(SNIPPET);
    regex assignment_reg(ASSIGNMENT);
    regex var_declaration_reg(VAR_DECLARATION);
    regex return_reg(RETURN);
    regex println_int_reg(PRINTLN_INT);
    regex func_declaration_reg(FUNC_DECLARATION);
    /*
    // 这里需要匹配代码段第一句 对句子分类之后具体处理
    // list<string>::iterator it = sentence.begin();
    */
    // 先分句，在匹配
    sregex_iterator current_match(code.begin(), code.end(), sentence_reg);
    sregex_iterator last_match;
    while(current_match != last_match){
        smatch match_result = *current_match;
        string token = match_result.str();
        // 匹配函数声明 这里不做实现
        if(regex_match(token, func_declaration_reg)){
            mips += ".intel_syntax noprefix\n";
            mips += ".global main\n";
            mips += ".extern printf\n";
            mips += ".data\n";
            mips += "format_str:\n";
            mips += " .asciz \"%d\\n\"\n";
            mips += ".text\n";
            mips += "main:\n";
            mips += "push ebp\n";
            mips += "mov ebp, esp\n";
            mips += "sub esp, 0x100\n\n" ;
        }
        // 先匹配代码段，若为代码段：递归调用
        else if(regex_match(token, snippet_reg)){
            regex reg("\\{|\\}");
            token = regex_replace(token, reg, "");
            mips += CtoMips(token);
        }
        // 变量声明 目前只匹配int
        else if(regex_match(token, var_declaration_reg)){
            // 若要区分不同变量类型，需要先确定声明变量类型，然后按照变量名依次声明
            // 这里为了简化 固定声明为int 类型，变量名从分句第四位开始匹配
            regex space("\\s*");
            regex keyword_reg(KEYWORD);
            token = regex_replace(token, keyword_reg, "");
            token = regex_replace(token, space, "");
            sregex_iterator var_match(token.begin(), token.end(), var_reg);
            while(var_match != last_match){
                var temp;
                smatch var_result = *var_match;
                string var_name = var_result.str();
                temp.name = var_name;
                temp.pos = var_pos;
                var_pos -= 4;
                vars.emplace_back(temp);
                mips += "mov DWORD PTR [ebp" + to_string(temp.pos) + "], 0\n";
                var_match++;
            }
        }
        // 赋值语句
        else if(regex_match(token, assignment_reg)){
            smatch s_var;
            smatch s_expression;
            regex assignable_left_reg("[A-Za-z_]\\w*(?=\\s*=)"); // 可赋值左值
            regex expression_reg("[^;]*;"); // 匹配表达式

            regex_search(token, s_var, assignable_left_reg);
            string var_name = s_var.str();

            regex space("\\s*");
            token = regex_replace(token, space, "");
            
            token = regex_replace(token, assignable_left_reg, ""); // 删除左值
            string expression = token.substr(1, token.length()-2); // 掐头 去尾 截去= ;
            // regex_search(token, s_expression, expression_reg);
            // string expression = s_expression.str();
            
            // 在变量表中寻找
            for(auto &var: vars){
                if(var.name == var_name){
                    mips += expressionHandler(var, expression);
                    break;
                }
            }
        }
        // return 此处仅处理返回单个变量
        else if(regex_match(token, return_reg)){
            smatch var_match;
            regex kw("return\\s*");
            regex _digit("[1-9]\\d*|0");
            token = regex_replace(token, kw, "");
            if (regex_search(token, var_match, var_reg)){
                string var_name = var_match.str();
                string pos = findVarPos(var_name);
                mips += "mov eax, DWORD PTR [ebp" + pos + "]\n";
            }
            else if (regex_search(token, var_match, _digit)){
                string digit = var_match.str();
                mips += "mov eax, " + digit + "\n";
            }
        }
        else if(regex_match(token, println_int_reg)){
            regex var_reg_func("\\([A-Za-z_]\\w*\\)"); // 用于匹配括号内 单 变量名
            smatch var_match;
            regex space("\\s*");
            token = regex_replace(token, space, "");
            regex_search(token, var_match, var_reg_func);
            string var_name = var_match.str();
            // var_name = regex_replace(var_name, _lr, "");
            mips += println_int(var_name.substr(1, var_name.length()-2));
        }
        else{
            mips +="\"" + token + "\" is not finished yet\n";
        }
        current_match++;
    }
    return mips;
}

// string int2str(int digital){
//     strstream ss;
//     string s;
//     ss << digital;
//     ss >> s;
//     return s;
// }

string expressionHandler(var& var, string expression){
    // 将赋值号右侧当作表达式统一处理
    regex _const("^[1-9]\\d*|0$"); // 非负整数
    regex _operator(">=|<=|==|!=|[-\\+\\*/|\\^&%><]");
    regex _var(VAR);
    regex _left("\\(");
    regex _right("\\)");
    // 考虑所给示例中的实现
    // 操作数的栈在汇编语言中实现，操作符的栈在此处实现，但仍需记录操作数地址，对于立即数同变量处理
    // 总之还是需要实现两个栈
    VarStack vs;
    stack<string> optStack;

    int stack_top=0;
    string mips;
    // 将表达式处理为token流
    list<string> token_list = expression2token(expression);
    for(auto token_iter=token_list.begin(); token_iter != token_list.end(); token_iter++){
        // cout << raw << endl;
        string token = *token_iter;
        if (regex_match(token, _const)){ // 处理常量
            // 加载立即数, 直接入栈
            mips += "mov eax, " + token + "\n";
            mips += vs.push("eax");
        }
        else if(regex_match(token, _var)){ // 处理变量
            // 变量提取地址，入栈
            string pos = findVarPos(token);
            mips += "mov eax, DWORD PTR [ebp" + pos + "]\n";
            mips += vs.push("eax");
        }
        else if(regex_match(token, _operator)){ // 处理运算符
            //如果当前运算符的优先级>栈顶符号优先级，则直接进栈
            if(optStack.empty()||priority(token)> priority(optStack.top()))
                optStack.push(token);
            //如果当前运算符的优先级<=栈顶符号的优先级
            else{
                //依次弹出优先级高于或等于当前符号的所有符号，并同时弹出数字栈中的两个数字作为操作数。注意先后顺序。
                while(!optStack.empty()&&priority(optStack.top())>= priority(token)){
                    string op2 = "ebx";
                    mips += vs.pop(op2);
                    string op1 = "eax";
                    mips += vs.pop(op1);
                    //将计算结果压入数字栈
                    mips += cal(optStack.top());
                    if (optStack.top() == "%")
                        mips += vs.push("edx");
                    else
                        mips += vs.push("eax");
                    //符号出栈
                    optStack.pop();
                }
                optStack.push(token);//最后把当前符号入栈
            }
        }
        else{
            if(regex_match(token, _left))
                optStack.push(token);
            else if(regex_match(token, _right)){
                //遇到右括号')'依次弹出符号栈内运算符，和数字栈内的两个运算符，做计算，直到遇到左括号'('为止
                while(!regex_match(optStack.top(), _left)){
                    string op2 = "ebx";
                    mips += vs.pop(op2);
                    string op1 = "eax";
                    mips += vs.pop(op1);
                    //将计算结果压入数字栈
                    mips += cal(optStack.top());
                    if (optStack.top() == "%")
                        mips += vs.push("edx");
                    else
                        mips += vs.push("eax");
                    //符号出栈
                    optStack.pop();
                }
                optStack.pop();
            }
        }
    }
    // 表达式处理完，保存
    // 将符号栈所有运算符出栈，与数字栈剩余数字做运算
    while(!optStack.empty()){
        string op2 = "ebx";
        mips += vs.pop(op2);
        string op1 = "eax";
        mips += vs.pop(op1);
        //将计算结果压入数字栈
        mips += cal(optStack.top());
        if (optStack.top() == "%")
            mips += vs.push("edx");
        else
            mips += vs.push("eax");
        //符号出栈
        optStack.pop();
    }
    mips += vs.pop("eax");
    mips += "mov DWORD PTR [ebp" + to_string(var.pos) + "], eax\n";
    return mips;// +"\n\"" + raw + "\" length: " + to_string(expression.size());
}

string findVarPos(string name){
    for(auto &var: vars){
        if(var.name == name){
            string pos;
            pos = to_string(var.pos);
            return pos;
        }
    }
    return "var not found!\n";
}

int priority(string s){
    // 
    if(s == "+" || s=="-")
        return 6;
    else if (s=="*"||s=="/"||s=="%")
        return 7;
    else if (s=="|")
        return 1;
    else if (s=="^")
        return 2;
    else if (s=="&")
        return 3;
    else if (s=="!="||s=="==")
        return 4;
    else if (s==">"||s==">="||s=="<"||s=="<=")
        return 5;
        //设括号优先级为0
    else return 0;
}

// map<string, int> priority = {
//         {"+", 1},
//         {"-", 1},
//         {"*", 2},
//         {"/", 2}
//     };

//四则运算
string cal(string op){
    string mips;
    if (op == "+") {
        mips = "add eax, ebx\n\n";
    }
    else if (op == "-") {
        mips = "sub eax, ebx\n\n";
    }
    else if (op == "*") {
        mips = "imul eax, ebx\n\n";
    }
    else if (op == "/") {
        mips = "cdq\nidiv eax, ebx\n\n";
    }
    else if (op == "%") {
        mips = "cdq\nidiv eax, ebx\n\n";
    }
    else if (op == "<") {
        mips = "cmp eax, ebx\nsetl al\nmovzx eax, al\n\n";
    }
    else if (op == "<=") {
        mips = "cmp eax, ebx\nsetle al\nmovzx eax, al\n\n";
    }
    else if (op == ">") {
        mips = "cmp eax, ebx\nsetg al\nmovzx eax, al\n\n";
    }
    else if (op == ">=") {
        mips = "cmp eax, ebx\nsetge al\nmovzx eax, al\n\n";
    }
    else if (op == "==") {
        mips = "cmp eax, ebx\nsete al\nmovzx eax, al\n\n";
    }
    else if (op == "!=") {
        mips = "cmp eax, ebx\nsetne al\nmovzx eax, al\n\n";
    }
    else if (op == "&") {
        mips = "and eax, ebx\n\n";
    }
    else if (op == "|") {
        mips = "or eax, ebx\n\n";
    }
    else if (op == "^") {
        mips = "xor eax, ebx\n\n";
    }
    return mips;
}

string println_int(string var){
    string pos=findVarPos(var);
    string instruct;

    instruct  = "push DWORD PTR [ebp" + pos + "]\n";
    instruct += "push offset format_str\n";
    instruct += "call printf\n";
    instruct += "add esp, 8\n\n";
    return instruct;
}

list<string> expression2token(string expression){
    list<string> token;

    regex _just_put_them_all_together(">=|<=|==|!=|([-\\+\\*/|\\^&%><\\(\\)])|([A-Za-z_]\\w*|[1-9]\\d*|0)");
    regex space(";");

    expression = regex_replace(expression, space, "");

    sregex_iterator all_match(expression.begin(), expression.end(), _just_put_them_all_together);
    sregex_iterator last_match;
    while(all_match != last_match){
        smatch all_result = *all_match;
        token.emplace_back(all_result.str());
        all_match++;
    }
    return token;
}