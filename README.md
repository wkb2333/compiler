# compiler
* c语言简单编译器，将c转换为x86。出自北理工计算机大三编译原理实验   
* 印象中我当时做这个实验和课程内容没太大关系，即理论和实践没联系起来，基本上纯靠编程能力。  
* 开始时实验任务说可以选择将c编译为x86或者mips，但是最终只能x86，所以代码里部分函数名称比较迷惑。不过两者基本没有区别。

* 起初打算用正则表达式识别所有内容，但是有的括号嵌套无法实现（可能是我没写出正确的RE），因此在之后的lab3、4中又实现了专门处理括号嵌套的函数。

## input.c 为源程序，x86.s 为目标程序，为了方便debug，在x86.s中同时生成了许多注释。自行参考
