#编译原理PL0报告    

---
##pl0.h的相关改动

###宏定义的常量上
```c
#define NRW  16 // number of reserved words
#define NSYM 14 // maximum number of symbols in array ssym and csym
#define ENDCX  499//作为代码的结束部分
```
原因：
+ 在保留字中未保留关键字array
+ 在ssym和csym特殊符号中未保留/和&，因为它们都可以在getsym()中单独判断
###枚举类型symtype
-增加如下的symtype
```c
    SYM_NOT,//!
	SYM_LEFTSPAREN,//[
	SYM_RIGHTSPAREN,//]
	SYM_ELSE,
	SYM_ELIF,
	SYM_EXIT,
	SYM_RETURN,
	SYM_FOR,
	SYM_AND,//&&
	SYM_OR,//||
	SYM_INC,//++
	SYM_DEC,//--
	SYM_ANDBIT,//&
	SYM_ORBIT,//|
	SYM_XOR,//&
	SYM_MOD,//%
```
###标识符类型idtype
-增加两个标识符类型，分别对应函数返回和数组
```c
enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE, ID_RETURN, ID_ARRAY
};
```
###机器的操作指令
-添加一些操作指令，实现更灵活的跳转
```c
enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JZ, RET, LODARR, STOARR, JNZ, JE, JNE, JG, JGE, JL, JLE, BAC, JZS, JNZS
};
```
-其中各个操作的指令含义如下
| F      | L    | A         | 含义               |
| ------ | ---- | --------- | ---------------- |
| INT    | ——   | 常量        | 在数据栈中分配空间        |
| LIT    | ——   | 常量        | 常数置于栈顶           |
| RET    | ——   | -（参数个数）-1 | 返回上一层程序          |
| LOD    | 层次差  | 数据地址      | 将变量值置于栈顶         |
| STO    | 层次差  | 数据地址      | 将栈顶的值置于变量        |
| LODARR | 层次差  | 数组首地址     | 待存储变量在栈顶，偏移量在次栈顶 |
| STOARR | 层次差  | 数组首地址     | 待存储变量在栈顶，偏移量在次栈顶 |
| CAL    | 层次差  | 程序地址      | 层次调用指令           |
| OPR    | ——   | 运算类别      | 一组算术或者逻辑操作指令     |
| JMP    | ——   | 程序地址      | 无条件跳转            |
| JZ     | ——   | 程序地址      | 栈顶为0跳转           |
| JZS    | ——   | 程序地址      | 栈顶为0跳转，不退栈       |
| JNZ    | ——   | 程序地址      | 栈顶不为0跳转          |
| JNZS   | ——   | 程序地址      | 栈顶不为0跳转，不退栈      |
| JE     | ——   | 程序地址      | 栈顶和次栈顶相等跳转       |
| JNE    | ——   | 程序地址      | 栈顶和次栈顶不相等跳转      |
| JG     | ——   | 程序地址      | 栈顶小于次栈顶跳转        |
| JGE    | ——   | 程序地址      | 小于等于             |
| JL     | ——   | 程序地址      | 大于               |
| JLE    | ——   | 程序地址      | 大于等于             |
| BAC    | ——   | 常量        | 栈顶退栈常量个数值        |
-对于OPR操作，添加了一组指令，对应我们增加的运算
```c
enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ, OPR_AND, OPR_OR, OPR_NOT,   //9.30号添加了与 或 非操作
	OPR_MOD, OPR_ANDBIT, OPR_ORBIT ,OPR_XOR   //10.9添加取模操作,按位与，按位或，异或
};
```
###error部分
增加了一些出错警告，在此不做详述
###对符号表的修改
```c
typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int arrayAdd;
	int  value;
} comtab;

comtab table[TXMAX];

typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	int   arrayAdd;
	short level;
	short address;
} mask;
```
增加arrayAdd属性，方便后续实现数组操作
###定义新变量
```c
int presym;         //上个symtype值
int readDim = 0;    //当前已经读入的数组的维数
int arrayDim[1000]; //用来存储数组的维数和相应维数的大小
int adx = 0;		//数组的个数
int temp_adx = 0;
int pcount = 0;
```
##pl0.c的改动
###词法部分
####正规式
```c
/*正规式*/
D   ->    [0-9]
L   ->    [a-zA-Z]

SYM_NULL        -> ϵ
SYM_IDENTIFIER  -> L(D|L)*//以字母开头的字母数字串
SYM_NUMBER      -> DD*
SYM_PLUS        -> +
SYM_MINUS       -> -
SYM_TIMES       -> *
SYM_SLASH       -> /
SYM_ODD         -> odd
SYM_EQU         -> =
SYM_NEQ         -> <>
SYM_LES         -> <
SYM_LEQ         -> <=
SYM_GTR         -> >
SYM_GEQ         -> >=
SYM_LPAREN      -> (
SYM_RPAREN      -> )
SYM_COMMA       -> ,
SYM_SEMICOLON   -> ;
SYM_PERIOD      -> .
SYM_BECOMES     -> :=
SYM_BEGIN       -> begin
SYM_END         -> end
SYM_IF          -> if
SYM_THEN        -> then
SYM_WHILE       -> while
SYM_DO          -> do
SYM_CALL        -> call
SYM_CONST       -> const
SYM_VAR         -> var
SYM_PROCEDURE   ->procedure
SYM_NOT         -> !
SYM_LEFTSPAREN  -> [
SYM_RIGHTSPAREN ->  ]
SYM_ELSE        -> else
SYM_ELIF        -> elif
SYM_EXIT        -> exit
SYM_RETURN      -> return
SYM_FOR         -> for
SYM_AND         -> &&
SYM_OR          -> ||
SYM_INC         -> ++
SYM_DEC         -> --
SYM_ANDBIT      -> &
SYM_ORBIT       -> |
SYM_XOR         -> &
SYM_MOD         -> %
```
相关函数：getch() getsym()
```c
/*从源文件中读取一行到行缓冲区中,将换行作为空格存储，每次从行缓冲区读取一个单词*/
void getch(void);
/*词法分析相关程序，跳过空格和制表符，分析单词，我们主要的修改部分*/
void getsym(void);
```
需要增加相应的单词symtype判断，具体的添加过程如下：
####注释
在条件判断处添加代码如下
```c
else if (ch == '/')		//判断注释
{
	getch();
	if (ch == '/')//是‘//’跳过当前行 
	{
		cc = ll;
		getch();
	}
	else if (ch == '*') //是‘/*’
	{
		getch();
		while (1) 
		{
			while (ch != '*')
			{
				getch();
			}
			getch();
			if (ch == '/')
			{
				getch();
				getsym();
				break;  //遇到‘*/’结束
			}
		}
	}
	else {//如果不是/*，那么就是除号
		sym = SYM_SLASH;
	}
}
```
####保留字
保留字直接在word和wsym数组中进行添加即可
```c
/*reserved word*/
char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while",
	"for","return","elif","exit","else"
};

int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,
	SYM_FOR,SYM_RETURN,SYM_ELIF,SYM_EXIT,SYM_ELSE
};
```
####与或
同注释一样，在getsym()条件判断处添加
```c
else if (ch == '|') //逻辑运算符的判断
{
	getch();
	if (ch == '|') 
	{
		sym = SYM_OR;
		getch();
	}
	else
		sym = SYM_ORBIT;
}
else if (ch == '&')
{
	getch();
	if (ch == '&')
	{
		sym = SYM_AND;
		getch();
	}
	else
		sym = SYM_ANDBIT;
}
```
####特殊符号的添加
直接在csym和ssym数组中进行添加即可
```c
int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA,
	SYM_PERIOD, SYM_SEMICOLON,SYM_NOT,SYM_LEFTSPAREN,
	SYM_RIGHTSPAREN,SYM_XOR,SYM_MOD
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', 
	'(', ')', '=', ',', 
	'.', ';','!','[',
	']','^','%'
};
```
####++和--的判断
这个比较特殊，因为我们已经在特殊符号中判断过+和-了，因此我们只需要在其后判断下个符号是不是+/-就行，如下
```c
if (++i)
	{
		sym = ssym[i];
		getch();
		if (sym == SYM_PLUS && ch == '+')
		{
			sym = SYM_INC;
			getch();
		}
		else if (sym == SYM_MINUS&&ch == '-')
		{
			sym = SYM_DEC;
			getch();
		}
	}
```
至此，词法部分添加结束
###对语法框架的修改
我们为了表示方便，对程序框架做了一些修改：
每一条语句后都有分号
begin-end后没有分号，在这里可以将begin-end当作C中的{}
首先，我们改动begin-end,主要目标是移除分号的判断
```c
else if (sym == SYM_BEGIN)
{ // block
	getsym();
	/*此时的fsys集合已经有了分号和end，这是无意义的*/
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);
	while (inset(sym, statbegsys))
	{
		statement(set);
	} // while
	destroyset(set1);
	destroyset(set);
	if (sym == SYM_END)
	{
		getsym();
	}
	else
	{
		error(17); //  'end' expected.
	}
}
```
然后我们在相应的语句后添加分号判别符
```c
if (sym == SYM_SEMICOLON)
{
	getsym();
}
/*为了for中的右括号*/
else if (sym != SYM_RPAREN)
{
	error(22);
}
```

###扩展PL0中的条件
####设计重要原则
在这里我们有一个重要设计原则，即保证一致性，我们在设计语法单元函数的时候需要遵循以下原则：
进入每一个语法单位处理这个程序之前，其第一个单词已经读出，退出时，应该读入下一个语法单元的第一个单词。
####逻辑算符的实现
#####逻辑算符&&和||的添加和实现
相关函数：condition(),conditions_or(),conditions_and()，interpret（）
考虑到&&和||的优先级高于其余逻辑运算符，比如=，<>，>这些，因此重新设计函数调用关系，首先添加如下函数：
```c
void conditions_and(symset fsys)
{
	symset set;
	/*为了错误恢复，此时&&是合法的后继符号集合*/
	set = uniteset(fsys, createset(SYM_AND, SYM_NULL));
	condition(set);
	while (sym == SYM_AND)
	{
		getsym();
		condition(set);
		gen(OPR, 0, OPR_AND);
	}
	destroyset(set);
}
void conditions_or(symset fsys)
{
	symset set;
	/*为了错误恢复，此时||是合法的后继符号集合*/
	set = uniteset(fsys, createset(SYM_OR, SYM_NULL));
	conditions_and(set);
	while (sym == SYM_OR)
	{
		getsym();
		conditions_and(set);
		gen(OPR, 0, OPR_OR);
	}
	destroyset(set);
}
```
原本调用condition的部分现在调用conditions_or，然后conditions_or调用conditions_and,conditions_and调用condition，体现出了优先级顺序，具体的语法图请看语法图相应部分，至此，语法相应部分设计完毕。
除此之外，还注意到应实现对应的OPR_OR和OPR_AND，具体实现过程如下：
在interpret（）中OPR对应switch-case语句中添加如下部分：
```c
case OPR_AND:
	top--;
	stack[top] = stack[top] && stack[top + 1];
	break;
case OPR_OR:
	top--;
	stack[top] = stack[top] || stack[top + 1];
	break;
```
相应的翻译部分也设计完毕：
#####C运算符&，|，%，^的实现
通过查询C的运算符优先级表格，发现按位或的优先级最高，其次是按位异或，再其次是按位与。因此我们依据这种优先级顺序添加如下函数：
```c
void expression_andbit(symset fsys)//&
{
	symset set;
    /*为了错误恢复，此时&是合法的后继符号集合*/
	set = uniteset(fsys, createset(SYM_ANDBIT, SYM_NULL));
	expression(set);
	while (sym == SYM_ANDBIT)
	{
		getsym();
		expression(set);
		gen(OPR, 0, OPR_ANDBIT);
	}
	destroyset(set);
}
void expression_xorbit(symset fsys)//^
{
	symset set;
	/*为了错误恢复，此时^是合法的后继符号集合*/
	set = uniteset(fsys, createset(SYM_XOR, SYM_NULL));
	expression_andbit(set);
	while (sym == SYM_XOR)
	{
		getsym();
		expression_andbit(set);
		gen(OPR, 0, OPR_XOR);
	}
	destroyset(set);
}
void expression_orbit(symset fsys)//|
{
	symset set;
	/*为了错误恢复，此时|是合法的后继符号集合*/
	set = uniteset(fsys, createset(SYM_ORBIT, SYM_NULL));
	expression_xorbit(set);
	while (sym == SYM_ORBIT)
	{
		getsym();
		expression_xorbit(set);
		gen(OPR, 0, OPR_OR);
	}
	destroyset(set);
}
```
同理，原本调用expression的地方现在调用expression_or,expression_or调用expression_xor，expression_xor调用expression_and，最终expression_and调用expression，详细的语法图请看语法图相应部分。对于取模的处理我们有一些区别，因为取模的优先级和乘法除法一致，因此我们在trem函数中添加相应代码。
```c
set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_MOD ,SYM_NULL));
factor(set);
while (sym == SYM_TIMES || sym == SYM_SLASH || sym == SYM_MOD)
{
	mulop = sym;
	getsym();
	factor(set);
	if (mulop == SYM_TIMES)
	{
		gen(OPR, 0, OPR_MUL);
	}
	else if(mulop == SYM_SLASH)
	{
		gen(OPR, 0, OPR_DIV);
	}
	else//添加取模操作
	{
		gen(OPR, 0, OPR_MOD);
	}
} // while
destroyset(set);
```
然后我们实现相应的翻译部分
```c
case OPR_ANDBIT:
	top--;
	stack[top] = stack[top] & stack[top + 1];
	break;
case OPR_ORBIT:
	top--;
	stack[top] = stack[top] | stack[top + 1];
	break;
case OPR_XOR:
	top--;
	stack[top] = stack[top] ^ stack[top + 1];
	break;
case OPR_MOD:
	top--;
	stack[top] %=  stack[top + 1];
	break;
```
如上，这些运算符添加完毕
#####将PL0的条件泛化
在此处我们的处理是将单个的表达式也可以作为一个条件。
在condition处取消错误判断
```c
if (!inset(sym, relset))
{
	/*error(20);*/ // 删去这里的出错判断，令条件泛化为表达式
}
```
#####条件的短路计算

运算符 “||” 的短路

```C
void short_condition_or(symset fsys)
{
	int cxTemp[1000], cxTempCount;
	cxTempCount = 0;
	short_condition_and(uniteset(createset(SYM_OR, SYM_NULL), fsys));
	cxTemp[++cxTempCount] = cx;
	gen(JNZS, 0, 0);
	while (sym == SYM_OR)
	{
		getsym();
		gen(BAC, 0, 1);
		short_condition_and(uniteset(createset(SYM_OR, SYM_NULL), fsys));
		cxTemp[++cxTempCount] = cx;
		gen(JNZS, 0, 0);
	}
	int i;
	for (i = cxTempCount; i >= 1; i--)
	{
		code[cxTemp[i]].a = cx;
	}
}
```

`short_condition_or` 的结构是这样的(以下简称sc_or)：

```C
sc_and || sc_and || ... || sc_and
```

每一个`sc_and`都有一个truelist 和 falselist 。这个truelist就是当前`sc_or`的truelist，这个falselist就是下一个||符号开始的第一个指令地址。我们通过数组`cxTemp[]`保存每个sc_and的truelist，然后在最后读完的时候进行回填。

运算符"&&"的短路同理：

```C
void short_condition_and(symset fsys)
{
	int cxTemp[1000], cxTempCount;
	cxTempCount = 0;
	condition(uniteset(createset(SYM_AND, SYM_NULL), fsys));
	cxTemp[++cxTempCount] = cx;
	gen(JZS, 0, 0);
	while (sym == SYM_AND)
	{
		getsym();
		gen(BAC, 0, 1);
		condition(uniteset(createset(SYM_AND, SYM_NULL), fsys));
		cxTemp[++cxTempCount] = cx;
		gen(JZS, 0, 0);
	}
	int i;
	for (i = cxTempCount; i >= 1; i--)
	{
		code[cxTemp[i]].a = cx;
	}

}
```

`short_condition_and`的结构是这样的:

```C
condition && condition && ... && condition
```

这里的condition包括对expression的实现，所以基于类似(i+1)这样的“条件”也是合法的。每个condition都有自己的truelist和falselist。其中这里的falselist是当前sc_and的falselist。这里的truelist是下一个&&符号开始的第一条指令的地址。我们同样通过数组`cxTemp[]`保存每个condition的falselist，然后在最后读完的时候进行回填。

####添加数组

在前面已经说过，在pl0.h中我们定义了三个变量，这将会运用到我们接下来的数组操作中
```c
int readDim = 0;    //当前已经读入的数组的维数
int arrayDim[1000]; //用来存储数组的维数和相应维数的大小
int adx = 0;		//数组的个数
int temp_adx=0;     //永远指向数组的维数，方便声明
```
#####数组声明
在这里定义两个函数来进行数组的声明过程
```c
/*数组的登入过程
调用此函数会将符号表中原本的ID_VARIABLE替换为ID_ARRAY,并进行相应的初始化
*/
void enterArray()
{
	mask *mk;
	table[tx].kind = ID_ARRAY;//将当前变量的类型定义为数组
	mk = (mask *)&table[tx];
	mk->level = level;
	mk->address = dx-1;		//在enter符号表的时候dx已经++了
	mk->arrayAdd = ++adx;	//当前总数组的个数
	temp_adx = adx;         //永远指向当前数组的维数
	arrayDim[adx] = 0;		//数组的维数置0
}
/*计算数组的长度
调用此函数会分析[]的内容，即数组的维数部分，计算出数组所需要的大小，其中arrayDim[adx]存储数组的维数，arrayDim[adx+常量]存储数组常量维的大小，最后将它们相乘得出数组大小，然后dx+=数组大小-1，以此来分配空间
*/
void arrayDecl()
{
	if (presym == SYM_LEFTSPAREN)/*遇到左中括号*/
	{
		if (sym != SYM_NUMBER)
		{
			printf("expected number when declare array.\n");
			err++;
			return;
		}
		else 	// sym == SYM_LEFTSPAREN
		{
			arrayDim[temp_adx]++;	//数组的维数增加1
			adx++;
			arrayDim[adx] = num;	//存储数组对应维数的大小
			getsym();
		}
		if (sym != SYM_RIGHTSPAREN)
		{
			printf("expected rightsparen here while declare array at dim %d\n", arrayDim[temp_adx]);
			err++;
			return;
		}
		else
		{
			getsym();
			arrayDecl();
		}
	}
	else if (presym == SYM_RIGHTSPAREN)
	{
		if (sym == SYM_LEFTSPAREN)
		{
			getsym();
			arrayDecl();
		}
		else // ; or ,
		{
			int count = 1;
			int i = 1;
			for(i = 1; i <= arrayDim[temp_adx]; i++)
			{
				count *= arrayDim[temp_adx + i];
			}//计算数组所需的空间大小
			printf("the array count is %d", count);
			dx += count - 1;
			return;
		}
	}
	else
	{
		printf("expected '[' or ']'\n");
		err++;
		getsym();
		return;
	}
}
```
用原本的变量声明来调用上面两个函数：
vardeclaration(）
```c
void vardeclaration(void)
{
	if (sym == SYM_IDENTIFIER)
	{
		enter(ID_VARIABLE);
		getsym();
		if (sym == SYM_LEFTSPAREN)
		{
			getsym();
			enterArray();
			arrayDecl();
		}
	}
	else
	{
		error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
	}
}
```
经过测试，数组的声明可以通过，并且正确
#####数组的使用
在数组的使用我们需要清楚两个原则：
首先，数组相当于一个变量，所以它应该在原本在程序中出现ID_VARIABLE的地方也可以出现数组。
其次，数组的位置确定需要两个值，数组的基地址和偏移地址。
从第一个原则可以分析出数组在语法图的相应位置。
从第二个原则可以看出数组需要一个函数（计算偏移地址并放在栈顶），并且需要新的机器指令STOARR和LODARR
首先，先定义一个函数calAdd()
```c
/*传入数组声明在符号表中的位置
动作：将数组元素相对于基地址的元素置于栈顶
具体过程：读取一个新的数后，采取先乘再加的方法
计算出偏移地址并放在栈顶
*/
void calAdd(int i)
{
	void expression_orbit(symset fsys);
	mask *mk = (mask *)&table[i];
	int temp = mk->arrayAdd;

	if (sym == SYM_LEFTSPAREN)//左中括号
	{
		if (presym != SYM_IDENTIFIER && presym != SYM_RIGHTSPAREN)
		{
			printf("expected identifier or rightsparen after leftsparen\n");
			err++;
		}
		else
		{
			getsym();
			expression_orbit(uniteset(createset(SYM_RIGHTSPAREN, SYM_NULL),statbegsys));
			readDim++;//已经读入的数组的维数
			gen(OPR, 0, OPR_ADD);//先加
			if (readDim == arrayDim[temp])
			{
				// gen(LODARR,0,mk->address); //LODARR undeclared!
				if (sym != SYM_RIGHTSPAREN)
				{
					printf("expected rightsparen after expression\n");
					err++;
					return;
				}
				else
				{
					getsym();
					return;
				}
			}
			else
			{
				gen(LIT, 0, arrayDim[temp + readDim + 1]);
				gen(OPR, 0, OPR_MUL);//再乘
				if (sym != SYM_RIGHTSPAREN)
				{
					printf("expected rightsparen after expression\n");
					err++;
					return;
				}
				else
				{
					getsym();
					calAdd(i);
				}
			}
		} //else
	}
	else
	{
		printf("expected leftsparen\n");
		return;
	}
}
```
有了上述过程，我们在每次调用的时候需要初始化,调用格式如下：
```c
gen(LIT, 0, 0);
readDim = 0;//初始化
calAdd(i);
```
当然，判断是否是数组可以用如下的判断条件：
```c
if(id==ID_ARRAY)
```
我们在两个地方添加了数组，分别是statement和factor（具体可从语法图中看出来），添加的代码如下所示：
```c
/*factor中，即id[]合法*/
case ID_ARRAY:
    getsym();
    gen(LIT, 0, 0);
    readDim = 0;//初始化
    calAdd(i);
    mk = (mask *)&table[i];
    gen(LODARR, level - mk->level, mk->address);
    break;
```
```c
/*statement中，id[]:=合法化*/
else if (table[i].kind == ID_ARRAY)
{
	getsym();
	gen(LIT, 0, 0);
	readDim = 0;
	calAdd(i);
	mk = (mask*)&table[i];
	if (sym == SYM_BECOMES)
	{
		getsym();
		expression_orbit(uniteset(createset(SYM_RIGHTSPAREN,SYM_NULL),fsys));
		gen(STOARR, level - mk->level, mk->address);
	}
	else
	{
		error(13); // ':=' expected.
	}
}
```
至此，数组的使用到此结束
####参数传递
#####完善函数的声明过程
在此补充完善函数定义：即函数一定要有（参数列表），其中参数列表可以为空，构建一个函数，来分析函数的参数列表，注意满足一致性，即：进入每一个语法单位处理这个程序之前，其第一个单词已经读出，退出时，应该读入下一个语法单元的第一个单词，我们构建如下函数：
```c
void enterPara(char *idTemp, int kind)
{
	mask* mk;
	tx++;
	strcpy(table[tx].name, idTemp);
	table[tx].kind = kind;
	switch (kind)
	{
	case(ID_VARIABLE):
		mk = (mask*)&table[tx];
		mk->level = level;
		mk->address = pcount--;//pcount初始值为-2，每读到一个数--
		break;
	case(ID_RETURN):/*注意我们有特殊类型，下面介绍*/
		mk = (mask*)&table[tx];
		mk->level = level;
		// printf("The pcount is %d ***************  \n",pcount);
		mk->address = pcount + 1;//return存储应退栈的大小
	}
}
/*在次函数中*/
void paraList(symset fsys)
{
	symset set1, set;
	set1 = createset(SYM_RPAREN, SYM_COMMA, SYM_NULL);
	set = createset(SYM_IDENTIFIER, SYM_RPAREN, SYM_NULL);//presym非标识符时的合法的开始符号
	if (presym == SYM_LPAREN)/*前一个符号是左括号*/
	{
		test(set, fsys, 26);//"There must be an identifier"
		if (sym == SYM_IDENTIFIER)/*遇到标识符*/
		{
			char idTemp[MAXIDLEN + 1];
			strcpy(idTemp, id);
			getsym();
			paraList(fsys);
			enterPara(idTemp, ID_VARIABLE);//登入参数表
		}
	}
	else if (presym == SYM_COMMA)/*前一个符号是逗号*/
	{
		test(set, fsys, 26);//"There must be an identifier"
		if (sym == SYM_IDENTIFIER)/*当前符号是标识符*/
		{
			char idTemp[MAXIDLEN + 1];
			strcpy(idTemp, id);
			getsym();
			paraList(fsys);
			enterPara(idTemp, ID_VARIABLE);
		}
		else/*否则报错*/
		{
			error(26);//"The paralist is wrong",
		}
	}
	else if (presym == SYM_IDENTIFIER)/*前一个符号是标识符*/
	{
		test(set1, fsys, 27);//There must be an ','or')'
		if (sym == SYM_COMMA)
		{
			getsym();
			paraList(fsys);

		}
		/*不是逗号的话返回*/
	}
	else
	{
		error(26);
	}
	destroyset(set1);
	destroyset(set);
}
```
调用这两个函数的语句为函数声明过程，函数的声明在block（）的声明部分，应该在block（）的一开始添加代码如下：
```c
/*确保对于level>0的函数一定有（）*/
if (level > 0) {//不是主函数，判断并进行错误恢复
	set = uniteset(fsys,createset(SYM_RPAREN,SYM_SEMICOLON,SYM_NULL));
	test(createset(SYM_LPAREN, SYM_NULL), set, 28);
	destroyset(set);
}
/*如果有括号，进入参数分析部分*/
if (sym == SYM_LPAREN)
{
	getsym();
	/*为了错误恢复，引入set*/
	set = uniteset(fsys, createset(SYM_RPAREN, SYM_SEMICOLON, SYM_NULL));
	pcount = -2;
	paraList(set);
	destroyset(set);
	enterPara("return", ID_RETURN);
	if (sym == SYM_RPAREN)
	{
		getsym();
	}
	else
	{
		error(22);//"Missing ')'."
	}
}
```
#####函数的调用
函数的调用需要依赖一个分析参数并且将其压栈的过程，因此在此处我们定义一个函数执行这个过程
```c
/*把参数放在栈顶，最后给返回值留下一个空间*/
void procedureCall(symset fsys)
{
	void expression_orbit(symset fsys);
	int i;
	mask *mk;
	if (inset(sym,facbegsys))// 
	{
		expression_orbit(uniteset(createset(SYM_COMMA, SYM_RPAREN, SYM_NULL),fsys));
		procedureCall(fsys);
	}
	else if (sym == SYM_COMMA)
	{
		/*
		if (presym != SYM_IDENTIFIER && presym != SYM_NUMBER && presym != SYM_INC && presym != SYM_DEC)
		{
			printf("Error in procedureCall 2\n");
			error(26);
		}
		else
		{
			getsym();
			procedureCall(fsys);
		}
		*/
		getsym();
		procedureCall(fsys);
	}
	else if (sym == SYM_RPAREN)
	{
		/*
		if (presym != SYM_IDENTIFIER && presym != SYM_LPAREN && presym != SYM_NUMBER && presym != SYM_RIGHTSPAREN && presym != SYM_RPAREN && presym != SYM_INC && presym != SYM_DEC)
		{
			printf("Error in procedureCall 3\n");
			error(26);
		}
		else
		{
			gen(INT, 0, 1);
			getsym();
			return;
		}  // for return value
		*/
		gen(INT, 0, 1);
		getsym();
	}
}
```
函数的调用同数组很像，但函数调用无需初始化变量，但是需要在最后生成一条指令
```c
gen(CAL, level - mk->level, mk->address);
```
需要在两个地方做一些添加，factor和statement
具体过程如下
```c
/*factor中，为了实现id(参数列表)的合法化*/
case ID_PROCEDURE:
	set = uniteset(fsys, createset(SYM_LPAREN, SYM_RPAREN, SYM_SEMICOLON, SYM_NULL));
	getsym();
	test(createset(SYM_LPAREN, SYM_NULL), set, 28);//There must be an '(' after procedure"
	if (sym == SYM_LPAREN)
	{
		getsym();
		procedureCall(set);
		gen(CAL, level - mk->level, mk->address);
	}
	destroyset(set);
	test(createset(SYM_SEMICOLON, SYM_NULL), fsys, 10);//"';' expected."
	break;
/*statement中，实现:=id()合法化*/
else if (table[i].kind == ID_PROCEDURE)
{
	mk = (mask*)&table[i];
	getsym();
	set = uniteset(fsys, createset(SYM_LPAREN, SYM_RPAREN, SYM_SEMICOLON, SYM_NULL));
	test(createset(SYM_LPAREN, SYM_NULL), set, 28);//There must be an '(' after procedure"
	if (sym == SYM_LPAREN)
	{
		getsym();
		procedureCall(set);
		gen(CAL, level, mk->address); // 2017.10.30 level - mk->level change to level
	}
	destroyset(set);
	test(createset(SYM_SEMICOLON, SYM_NULL), fsys, 10);//"';' expected."
}
```
经过测试，代码可以正常通过
####添加语句实现
#####else/elif/exit语句
######else/elif
首先考虑else和elif语句，他们都是条件转移语句，在这里我们需要使用回填技术，但是elif的数量不能确定，因此我们可以通过引入一个辅助数组来记录需要回填的代码行号（这些代码都需要跳转到if-elif-else的末尾），我们可以通过如下的代码来实现if-elif-else，代码添加在statement中。
数组的定义在statement的开始部分：
```c
int i, j=0, cx1, cx2, cx3[100];
/*j和cx3是新定义的数组和变量，j用来确定需要跳转的语句数目，cx3用来存储上述代码位置*/
```
```c
/*
代码块功能：读入if-elif-else并进行分析，
通过回填技术完成各个跳转指令
*/
else if (sym == SYM_IF)
{ // if statement
	j = 0;
	while (sym == SYM_IF || sym == SYM_ELIF)
	{
		getsym();
		if (sym != SYM_LPAREN)
		{
			error(28);
		}
		getsym();
		short_condition_or(uniteset(createset(SYM_RPAREN, SYM_ELIF, SYM_ELSE, SYM_NULL), fsys));
		if (sym != SYM_RPAREN)
		{
			error(22);
		}
		getsym();
		cx1 = cx;
		gen(JZ, 0, 0);//为0则跳转
		statement(uniteset(createset(SYM_RPAREN, SYM_ELIF, SYM_ELSE, SYM_NULL), fsys));
		cx3[j] = cx;//记录每一个的elif和if内的语句结束的地址，如果执行了这些语句，需要跳转到最后
		gen(JMP, 0, 0);
		code[cx1].a = cx;//回填当前if或者elif
		j++;
	}
	printf("now %d:\n", sym);
	if (sym == SYM_ELSE)
	{
		getsym();
		statement(fsys);
	}
	for (i = 0; i < j; i++)
	{
		code[cx3[i]].a = cx;//if-elif-else 的末尾
	}
}
```
######exit的实现
```c
else if (sym == SYM_EXIT)
{
	getsym();
	if (sym != SYM_LPAREN)
	{
		printf("expected leftsparen after exit \n");
		err++;
	}
	getsym();
	if (sym != SYM_RPAREN)
	{
		printf("expected rightsparen after exit \n");
		err++;
	}
	getsym();
	gen(JMP, 0, ENDCX);
}
```
```c
      int main (int argc,char *argv[])
      {
      	...
      	code[ENDCX].f=OPR;
      	code[ENDCX].l=0;
      	code[ENDCX].a=OPR_RET;
      ...
      } // main
```
```c
      #define CXMAX      500    // size of code array
      #define ENDCX	   499
```
所以最后一行代码总是返回，注意是OPR_RET，不是RET。看到exit就生成一条代码，跳转到最后一条代码。
#####return语句和返回值
```c
if (sym == SYM_RETURN)
{
    mask *mk;
    int j = position(id);
    if (j)
    {
    
    	mk = (mask *)&table[j];
    	retOffset = mk->address;
    }
    getsym();
    expression_orbit(fsys);
    gen(STO, 0, -1);
    gen(RET, 0, retOffset); //2017.10.30
}
```
#####for语句的实现
对于for语句，我们将其添加到了语法位置，但对其语义做了如下的限制：
for语句的格式是：
```c
  for(expression1;condition;expression2)
  {
    statement;
  }
```
我们为了符合语义，做如下限制：
具体请看语法图
expression1只能是以变量或者数组或者函数名打头（非保留字）
condition是短路条件
expression2只能是++、--或者标识符打头
在for中实现了i++,i--,++i,--i以及赋值等操作，具体代码如下
```c
else if (sym == SYM_FOR)
{
	instruction codeTemp[100];
	int cxTemp, tempCodeCount;
	int CFalseAdd, ENext;
	getsym();
	if (sym != SYM_LPAREN)
	{
		printf("expected '(' after for declaration\n");
		err++;
	}
	else
	{
		getsym();
		if (sym !=SYM_IDENTIFIER)/*在for中第一个语句只能是标识符打头*/
		{
			printf("expected identifier in the first field of for declaration\n");
			err++;
		}
		else  // E1
		{
			statement(uniteset(createset(SYM_SEMICOLON, SYM_NULL), fsys));
		}
		if (sym != SYM_SEMICOLON)
		{
			printf("expected ';' after the first field in for statement\n");
			err++;
		}
		else  // Condition
		{
			ENext = cx;
			getsym();
			short_condition_or(fsys);
			CFalseAdd = cx;
			gen(JZ, 0, 0);
		}
		if (sym != SYM_SEMICOLON)
		{
			printf("expected ';' after the second field in for statement\n");
			err++;
		}
		else  // E2
		{
			cxTemp = cx;
			getsym();
			if (sym != SYM_IDENTIFIER && sym != SYM_DEC && sym != SYM_INC)
			{
				printf("expected identifier in the first field of for declaration\n");
				err++;
			}
			statement(uniteset(createset(SYM_SEMICOLON, SYM_RPAREN, SYM_NULL), fsys));
			tempCodeCount = cx - cxTemp;
			/*实现代码移动*/
			int j;
			for ( j = 0; j<tempCodeCount; j++)
			{
				codeTemp[j].f = code[cxTemp + j].f;
				codeTemp[j].l = code[cxTemp + j].l;
				codeTemp[j].a = code[cxTemp + j].a;
			}
			cx = cxTemp;
		}
		if (sym != SYM_RPAREN)
		{
			printf("expected SYM_RPAREN\n");
			err++;
		}
		else // body
		{
			getsym();
			statement(fsys);
			int i;
			for (i = 0; i<tempCodeCount; i++)
			{
				code[cx].f = codeTemp[i].f;
				code[cx].l = codeTemp[i].l;
				code[cx++].a = codeTemp[i].a;
			}
			gen(JMP, 0, ENext);
			code[CFalseAdd].a = cx;
		}
	}
}
```
#####++和--的实现
截至目前，我们实现++和--还是对变量进行操作，主要改动的地方在factor和statement中，注意为什么要在statement中进行添加，因为我们如果只在factor中进行添加的话，那么i++和i--以及++i，--i就不能作为一个完整的语句
```c
/*在statement中，主要任务是实现++i,--i，并且其可以作为一条语句*/
else if (sym == SYM_INC)
{
	getsym();
	if (sym != SYM_IDENTIFIER)
	{
		printf("expected id here \n");
		err++;
		getsym();
	}
	else
	{
		int i = position(id);
		mask *mk = (mask *)&table[i];
		gen(LOD, level - mk->level, mk->address);
		gen(LIT, 0, 1);
		gen(OPR, 0, OPR_ADD);
		gen(STO, level - mk->level, mk->address);
		gen(LOD, level - mk->level, mk->address);
		getsym();
	}
}
else if (sym == SYM_DEC)
{
	getsym();
	if (sym != SYM_IDENTIFIER)
	{
		printf("expected id here \n");
		err++;
		getsym();
	}
	else
	{
		int i = position(id);
		mask *mk = (mask *)&table[i];
		gen(LOD, level - mk->level, mk->address);
		gen(LIT, 0, 1);
		gen(OPR, 0, OPR_MIN);
		gen(STO, level - mk->level, mk->address);
		gen(LOD, level - mk->level, mk->address);
		getsym();
	}
}
```
```c
/*在statement的sym_identifire中进行如下语句的添加，主要目的：允许i++,i--称为一个完整的句子*/
else if (table[i].kind != ID_VARIABLE)
{
	error(12); // Illegal assignment.
	i = 0;
}
else {		//table[i].kind == ID_VARIABLE
	getsym();
	mk = (mask*)&table[i];
	if (sym == SYM_BECOMES)
	{
		getsym();
		expression_orbit(fsys);
		/*层次差+偏移量寻址*/
		if (i)
		{
			gen(STO, level - mk->level, mk->address);
		}
	}
	else if (sym==SYM_INC)
	{
	    gen(LOD, level - mk->level, mk->address);
		gen(LOD, level - mk->level, mk->address);
		gen(LIT, 0, 1);
		gen(OPR, 0, OPR_ADD);
		gen(STO, level - mk->level, mk->address);
		getsym();
	}
	else if (sym == SYM_DEC)
	{
	    gen(LOD, level - mk->level, mk->address);
		gen(LOD, level - mk->level, mk->address);
		gen(LIT, 0, 1);
		gen(OPR, 0, OPR_MIN);
		gen(STO, level - mk->level, mk->address);
		getsym();
	}
	else
	{
		error(13); // "':=','++'or'--' expected.",
	}
}
}
```
在factor中进行修改，与上面类似，只不过我们的目的是使得i++,i--,++i,--i可以代替原先的i，注意在这里我们需要考虑到++i和i++的不同，因此我们会生成不同的指令
```c
/*实现i++,i--替换i*/
case ID_VARIABLE:
					/*如果标识符是变量名，生成LOD指令*/
					/*把位于当前层次的偏移地址为address的值放在栈顶*/
					mk = (mask*)&table[i];
					gen(LOD, level - mk->level, mk->address);
					getsym();
					/*如果遇到++*/
					if (sym == SYM_INC)
					{
						gen(LOD, level - mk->level, mk->address);//将变量的值置于栈顶
						gen(LIT, 0, 1);//常数置于栈顶
						gen(OPR, 0, OPR_ADD);//相加
						gen(STO, level - mk->level, mk->address);//将栈顶的值赋予变量
						getsym();
					}
					else if (sym == SYM_DEC)
					{
						gen(LOD, level - mk->level, mk->address);
						gen(LIT, 0, 1);
						gen(OPR, 0, OPR_MIN);
						gen(STO, level - mk->level, mk->address);
						getsym();
					}
					break;
```
```c
/*实现++i，--i替换i*/
else if (sym == SYM_INC) // ++
{
	getsym();
	if (sym != SYM_IDENTIFIER)
	{
		printf("expected id here \n");
		err++;
		getsym();
	}
	else
	{
		int i = position(id);
		mask *mk = (mask *)&table[i];
		gen(LOD, level - mk->level, mk->address);//将变量值置于栈顶
		gen(LIT, 0, 1);//常数置于栈顶
		gen(OPR, 0, OPR_ADD);//相加
		gen(STO, level - mk->level, mk->address);//将栈顶的值赋予变量
		gen(LOD, level - mk->level, mk->address);//将变量值置于栈顶
		getsym();
	}
}
else if (sym == SYM_DEC)
{
	getsym();
	if (sym != SYM_IDENTIFIER)
	{
		printf("expected id here \n");
		err++;
		getsym();
	}
	else
	{
		int i = position(id);
		mask *mk = (mask *)&table[i];
		gen(LOD, level - mk->level, mk->address);
		gen(LIT, 0, 1);
		gen(OPR, 0, OPR_MIN);
		gen(STO, level - mk->level, mk->address);
		gen(LOD, level - mk->level, mk->address);
		getsym();
	}
}
```