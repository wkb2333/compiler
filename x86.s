.intel_syntax noprefix
.global gcd
.global main
.extern printf
.data
format_str:
 .asciz "%d\n"
 .text

/* declare: int gcd(int a, int b)  */
gcd:
 push ebp
 mov ebp, esp
 sub esp, 0x100

/* local parameter: a */
 mov eax, DWORD PTR [ebp+8]
 mov DWORD PTR [ebp-4], eax
/* local parameter: b */
 mov eax, DWORD PTR [ebp+12]
 mov DWORD PTR [ebp-8], eax
/* call  if (b == 0)  */
/* expression handling: b==0 */
 mov eax, DWORD PTR [ebp-8]
 push eax
 mov eax, 0
 push eax
 pop ebx
 pop eax
 cmp eax, ebx
 sete al
 movzx eax, al

 push eax
 pop eax
 cmp eax, 0
 je .L_else_1
/* return  return a; */
/* expression handling: a; */
 mov eax, DWORD PTR [ebp-4]
 push eax
 pop eax
 leave
 ret
 jmp .L_if_end_1
.L_else_1: 
/* return  return gcd(b, a % b); */
/* expression handling: gcd(b,a%b); */
/* call gcd(b,a%b) */
/* expression handling: a%b */
 mov eax, DWORD PTR [ebp-4]
 push eax
 mov eax, DWORD PTR [ebp-8]
 push eax
 pop ebx
 pop eax
 cdq
 idiv eax, ebx

 push edx
 pop eax
 push eax
/* expression handling: b */
 mov eax, DWORD PTR [ebp-8]
 push eax
 pop eax
 push eax
 call gcd
 add esp, 8
 push eax
/* end call */
 pop eax
 leave
 ret
.L_if_end_1: 
/* declare:  int main()  */
main:
 push ebp
 mov ebp, esp
 sub esp, 0x100

/* var declaration: num1=150 */
/* expression handling: 150 */
 mov eax, 150
 push eax
 pop eax
 mov DWORD PTR [ebp-4], eax

/* var declaration: num2=120 */
/* expression handling: 120 */
 mov eax, 120
 push eax
 pop eax
 mov DWORD PTR [ebp-8], eax

/* call  int result = gcd(num1, num2); */
/* expression handling: gcd(num1,num2 */
 pop eax
 push eax
 call intresult
 add esp, 4
/* expression handling: result */
 mov eax, DWORD PTR [ebpvar result not found!]
 push eax
 pop eax
 push eax
 push offset format_str
 call printf
 add esp, 8

/* return  return 0; */
/* expression handling: 0; */
 mov eax, 0
 push eax
 pop eax
 leave
 ret

