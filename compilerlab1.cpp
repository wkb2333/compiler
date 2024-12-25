#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <stack>
// #include <strstream>
#include <regex>

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

    string push(string str){
        // string s = "sw $";
        // s += str + ", 0($sp)\n" + "addiu $sp, $sp, -4\n";
        string s = "push ";
        s += str + "\n";
        sp -= 4;
        return s;
    }
    string pop(string str){
        // pop需要给出存放的寄存器
        string s = "pop "; //"lw $";
        s += str + "\n"; // + ", 4($sp)\n" + "addiu $sp, $sp, 4\n";
        sp += 4;
        return s;
    }
};

string CtoMips(list<string> sentence);
// string int2str(int digital);
string findVarPos(string name);
string cal(string opt);
int level(string s);
string expressionHandler(var& var, list<string> expression);

list<var> vars;
int var_pos = -4;

int main(int argc, char * argv[]) {
    string path = argv[1];
    // string path = "input.txt";
    ifstream infile; 

    list<string> tokens;
    string token;
    
    infile.open(path, ios::in); 
    // list<string>::iterator nextBegin = tokens.begin();
    while(infile >> token){
        tokens.emplace_back(token);
        if(tokens.back() == ";"){
            list<string> sentence(tokens.begin(), --tokens.end());
            tokens.erase(tokens.begin(), tokens.end());
            string mipsCode = CtoMips(sentence);
            cout << mipsCode << endl;
        }
        // cout << tokens.back() << endl;
    }
    // for(auto &var: vars)
    //     cout << var.name << var.pos << var.value << endl;
    return 0;
}

string CtoMips(list<string> sentence)
{
    regex var_reg("[A-Za-z]"); // 匹配单个字母变量
    list<string>::iterator it = sentence.begin();
    while(it != sentence.end()){
        // 变量声明 -> 开辟存储空间
        // 这里只处理特例，不再预留扩展空间
        // int 起始，并且第二个单词为单字母变量名
        if((*it) == "int"){
            var temp;
            it++;
            if(regex_match(*it, var_reg))
            temp.name = *it;
            temp.pos = var_pos;
            var_pos -= 4;
            vars.emplace_back(temp);
            return "mov DWORD PTR [ebp" + to_string(temp.pos) + "], 0\n";
        }

        // 赋值语句，第二个单词为 = 
        if(*++it == "="){
            string var_name = *--it;
            string mips;
            // 在变量表中寻找
            for(auto &var: vars){
                if(var.name == var_name){
                    it++; it++; // 跳到表达式
                    list<string> expression(it, sentence.end());
                    mips += expressionHandler(var, expression);
                    break;
                }
            }
            return mips;
        }
        it--;

        // 此处仅处理返回单个变量
        if(*it == "return"){
            it++;
            string var_name = *it;
            string mips;
            string pos = findVarPos(var_name);
            mips = "mov eax, DWORD PTR [ebp" + pos + "]\n";
            return mips;
        }
        it++;
    }
    string raw;
    for(auto word: sentence)
        raw += word + " ";
    return "\"" + raw + "\" is not finished yet\n";
}

// string int2str(int digital){
//     strstream ss;
//     string s;
//     ss << digital;
//     ss >> s;
//     return s;
// }

string expressionHandler(var& var, list<string> expression){
    // 将赋值号右侧当作表达式统一处理
    regex _digital("0|[1-9][0-9]*");
    regex _operator("[-+*/]");
    regex _var("[A-Za-z]");
    regex _left("\\(");
    regex _right("\\)");
    // 考虑所给示例中的实现
    // 操作数的栈在汇编语言中实现，操作符的栈在此处实现，但仍需记录操作数地址，对于立即数同变量处理
    // 总之还是需要实现两个栈
    VarStack vs;
    stack<string> optStack;

    int stack_top=0;
    
    string mips;
    string raw;
    for(auto it=expression.begin(); it != expression.end(); it++){
        raw += *it;
        // cout << raw << endl;
        if (regex_match(*it, _digital)){ // 处理常量
            // 加载立即数, 直接入栈
            mips += "mov eax, " + *it + "\n";
            mips += vs.push("eax");
        }
        else if(regex_match(*it, _var)){ // 处理变量
            // 变量提取地址，入栈
            string pos = findVarPos(*it);
            mips += "mov eax, DWORD PTR [ebp" + pos + "]\n";
            mips += vs.push("eax");
        }
        else if(regex_match(*it, _operator)){ // 处理运算符
            //如果当前运算符的优先级>栈顶符号优先级，则直接进栈
            if(optStack.empty()||level(*it)> level(optStack.top()))
                optStack.push(*it);
            //如果当前运算符的优先级<=栈顶符号的优先级
            else{
                //依次弹出优先级高于或等于当前符号的所有符号，并同时弹出数字栈中的两个数字作为操作数。注意先后顺序。
                while(!optStack.empty()&&level(optStack.top())>= level(*it)){
                    string op2 = "ebx";
                    mips += vs.pop(op2);
                    string op1 = "eax";
                    mips += vs.pop(op1);
                    //将计算结果压入数字栈
                    mips += cal(optStack.top());
                    mips += vs.push("eax");
                    //符号出栈
                    optStack.pop();
                }
                optStack.push(*it);//最后把当前符号入栈
            }
        }
        else{
            if(regex_match(*it, _left))
                optStack.push(*it);
            else if(regex_match(*it, _right)){
                //遇到右括号')'依次弹出符号栈内运算符，和数字栈内的两个运算符，做计算，直到遇到左括号'('为止
                while(!regex_match(optStack.top(), _left)){
                    string op2 = "ebx";
                    mips += vs.pop(op2);
                    string op1 = "eax";
                    mips += vs.pop(op1);
                    //将计算结果压入数字栈
                    mips += cal(optStack.top());
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

//规定优先级
int level(string s){
    if(s == "+" || s=="-")
        return 1;
    else if (s=="*"||s=="/")
        return 2;
        //设括号优先级为0
    else return 0;
}

//四则运算
string cal(string _operator){
    char opt = _operator[0];
    switch (opt) {
        case '+': return "add eax, ebx\n\n";break;
        case '-': return "sub eax, ebx\n\n";break;
        case '*': return "imul eax, ebx\n\n";break;
        case '/': return "cdq\nidiv ebx\n\n";break;
    }
}
