#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <stack>
// #include <strstream>
#include <regex>
#include <vector>

#define VAR "[A-Za-z_]\\w*"
#define PRINTLN_INT "\\s*println_int\\s*\\(.*\\)\\s*;"
#define SENTENCE "(\\{(?:\\g<0>|[^{}])*\\})|[^;\n\\{]+;|(\\s*(int|void)\\s*[A-Za-z_]\\w*\\s*\\(\\s*((int|string|char)\\s*[A-Za-z_]\\w*\\s*)?(,\\s*(int|string|char)\\s*[A-Za-z_]\\w*\\s*)*\\))"
#define ASSIGNMENT "\\s*[A-Za-z_]\\w*\\s*=\\s*[^;\n]+;"
#define SNIPPET "\\s*\\{.*\\}"
#define VAR_DECLARATION "\\s*(int)\\s*(\\s*(([A-Za-z_]\\w*)|([A-Za-z_]\\w*\\s*=\\s*[^,]+))\\s*,?)+\\s*;"
#define FUNC_DECLARATION "\\s*(int|void)\\s*[A-Za-z_]\\w*\\s*\\(\\s*((int|string|(char\\s*\\*)|char)\\s*[A-Za-z_]\\w*\\s*)?(,\\s*(int|string|char|(char\\s*\\*))\\s*[A-Za-z_]\\w*\\s*)*\\)\\s*"
#define RETURN "\\s*return\\s*[^;]+;"
#define KEYWORD "int|void|return|println_int"

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
        string s = " push ";
        s += reg + "\n";
        sp -= 4;
        return s;
    }
    string pop(string reg){
        // pop需要给出存放的寄存器
        string s = " pop "; //"lw $";
        s += reg + "\n"; // + ", 4($sp)\n" + "addiu $sp, $sp, 4\n";
        sp += 4;
        return s;
    }
};

struct func{
    string func_name;
    string func_type;
    list<var> local_var;
};

string CtoMips(string code);
// string int2str(int digital);
string findVarPos(string name);
string cal(string opt);
int priority(string s);
string expressionHandler(string expression);
string println_int(string var);
list<string> expression2token(string expression);
string assignment(string token);
string func_call(string func_expression, vector<string> tokens, int& i);
string func_declare(string func);
vector<string> scanner(string code);

list<var> global_vars;
vector<func> global_func;
int var_pos = -4;
func* current_func;
int if_count=0;
stack<string> loop_stack;

int main(int argc, char * argv[]) {
    // string path = argv[1];
    string path = "input.c";
    string out_path = "x86.s";
    ifstream file_in; 
    ofstream file_out;

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

    string prefix;
    string mips;

    mips += CtoMips(code);
    prefix += ".intel_syntax noprefix\n";
    for (func temp: global_func){
        prefix += ".global " + temp.func_name + "\n";
    }
    prefix += ".extern printf\n";
    prefix += ".data\n";
    prefix += "format_str:\n";
    prefix += " .asciz \"%d\\n\"\n";
    prefix += " .text\n";

    file_out.open(out_path, ios::out);
    file_out << prefix << endl;
    file_out << mips << endl;

    cout << prefix << endl;
    cout << mips << endl;
    
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
    regex call_func_reg("\\s*[A-Za-z_]\\w*\\s*\\((\\s*[^,]\\s*)?(,\\s*[^,]\\s*)*\\)");
    regex continue_reg("\\s*continue\\s*;\\s*");
    regex break_reg("\\s*break\\s*;\\s*");

    // 这里需要匹配代码段第一句 对句子分类之后具体处
    // list<string>::iterator it = sentence.begin();
    // 先分句，在匹配

    // sregex_iterator current_match(code.begin(), code.end(), sentence_reg);
    sregex_iterator last_match;
    // while(current_match != last_match)
    vector<string> tokens = scanner(code);
    // for (string token: tokens)
    //     cout << token << endl << "------------" << endl;
    for (int i = 0; i < tokens.size(); i++)
    {
        string token = tokens[i];
        // smatch match_result = *current_match;
        // string token = match_result.str();
        // 匹配函数声明
        if(regex_match(token, func_declaration_reg)){
            // regex keyword_reg(KEYWORD);
            // string temp = regex_replace(token, keyword_reg, "");
            // smatch s_name;
            // regex_search(temp, s_name, var_reg);
            // string func_name = s_name.str();
            // // for (func& temp: global_func)
            // //     if (temp.func_name == func_name)
            // //         current_func = &temp;
            mips += func_declare(token);
            // mips += CtoMips(token);
        }
        // 先匹配代码段，若为代码段：递归调用
        else if(regex_match(token, snippet_reg)){
            token = token.substr(1, token.length()-2);
            mips += CtoMips(token);
            // mips += " leave\n ret\n";
        }
        // 变量声明 目前只匹配int
        else if(regex_match(token, var_declaration_reg)){
            // 若要区分不同变量类型，需要先确定声明变量类型，然后按照变量名依次声明
            // 这里为了简化 固定声明为int类型
            // 这里需要分类，分为单纯的变量声明 以及 赋值声明
            regex space("\\s*");
            regex keyword_reg(KEYWORD);

            regex assign_declare("([A-Za-z_]\\w*=[^,;]+)|([A-Za-z_]\\w*)"); // 匹配赋值语句或者变量名
            regex assign_reg_for_declaration("[A-Za-z_]\\w*=[^,;]+[,;]");
            token = regex_replace(token, keyword_reg, "");
            token = regex_replace(token, space, "");
            sregex_iterator declare_match(token.begin(), token.end(), assign_declare);
            while(declare_match != last_match){
                smatch declare_result = *declare_match;
                string declaration = declare_result.str();
                mips += "/* var declaration: " + declaration + " */\n";
                if (regex_match(declaration, var_reg)){
                    var temp;
                    temp.name = declaration;
                    temp.pos = var_pos;
                    var_pos -= 4;
                    current_func->local_var.emplace_back(temp);
                    mips += " mov DWORD PTR [ebp" + to_string(temp.pos) + "], 0\n";
                }
                else /*if(regex_match(declaration, assign_reg_for_declaration))*/{
                    smatch s_var;
                    regex assignable_left_reg("[A-Za-z_]\\w*(?=\\s*=)"); // 可赋值左值
                    regex_search(declaration, s_var, assignable_left_reg);
                    string var_name = s_var.str();

                    var temp;
                    temp.name = var_name;
                    temp.pos = var_pos;
                    var_pos -= 4;
                    current_func->local_var.emplace_back(temp);
                    // mips += " mov DWORD PTR [ebp" + to_string(temp.pos) + "], 0\n";

                    // regex space("=");
                    // declaration = regex_replace(declaration, assignable_left_reg, ""); // 删除左值
                    // declaration = regex_replace(declaration, space, ""); // 只保留表达式
                    declaration = declaration.substr(1+var_name.length(), declaration.length());
            
                    // 在变量表中寻找
                    for(auto &var: current_func->local_var){
                        if(var.name == var_name){
                            mips += expressionHandler(declaration);
                            mips += " mov DWORD PTR [ebp" + to_string(var.pos) + "], eax\n";
                            mips += "\n";
                            break;
                        }
                    }
                }
                declare_match++;
            }
        }
        // 赋值语句
        else if(regex_match(token, assignment_reg)){
            mips += "/* assignment: " + token + " */\n";
            mips += assignment(token);
        }
        // return 
        else if(regex_match(token, return_reg)){
            mips += "/* return " + token + " */\n";
            smatch var_match;
            regex kw("return\\s*");
            regex space(" ");
            token = regex_replace(token, kw, "");
            token = regex_replace(token, space, "");
            mips += expressionHandler(token);
            mips += " leave\n ret\n";
        }
        else if(regex_match(token, println_int_reg)){
            regex exp_reg_func("\\(.*\\)"); // 用于匹配括号内表达式
            smatch exp_match;
            regex space("\\s*");
            token = regex_replace(token, space, "");
            regex_search(token, exp_match, exp_reg_func);
            string expression = exp_match.str();
            mips += println_int(expression.substr(1, expression.length()-2));
        }
        else if (regex_match(token, continue_reg)){
            string jump = loop_stack.top();
            mips += " jmp .L_while_cond_" + jump + "\n";
        }
        else if (regex_match(token, break_reg)){
            string jump = loop_stack.top();
            mips += " jmp .L_while_end_" + jump + "\n";
        }
        else{  // 非赋值函数调用
            mips += func_call(token, tokens, i);
        }
        // else{
        //     mips +="\"" + token + "\" is not finished yet\n";
        // }
        // current_match++;
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

string expressionHandler(string expression){
    // 将赋值号右侧当作表达式统一处理
    regex _const("^[1-9]\\d*|0$"); // 非负整数
    regex _operator(">=|<=|==|!=|&&|(-`)|(\\|\\|)|[-~!\\+\\*/|\\^&%><]"); // 全部运算符
    regex _operator_ ("(-`)|!|~"); // 单目运算符
    regex _var(VAR);
    regex _left("\\(");
    regex _right("\\)");
    regex _lr("\\(|\\)");
    // 考虑所给示例中的实现
    // 操作数的栈在汇编语言中实现，操作符的栈在此处实现，但仍需记录操作数地址，对于立即数同变量处理
    // 总之还是需要实现两个栈
    VarStack vs;
    stack<string> optStack;

    int stack_top=0;
    string mips;
    mips += "/* expression handling: " + expression + " */\n";
    // 将表达式处理为token流
    list<string> token_list = expression2token(expression);
    for(auto token_iter=token_list.begin(); token_iter != token_list.end(); token_iter++){
        // cout << raw << endl;
        string token = *token_iter;
        if (regex_match(token, _const)){ // 处理常量
            // 加载立即数, 直接入栈
            mips += " mov eax, " + token + "\n";
            mips += vs.push("eax");
        }
        else if(regex_match(token, _var)){ // 处理变量
            // 变量提取地址，入栈
            string pos = findVarPos(token);
            mips += " mov eax, DWORD PTR [ebp" + pos + "]\n";
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
                    string op = optStack.top();
                    if (regex_match(op, _operator_)) {// 单目运算符
                        string op1 = "eax";
                        mips += vs.pop(op1);
                        mips += cal(op);
                        mips += vs.push("eax");
                        optStack.pop();
                    }
                    else {
                        string op2 = "ebx";
                        mips += vs.pop(op2);
                        string op1 = "eax";
                        mips += vs.pop(op1);
                        //将计算结果压入数字栈
                        mips += cal(op);
                        if (optStack.top() == "%")
                            mips += vs.push("edx");
                        else
                            mips += vs.push("eax");
                        //符号出栈
                        optStack.pop();
                    }
                }
                optStack.push(token);//最后把当前符号入栈
            }
        }
        else if (regex_match(token, _lr)){ // 匹配到单括号
            if(regex_match(token, _left))
                optStack.push(token);
            else if(regex_match(token, _right)){
                //遇到右括号')'依次弹出符号栈内运算符，和数字栈内的两个运算符，做计算，直到遇到左括号'('为止
                while(!regex_match(optStack.top(), _left)){
                    string op = optStack.top();
                    if (regex_match(op, _operator_)) // 单目运算符
                    {
                        string op1 = "eax";
                        mips += vs.pop(op1);
                        mips += cal(op);
                        mips += vs.push("eax");
                        optStack.pop();
                    }
                    else {
                        string op2 = "ebx";
                        mips += vs.pop(op2);
                        string op1 = "eax";
                        mips += vs.pop(op1);
                        //将计算结果压入数字栈
                        mips += cal(op);
                        if (optStack.top() == "%")
                            mips += vs.push("edx");
                        else
                            mips += vs.push("eax");
                        //符号出栈
                        optStack.pop();
                    }
                }
                optStack.pop();
            }
        }
        else { // 匹配到函数  函数调用: 返回值入栈
            vector<string> vt;
            mips += func_call(token, vt, if_count);
            mips += vs.push("eax");
            mips += "/* end call */\n";
        }
    }
    // 表达式处理完，保存
    // 将符号栈所有运算符出栈，与数字栈剩余数字做运算
    while(!optStack.empty()){
        string op = optStack.top();
        if (regex_match(op, _operator_)) // 单目运算符
        {
            string op1 = "eax";
            mips += vs.pop(op1);
            mips += cal(op);
            mips += vs.push("eax");
            optStack.pop();
        }
        else {
            string op2 = "ebx";
            mips += vs.pop(op2);
            string op1 = "eax";
            mips += vs.pop(op1);
            //将计算结果压入数字栈
            mips += cal(op);
            if (optStack.top() == "%")
                mips += vs.push("edx");
            else
                mips += vs.push("eax");
            //符号出栈
            optStack.pop();
        }
    }
    // 将运算结果放入eax
    mips += vs.pop("eax");
    // mips += "; expression handle ends!\n";
    return mips;// +"\n\"" + raw + "\" length: " + to_string(expression.size());
}

string findVarPos(string name){
    for(auto &var: current_func->local_var){
        if(var.name == name){
            string pos;
            pos = to_string(var.pos);
            return pos;
        }
    }
    string error;
    error = "var " + name + " not found!";
    return error;
}

int priority(string s){
    // 
    if(s == "+" || s=="-")
        return 8;
    else if (s=="*"||s=="/"||s=="%")
        return 9;
    else if (s=="|")
        return 3;
    else if (s=="^")
        return 4;
    else if (s=="&")
        return 5;
    else if (s=="!="||s=="==")
        return 6;
    else if (s==">"||s==">="||s=="<"||s=="<=")
        return 7;
    else if (s=="||")
        return 1;
    else if (s=="&&")
        return 2;
    else if (s=="!"||s=="-`"||s=="~")
        return 10;
        //设括号优先级为0
    else return 0;
}

// map<string, int> priority = {
//         {"+", 1},
//         {"-", 1},
//         {"*", 2},
//         {"/", 2}
//     };

//运算符
string cal(string op){
    string mips;
    if (op == "+") {
        mips = " add eax, ebx\n\n";
    }
    else if (op == "-") {
        mips = " sub eax, ebx\n\n";
    }
    else if (op == "*") {
        mips = " imul eax, ebx\n\n";
    }
    else if (op == "/") {
        mips = " cdq\n idiv eax, ebx\n\n";
    }
    else if (op == "%") {
        mips = " cdq\n idiv eax, ebx\n\n";
    }
    else if (op == "<") {
        mips = " cmp eax, ebx\n setl al\n movzx eax, al\n\n";
    }
    else if (op == "<=") {
        mips = " cmp eax, ebx\n setle al\n movzx eax, al\n\n";
    }
    else if (op == ">") {
        mips = " cmp eax, ebx\n setg al\n movzx eax, al\n\n";
    }
    else if (op == ">=") {
        mips = " cmp eax, ebx\n setge al\n movzx eax, al\n\n";
    }
    else if (op == "==") {
        mips = " cmp eax, ebx\n sete al\n movzx eax, al\n\n";
    }
    else if (op == "!=") {
        mips = " cmp eax, ebx\n setne al\n movzx eax, al\n\n";
    }
    else if (op == "&") {
        mips = " and eax, ebx\n\n";
    }
    else if (op == "|") {
        mips = " or eax, ebx\n\n";
    }
    else if (op == "^") {
        mips = " xor eax, ebx\n\n";
    }
    else if (op == "!"){
        mips = " cmp eax, 0\n sete al\n movzx eax, al\n";
    }
    else if (op == "~"){
        mips = " not eax\n";
    }
    else if (op == "-`"){
        mips = " neg eax\n";
    }
    else if (op == "&&") {
        mips = " and eax, ebx\n setne al\n movzx eax, al\n";
    }
    else if (op == "||") {
        mips = " or eax, ebx\n setne al\n movzx eax, al\n";
    }
    return mips;
}

string println_int(string expression){
    string instruct = expressionHandler(expression);
    instruct += " push eax\n";
    instruct += " push offset format_str\n";
    instruct += " call printf\n";
    instruct += " add esp, 8\n\n";
    return instruct;
}

list<string> expression2token(string expression){
    list<string> token;
    // 匹配 符号 变量或数字或函数
    regex _just_put_them_all_together("(>=)|(<=)|(==)|(!=)|(&&)|(\\|\\|)|>|<|([-~\\+\\*/|\\^&%\\(\\),!])|([A-Za-z_]\\w*|[1-9]\\d*|0)");
    regex operator_reg("(>=)|(<=)|(==)|(!=)|(&&)|(\\|\\|)|>|<|[-\\+\\*/|\\^&%~!]");
    regex var_reg(VAR);
    regex space(";");

    expression = regex_replace(expression, space, "");

    sregex_iterator all_match(expression.begin(), expression.end(), _just_put_them_all_together);
    sregex_iterator last_match;

    sregex_iterator last_itr = all_match;
    int count = 0;
    while(all_match != last_match){
        smatch all_result = *all_match;
        sregex_iterator crt = all_match;
        smatch next = *(++crt);
        smatch last = *last_itr;
        string last_str = last.str();
        string crt_str = all_result.str();
        // // 识别单目运算符 - 前一个是运算符或开头
        // bool a = all_result.str()=="-";
        // bool b = regex_match(last.str(), operator_reg);
        // bool c = last_itr==all_match;
        if ((all_result.str()=="-")&&(regex_match(last.str(), operator_reg)||last_itr==all_match)) {
            token.emplace_back("-`");
            all_match++;
            count++;
            continue;
        }
        // 处理函数 变量名+括号
        if (regex_match(all_result.str(), var_reg) && next.str()=="("){
            sregex_iterator _begin = all_match;
            int stack = 0;
            string func;
            for (; all_match != last_match; all_match++) {
                smatch current = *all_match;
                string current_str = current.str();
                func += current_str;
                if (current_str == "(") {
                    stack++;
                } else if (current_str == ")") {
                    if (stack) {
                        stack--;
                        if (!stack) {
                            token.emplace_back(func);
                            all_match++;
                            count++;
                            break;
                        }
                    }
                }
            }
            continue;
        }
        string segment = all_result.str();
        token.emplace_back(segment);
        all_match++;
        if(count > 0)
            last_itr++; // 第一轮不迭代，落后一轮
        count++;
    }
    return token;
}

string assignment(string token){
    string mips;

    smatch s_var;
    smatch s_expression;
    regex assignable_left_reg("[A-Za-z_]\\w*(?=\\s*=)"); // 可赋值左值
    // regex expression_reg("[^;]*"); // 匹配表达式

    regex_search(token, s_var, assignable_left_reg);
    string var_name = s_var.str();

    regex space("\\s*|;");
    // token = regex_replace(token, assignable_left_reg, ""); // 删除左值
    token = regex_replace(token, space, "");
    // string expression = token.substr(1, token.length()-2); // 掐头 去尾 截去= ;
    // regex_search(token, s_expression, expression_reg);
    // string expression = s_expression.str();
    token = token.substr(1+var_name.length(), token.length());
    
    // 在变量表中寻找
    for(auto &var: current_func->local_var){
        if(var.name == var_name){
            mips += expressionHandler(token);
            mips += " mov DWORD PTR [ebp" + to_string(var.pos) + "], eax\n";
            break;
        }
    }
    return mips;
}

string func_call(string func_expression, vector<string> tokens, int& i)
{
    string mips;
    mips += "/* call " + func_expression + " */\n";
    regex name_reg(VAR);
    regex space(" |;");
    regex else_reg("\\s*else\\s*");

    string func = regex_replace(func_expression, space, "");
    smatch s_func;
    regex_search(func, s_func, name_reg);
    string func_name = s_func.str();

    if (func_name == "if"){
        i++;
        if_count++;
        string exp = func.substr(func_name.length()+1, func.length() - func_name.length()-2);
        string jump = to_string(if_count);
        mips += expressionHandler(exp);
        mips += " cmp eax, 0\n";
        if (regex_match(tokens[i+1], else_reg)){
            mips += " je .L_else_"+ jump + "\n";
            mips += CtoMips(tokens[i].substr(1, tokens[i].length()-2));
            mips += " jmp .L_if_end_" + jump + "\n";
            mips += ".L_else_" + jump + ": \n";
            string else_token = tokens[i+2];
            mips += CtoMips(else_token.substr(1, else_token.length()-2));
            mips += ".L_if_end_" + jump + ": \n";
            i += 2;
        }
        else {
            mips += " je .L_if_end_" + jump + "\n";
            mips += CtoMips(tokens[i].substr(1, tokens[i].length()-2));
            mips += ".L_if_end_" + jump + ": \n";
        }
    }
    else if (func_name == "while"){
        i++;
        if_count++;
        string jump = to_string(if_count);
        string exp = func.substr(func_name.length()+1, func.length() - func_name.length()-2);
        loop_stack.push(jump);
        mips += ".L_while_cond_" + jump + ": \n";
        mips += expressionHandler(exp);
        mips += " cmp eax, 0\n";
        mips += " je .L_while_end_" + jump + "\n";
        mips += CtoMips(tokens[i].substr(1, tokens[i].length()-2));
        mips += " jmp .L_while_cond_" + jump + "\n";
        mips += ".L_while_end_" + jump + ": \n";
        loop_stack.pop();
    }
    else {
        string exp_list = func.substr(func_name.length()+1, func.length() - func_name.length()-2);
        vector<string> exps;
        int el_len = exp_list.length();
        int count = 0;
        string exp="";
        // 只有遇到括号外面的逗号才保存表达式
        for (int i = 0; i < el_len; i++) {
            exp += exp_list[i];
            if (exp_list[i]=='(')
                count++;
            if (exp_list[i]==')')
                count--;
            if (count==0 && exp_list[i]==',') {
                // 去掉在末尾的 逗号
                exps.emplace_back(exp.substr(0, exp.length()-1));
                exp = "";
            }
        }
        exps.emplace_back(exp);
        for (int i = exps.size()-1; i >= 0; i--) {
            mips += expressionHandler(exps[i]);
            mips += " push eax\n";
        }

        mips += " call " + func_name + "\n";
        mips += " add esp, " + to_string(4*exps.size()) + "\n";
    }
    return mips;
}

string func_declare(string func_expression)
{
    func local_func;
    string mips;
    mips += "/* declare: " + func_expression + " */\n";

    regex name_reg(VAR);
    regex keyword_reg(KEYWORD);
    regex type_reg("\\s*(int|void)\\s+");
    regex type_name("\\s*(int|void)\\s+[A-Za-z_]\\w*");
    regex space(" ");
    // 函数类型
    smatch s_func;
    regex_search(func_expression, s_func, type_reg);
    string func_type = s_func.str();
    func_type = regex_replace(func_type, space, "");
    regex_search(func_expression, s_func, type_name);
    string func_type_name = s_func.str();
    func_type_name = regex_replace(func_type_name, space, "");
    string func_name = func_type_name.substr(func_type.length(), func_type_name.length());

    string func = regex_replace(func_expression, type_reg, "");
    func = regex_replace(func, space, "");
    

    sregex_iterator all_match(func.begin(), func.end(), name_reg);
    sregex_iterator last_match;

    // string func_name = all_match->str();
    all_match++;

    mips += func_name + ":\n";
    mips += " push ebp\n";
    mips += " mov ebp, esp\n";
    mips += " sub esp, 0x100\n\n";

    local_func.func_name = func_name;
    local_func.func_type = func_type;
    int local_var_pos = -4;
    int local_param_pos = 8;

    while(all_match != last_match){
        var var0;
        smatch all_result = *all_match;
        string var_name = all_result.str();
        var0.name = var_name;
        var0.pos = local_var_pos;
        local_func.local_var.emplace_back(var0);
        mips += "/* local parameter: " + var_name + " */\n";
        mips += " mov eax, DWORD PTR [ebp+" + to_string(local_param_pos) + "]\n";
        mips += " mov DWORD PTR [ebp" + to_string(var0.pos) + "]" + ", eax\n";
        
        local_var_pos -= 4;
        local_param_pos += 4;
        all_match++;
    }

    global_func.emplace_back(local_func);

    var_pos = local_var_pos;
    current_func = &global_func.back();
    
    return mips;
}

vector<string> scanner(string code)
{
    /*
    划分代码段、单句、函数定义
    */
    vector<string> exps;

    int code_len = code.length();
    int count = 0;
    bool is_inblock = false;

    string sentence="";
    for (int i = 0; i < code_len; i++) {
        sentence += code[i];
        if (code[i]=='{' && !is_inblock){ // 不在代码段内并且 遇到{
            exps.emplace_back(sentence.substr(0, sentence.length()-1));
            sentence = "{";
            count++;
            is_inblock = true;
        }
        else if (code[i]=='{' && is_inblock){
            count++;
        }
        else if (code[i]=='}'){
            count--;
            if (count==0) {
            exps.emplace_back(sentence);
            sentence = "";
            is_inblock = false;
            }
        }
        else if (code[i] == ';' && !is_inblock){ //不在代码段内，这里表明已经进入代码段内部，不再整体处理
            exps.emplace_back(sentence);
            sentence = "";
        }
    }        
    return exps;
}