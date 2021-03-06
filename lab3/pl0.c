// pl0 compiler source code

#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pl0.h"
#include "set.c"

//////////////////////////////////////////////////////////////////////
// print error message.
void error(int n)
{
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s  ", n, err_msg[n]);
	printf("the sym in here is %d\n",sym);//err_msg[]是各种错误输出信息的数组。在pl0.h_78有定义。
	err++;
} // error

//////////////////////////////////////////////////////////////////////
void getch(void)
{
	if (cc == ll)
	{
		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;//ll是line length，在pl0.h_121有定义。
		printf("%5d  ", cx);//cx是下标。 index of current instruction to be generated.
		while ( (!feof(infile)) // added & modified by alex 01-02-09
			    && ((ch = getc(infile)) != '\n') )
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = '\n';
	}
	ch = line[++cc];//将line[1]中的元素赋值给ch。
} // getch

//////////////////////////////////////////////////////////////////////
// gets a symbol from input stream.
void getsym(void)
{
	presym=sym;
	int i, k;
	char a[MAXIDLEN + 1];

	while (ch == ' '||ch == '\t'||ch == '\n'||ch == '\r')//跳过空白符 
		getch();
	// printf("%c",ch);
	if (isalpha(ch))
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		}
		while (isalpha(ch) || isdigit(ch));//由字母和数字组成标识符或者保留字。
		a[k] = 0;
		strcpy(id, a);
		word[0] = id;//word[]是字符串数组，在pl0.h_132中有定义。存储着保留字。其中word[0]放空。
		i = NRW;
		while (strcmp(id, word[i--]));//比较word[i]和id，直到相等。（总会与其中一个项相等，因为word[0]里拷贝进了id)
		if (++i)//如果不是word[0]与id相等，说明id是保留字。
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   // symbol is an identifier
	}
	else if (isdigit(ch))
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do  //字符串表示的数字转换成数字形式
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		}
		while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')
	{
		getch();
		if (ch == '=')// ":="  是赋值符号.
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else //:后面接其他就代表是单独的符号。一般是用来做变量类型的定义。
		{
			sym = SYM_COLON;  
			getch();     // illegal?
		}
	}
	else if (ch == '>')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <> 不等于
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
	else if (ch == '^')
	{
		sym = SYM_XOR;
		getch();
	}
	else if (ch == '/')
	{
		getch();
		if (ch == '/')  //发现代码中出现了//，那么无视这一行后面的所有内容
		{
			getch();
			while((ch != '\n') && (ch != '\r'))
				getch();
			getsym();
		}
		else if (ch == '*')  //如果发现了/*，那么无视/* */及其中的所有内容
		{
			getch();
			while(1)
			{
				while(ch != '*')
					getch();
				getch();
				if(ch == '/')
				{
					getch();
					getsym();
					break;
				}
			}
		}
		else //如果后面不是/也不是*，那么它就是除法符号，除法符号是ssym[4]
		{
			sym = ssym[4];
		}
	}
	else if(ch == '&')
	{
		getch();
		if(ch == '&')
		{
			sym = SYM_AND;
			getch();
		}
		else 
		{
			sym = SYM_ANDBIT;
		}
	}
	else if(ch == '|')
	{
		getch();
		if(ch == '|')
		{
			sym = SYM_OR;
			getch();
		}
		else 
		{
			sym = SYM_ORBIT;
		}
	}							
	else
	{ // other tokens
		i = NSYM;  
		csym[0] = ch;
		while (csym[i--] != ch);  //检查是否是'+', '-', '*', '/', '(', ')', '=', ',', '.', ';'中的一个
		if (++i)  //是其中一个
		{
			sym = ssym[i];
			getch();
			if( sym==SYM_PLUS   &&  ch=='+')
			{
				sym=SYM_INC;
				getch();
			}
			else if(sym == SYM_MINUS && ch == '-')
			{
				sym=SYM_DEC;
				getch();
			}
		}
		else  //不是其中一个，无法识别的符号
		{
			printf("Fatal Error: Unknown character. And the presym is %d\n", sym);
			//printf("%c",ch);
			exit(1);
		}
	}
} // getsym

// generates (assembles) an instruction.
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
} // gen

// tests if error occurs and skips all symbols that do not belongs to s1 or s2.
void test(symset s1, symset s2, int n)
{
	symset s;

	if (! inset(sym, s1))
	{
		error(n);
		s = uniteset(s1, s2);
		while(! inset(sym, s))
			getsym();
		destroyset(s);
	}
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

int pcount=-2;
// enter object(constant, variable or procedre) into table.
void enter(int kind)
{
	mask* mk;

	tx++;
	strcpy(table[tx].name, id);
	table[tx].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_VARIABLE:
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_PROCEDURE:
		mk = (mask*) &table[tx];
		mk->level = level;
		break;
	case ID_POINTER:
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
	} // switch

	// printf("Message of var in table is : name = %s  level = %d  address = %d  \n",table[tx].name,mk->level,(int)(mk->address));
} // enter

void enterPara(char *idTemp,int kind)
{
	mask* mk;
	tx++;
	strcpy(table[tx].name, idTemp);
	table[tx].kind = kind;
	switch(kind)
	{
		case(ID_VARIABLE):
			mk = (mask*) &table[tx];
			mk->level = level;
			mk->address = pcount--;
			break;
		case(ID_RETURN):
			mk=(mask*) &table[tx];
			mk->level = level;
			// printf("The pcount is %d ***************  \n",pcount);
			mk->address = pcount+1;
	}
	// printf("Message of var in table is : name = %s  level = %d  address = %d  \n",table[tx].name,mk->level,(int)(mk->address));
}

void enterArray()
{
	mask *mk;
	tx;
	table[tx].kind = ID_ARRAY;
	mk = (mask *)&table[tx];
	mk->level = level;
	mk->address = dx++;
	mk -> arrayAdd =++adx;
	arrayDim[adx]=0;
}

//////////////////////////////////////////////////////////////////////
// locates identifier in symbol table.
int position(char* id)
{
	int i;
	strcpy(table[0].name, id);
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0);
	return i;
} // position

//////////////////////////////////////////////////////////////////////
void constdeclaration()
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	} else	error(4);
	 // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
void vardeclaration(void)
{
	if(sym == SYM_IDENTIFIER)
	{
		enter(ID_VARIABLE);
		getsym();
		if(sym == SYM_LEFTSPAREN)
		{
			getsym();
			enterArray();
			arrayDecl();
		}
	}
	else if(sym == SYM_TIMES)
	{
		getsym();
		while(sym == SYM_TIMES)getsym();
		if(sym == SYM_IDENTIFIER)
		{
			enter(ID_POINTER);
			getsym();
		}
		else
		{
			printf("expected id as a pointer declaration\n");
			exit(1);
		}
	}
	else
	{
		printf("Error:expected SYM_IDENTIFIER in vardeclaration\n");
		err++;
	}
} // vardeclaration

//////////////////////////////////////////////////////////////////////
void listcode(int from, int to)
{
	int i;
	
	printf("\n");
	for (i = from; i < to; i++)
	{
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
	printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////
void procedureCall()
{
	int i;
	mask *mk;
	if(sym == SYM_IDENTIFIER || sym == SYM_NUMBER || sym == SYM_ANDBIT)
	{
		expr_andbit(statbegsys);
		procedureCall();
	}
	else if(sym == SYM_COMMA)
	{
		if(presym != SYM_IDENTIFIER && presym != SYM_NUMBER)
		{
			printf("Error in procedureCall 2\n");
			error(26);
		}
		else
		{
			getsym();
			procedureCall();
		}
	}
	else if(sym == SYM_RPAREN)
	{
		if(presym != SYM_IDENTIFIER && presym != SYM_LPAREN && presym != SYM_NUMBER && presym != SYM_RIGHTSPAREN && presym != SYM_RPAREN)
		{
			printf("Error in procedureCall 3\n");
			error(26);
		}
		else
		{
			 gen(INT,0,1);
			 getsym();
			 return;
		}  // for return value
	}
	else
	{
		printf("Error in procedureCall 4 and the sym is %d \n",sym);
		error(26);
	}
}


void factor(symset fsys)
{
	void expr_andbit(symset fsys);
	int i;
	symset set;
	
	// test(facbegsys, fsys, 24); 
	// The symbol can not be as the beginning of an expression.
	// printf("the sym in factor is  *************   %d \n",sym);
	while(inset(sym, facbegsys))
	{
		if (sym == SYM_IDENTIFIER)
		{
			int j=position(id);
			mask *mk=(mask *) &table[j];
			if(j)
			{

				if(table[j].kind == ID_PROCEDURE)//procedure Call
				{
					// printf("Is that correct? the ooxx is %s \n",table[j].name);
					getsym();
					if(sym == SYM_LPAREN)
					{
						getsym();
						procedureCall();
						gen(CAL, level - mk->level, mk->address);
					}
				}
				//**********20171102************
				else if(table[j].kind == ID_ARRAY)
				{
					getsym();
					gen(LIT,0,0);
					readDim=0;
					calAdd(j);
					if(sym == SYM_BECOMES)
					{
						getsym();
						expr_andbit(fsys);
						gen(STOARR,level-mk->level,mk->address);
						gen(CPY,0,2);
					}
					else gen(LODARR,level- mk->level,mk->address);
				}
				//******************************
				else if(table[j].kind == ID_CONSTANT)
				{
					gen(LIT,0,table[j].value);
					getsym();
				}
				else // normal variable
				{
						// printf("Is that correct ? kind = %d \n",table[j].kind);
					mask* mk;
					mk = (mask*) &table[j];
					getsym();
					if(sym == SYM_BECOMES)
					{
						getsym();
						expr_andbit(fsys);
						gen(STO,level-mk->level,mk->address);
						gen(CPY,0,1);
					}
					else if(sym == SYM_INC)
					{
						gen(LOD, level - mk->level, mk->address);
						printf("target in SYM_INC\n");
						gen(LOD, level-mk->level, mk->address);
						gen(LIT, 0, 1);
						gen(OPR, 0, OPR_ADD);
						gen(STO, level -mk->level, mk->address);
						getsym();
					}
					else if(sym == SYM_DEC)
					{
						gen(LOD, level - mk->level, mk->address);
						gen(LOD, level-mk->level, mk->address);
						gen(LIT, 0, 1);
						gen(OPR, 0, OPR_MIN);
						gen(STO, level -mk->level, mk->address);
						getsym();
					}
					else gen(LOD, level - mk->level, mk->address);
						// printf("After return q(n the sym is : %d\n",sym);
				}
			}
			else
			{
				printf("Error : false in factor that Undeclared id\n");
				getsym();
			}
			break;
		}
		else if (sym == SYM_NUMBER)
		{
			if (num > MAXADDRESS)
			{
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, num);
			getsym();
			break;
		}
		else if (sym == SYM_LPAREN)
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			expr_andbit(set);
			//expression(set);
			destroyset(set);
			if (sym == SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}
		else if(sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr
		{  
			 getsym();
			 expr_andbit(fsys);
			 //expression(fsys);
			 gen(OPR, 0, OPR_NEG); //不是减
		}
		//***************9.30添加下面的非***************
		else if(sym == SYM_NOT)
		{
			getsym();
			expr_andbit(fsys);
			//expression(fsys);
			gen(OPR,0,OPR_NOT);  //NOT
		}
		else if(sym == SYM_INC)
		{
			getsym();
			if(sym != SYM_IDENTIFIER)
			{
				printf("expected id here \n");
				err++;
				getsym();
			}
			else
			{
				int i=position(id);
				mask *mk=(mask *)&table[i];
				gen(LOD, level-mk->level, mk->address);
				gen(LIT, 0 ,1);
				gen(OPR, 0, OPR_ADD);
				gen(STO, level-mk->level, mk->address);
				gen(LOD, level-mk->level, mk->address);
				getsym();
			}
			break;
		}
		else if(sym == SYM_DEC)
		{
			getsym();
			if(sym != SYM_IDENTIFIER)
			{
				printf("expected id here \n");
				err++;
				getsym();
			}
			else
			{
				int i=position(id);
				mask *mk=(mask *)&table[i];
				gen(LOD, level-mk->level, mk->address);
				gen(LIT, 0 ,1);
				gen(OPR, 0, OPR_MIN);
				gen(STO, level-mk->level, mk->address);
				gen(LOD, level-mk->level, mk->address);
				getsym();
			}
			break;
		}
		else if(sym == SYM_ANDBIT)
		{
			getsym();
			if(sym == SYM_IDENTIFIER)
			{
				int i=position(id);
				if(i || table[i].kind == ID_ARRAY || table[i].kind == ID_VARIABLE)
				{
					mask *mk = (mask *) &table[i];
					gen(LODADD,level-mk->level,mk->address);
					getsym();
				}
				else 
				{
					printf("There are some errors in SYM_ANDBIT factor\n");
					exit(1);
				}
			}
			else
			{
				printf("expected SYM_IDENTIFIER after LODADD SYM_ANDBIT\n");
				exit(1);
			}
		}
		else if(sym == SYM_TIMES)
		{
			getsym();
			TimesBody(fsys);
			gen(LODA,0,0);
		}

		// test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);
	} // while
} // factor

//////////////////////////////////////////////////////////////////////
void term(symset fsys)
{
	int mulop;
	symset set;
	//****************10.9添加SYM_MOD***********************
	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH,SYM_MOD,SYM_XOR,SYM_NULL));
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH || sym == SYM_MOD || sym == SYM_XOR)  //'*' '/' '%' '^'
	{
		mulop = sym;
		getsym();
		factor(set);
		if (mulop == SYM_TIMES)
		{
			gen(OPR, 0, OPR_MUL);
		}
		else if(mulop == SYM_MOD)
		{
			gen(OPR, 0, OPR_MOD);
		}
		else if(mulop ==SYM_XOR)
		{
			gen(OPR, 0, OPR_XOR);
		}
		else
		{
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
} // term

//////////////////////////////////////////////////////////////////////
void expression(symset fsys)
{
	int addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));
	
	term(set);
	while (sym == SYM_PLUS || sym == SYM_MINUS) // '+' '-'
	{
		addop = sym;
		getsym();
		term(set);
		if (addop == SYM_PLUS)
		{
			gen(OPR, 0, OPR_ADD);
		}
		else
		{
			gen(OPR, 0, OPR_MIN);
		}
	} // while

	destroyset(set);
} // expression

//////////////////////////////////////////////////////////////////////
/************************10.10添加下面的函数*********************/
void expr_andbit(symset fsys)
{
	symset set;

	set = uniteset(fsys, createset(SYM_ANDBIT,SYM_NULL));
	
	expression(set);
	//term(set);
	while (sym == SYM_ANDBIT)  // &
	{
		getsym();
		expression(set);
		//term(set);
		gen(OPR, 0, OPR_ANDBIT);
	} // while

	destroyset(set);
}

////////////////////////////////////////////////////
void condition(symset fsys)
{
	int relop;
	symset set;
	if (sym == SYM_ODD)
	{
		getsym();
		expr_andbit(fsys);
		//expression(fsys);
		gen(OPR, 0, 6);  //OPR_ODD
	}
	else
	{
		//set = uniteset(relset, fsys);
		expr_andbit(relset);
		//expression(relset);
		//destroyset(set);
		if (! inset(sym, relset))
		{
			//
		}
		else
		{
			relop = sym;
			getsym();
			expression(fsys);
			switch (relop)
			{
			case SYM_EQU:
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ:
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES:
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ:
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR:
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ:
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else

} // condition

/*******************↓↓↓↓9.30号添加下面这一块↓↓↓↓************************/
//////////////////////////////////////////////////////////////////////
void conditions_and(symset fsys)  //condition是一个条件，conditions是多个条件的复合
{
	//int op;
	symset set;
	set = uniteset(fsys, createset(SYM_AND, SYM_NULL));   //类似与term的代码
	condition(set);
	//destroyset(set);
	while (sym == SYM_AND)
	{
		//op = sym;
		getsym();
		condition(set);
		gen(OPR,0,OPR_AND);
	}
	destroyset(set);
}

void conditions_or(symset fsys)
{
	symset set;
	set = uniteset(fsys,createset(SYM_OR,SYM_NULL));
	conditions_and(set);
	while (sym == SYM_OR)
	{
		getsym();
		conditions_and(set);
		gen(OPR,0,OPR_OR);
	}
	destroyset(set);
}
/****************↑↑↑↑9.30号添加↑↑↑↑************************/


void calAdd(int i)
{
	mask *mk=(mask *)&table[i];
	int temp=mk->arrayAdd;

	if(sym == SYM_LEFTSPAREN)
	{
		if(presym != SYM_IDENTIFIER && presym != SYM_RIGHTSPAREN)
		{
			printf("expected identifier or rightsparen after leftsparen\n");
			err++;
		}
		else
		{
			getsym();
			expr_andbit(statbegsys);
			readDim++;
			gen(OPR,0,2);
			if(readDim == arrayDim[temp])
			{
				// gen(LODARR,0,mk->address); //LODARR undeclared!
				if(sym != SYM_RIGHTSPAREN)
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
				gen(LIT,0,arrayDim[temp+readDim+1]);
				gen(OPR,0,OPR_MUL);
				if(sym != SYM_RIGHTSPAREN)
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

void short_condition(symset fsys, list *trueList, list *falseList)
{
	int relop;
	symset set;
	if (sym == SYM_ODD)
	{
		getsym();
		expr_andbit(fsys);
		gen(OPR, 0, 6);  //OPR_ODD
	}
	// else if(sym == SYM_LPAREN)
	// {
	// 	getsym();
	// 	short_condition_or(fsys, trueList,falseList);
	// 	if(sym != SYM_RPAREN)
	// 	{
	// 		printf("expected ) in short_condition while the sym is %d\n",sym);
	// 		exit(1);
	// 	}
	// 	else getsym();
	// }
	else
	{
		expr_andbit(relset);
		if (! inset(sym, relset))
		{	
			list *tail = trueList->tail;
			list *pt = (list *)malloc(sizeof(list));
			tail->next = pt;
			pt->cx = cx;
			pt->next = NULL;
			trueList->tail = pt;

			tail = falseList->tail;
			list *pf = (list *)malloc(sizeof(list));
			tail->next = pf;
			pf->cx = cx;
			pf->next = NULL;
			falseList->tail = pf;

			gen(JNZ,0,0);
		}
		else
		{
			relop = sym;
			getsym();
			expression(fsys);
			list *tail = trueList->tail;
			list *pt = (list *)malloc(sizeof(list));
			tail->next = pt;
			pt->cx = cx;
			pt->next = NULL;
			trueList->tail = pt;

			tail = falseList->tail;
			list *pf = (list *)malloc(sizeof(list));
			tail->next = pf;
			pf->cx = cx;
			pf->next =NULL;
			falseList->tail = pf;

			switch (relop)
			{
			case SYM_EQU:
				gen(JE, 0, 0);
				break;
			case SYM_NEQ:
				gen(JNE, 0, 0);
				break;
			case SYM_LES:
				gen(JL, 0, 0);
				break;
			case SYM_LEQ:
				gen(JLE, 0, 0);
				break;	
			case SYM_GEQ:
				gen(JGE, 0, 0);
				break;
			case SYM_GTR:
				gen(JG, 0, 0);
				break;

			} // switch
		} // else
	} // else

} // condition

void short_condition_and(symset fsys,list *trueList,list *falseList)
{
	list *trueList_1 = (list *)malloc(sizeof(list));
	list *p;
	trueList_1->next =NULL;
	trueList_1->tail = trueList_1;
	short_condition(fsys,trueList_1,falseList);
	while(sym == SYM_AND)
	{
		getsym();
		p = trueList_1->next;
		while(1)
		{
			if(p == NULL)break;
			code[p->cx].a=cx;
			p=p->next;
		}
		trueList_1->next = NULL;
		trueList_1->tail = trueList_1;
		short_condition(fsys,trueList_1,falseList);
	}
	p = trueList_1->next;
	list *tail = trueList->tail;
	if(p!=NULL)
	{
		tail->next = p;
		while(p->next != NULL)p=p->next;
		trueList->tail = p;
	}
	tail = trueList->tail;
	tail->next = NULL;
}

void short_condition_or(symset fsys,list *trueList, list *falseList)
{
	list *falseList_1 = (list *)malloc(sizeof(list));
	list *p;
	falseList_1->next = NULL;
	falseList_1->tail = falseList_1;
	short_condition_and(fsys,trueList,falseList_1);
	while(sym == SYM_OR)
	{
		getsym();
		p = falseList_1->next;
		while(1)
		{
			if(p == NULL)break;
			code[p->cx].l=cx;
			p=p->next;
		}
		falseList_1->next = NULL;
		falseList_1->tail = falseList_1;

		short_condition_and(fsys,trueList,falseList_1);
	}
	p = falseList_1->next;
	list *tail = falseList->tail;
	if(p!=NULL)
	{
		tail->next = p;
		while(p->next != NULL)p=p->next;
		falseList->tail = p;
	}
	tail = falseList->tail;
	tail->next = NULL;
}

void SwitchBody(symset fsys,int lastcx)
{
	if(sym != SYM_CASE && sym != SYM_DEFAULT)
	{
		printf("expected case or default in SwitchBody\n");
		exit(1);
	}
	if(sym == SYM_CASE)
	{

		getsym();
		if(sym != SYM_NUMBER)
		{
			printf("expected number after case\n");
			exit(1);
		}
		if(lastcx != 0)code[lastcx].a=cx;
		gen(LIT,0,num);
		int cxTemp = cx;
		gen(JNE,cx+1,0);
		getsym();
		if(sym != SYM_COLON)
		{
			printf("expected : in case\n");
			exit(1);
		}
		getsym();
		statement(fsys);
		while (inset(sym, statbegsys))
		{
			statement(fsys);
		} 
		SwitchBody(fsys,cxTemp);
	}
	else // default
	{
		getsym();
		if(sym != SYM_COLON)
		{
			printf("expected : in case\n");
			exit(1);
		}
		getsym();
		if(lastcx != 0)code[lastcx].a=cx;
		statement(fsys);
		while (inset(sym, statbegsys))
		{
			statement(fsys);
		} 
	}
}


void TimesBody(symset fsys)
{
	if(sym == SYM_TIMES)
	{
		getsym();
		TimesBody(fsys);
		gen(LODA,0,0);
	}
	else if(sym == SYM_IDENTIFIER)
	{
		expr_andbit(fsys);
	}
}

void statement(symset fsys)
{
	int i, cx1, cx2;
	symset set1, set;
	int retOffset;
	///****************1027*****************************
	if(sym == SYM_RETURN)
	{
			mask *mk;
			int j=position(id);
			if(j)
			{

				mk=(mask *) &table[j];
				retOffset=mk->address;
			}
			getsym();
			expr_andbit(fsys);
			gen(STO,0,-1);
			gen(RET,0,retOffset); //2017.10.30
			gen(BAC,0,1);
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ; in 896 but sym here is %d\n",sym);
				err++;
				getsym();
			}
			else getsym();
	}
	///*************************************************
	else if (sym == SYM_IDENTIFIER)
	{ // variable assignment
		mask* mk;
		if (! (i = position(id)))
		{
			error(11); // Undeclared identifier.
		}
		///****************1026*****************************
		else if (table[i].kind == ID_PROCEDURE)
		{
			mk = (mask*) &table[i];
			getsym();
			if(sym == SYM_LPAREN)
			{
				getsym();
				procedureCall();
				gen(CAL, level, mk->address); // 2017.10.30 level - mk->level change to level
			}
			else
			{
				printf("Error in statement .After procedureCall \n");
				exit(0);
			}
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ; in 903 \n");
				err++;
				getsym();
			}
			else getsym();
		}
		else if (table[i].kind == ID_ARRAY)
		{
			getsym();
			gen(LIT,0,0);
			readDim=0;
			calAdd(i);
			mk = (mask*) &table[i];
			if(sym == SYM_BECOMES)
			{
				getsym();
				expr_andbit(fsys);
				gen(STOARR,level-mk->level,mk->address);
			}
			else
			{
				error(13); // ':=' expected.
			}
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ; in 928 \n");
				err++;
				getsym();
			}
			else getsym();
		}
		else // table[i].kind == ID_VARIABLE
		{
			getsym();
			if (sym == SYM_BECOMES)
			{
				getsym();
			}
			else
			{
				error(13); // ':=' expected.
			}
			expr_andbit(fsys);
			mk = (mask*) &table[i];
			if (i)
			{
				gen(STO, level - mk->level, mk->address);
			}		/* code */
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ; in  953 but sym here is %d \n",sym);
				err++;
				getsym();
			}
			else getsym();
		}
	}

	else if(sym == SYM_TIMES)
	{
		getsym();
		TimesBody(fsys);
		if(sym == SYM_BECOMES)
		{
			getsym();
			expr_andbit(fsys);
			gen(STOA,0,0);
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ; in the end of SYM_TIMES \n");
				exit(1);
			}
			else getsym();
		}
	}
	
	else if(sym == SYM_SWITCH)
	{
		breakLevel++;
		breakCx[breakLevel] = (int *)malloc(50*sizeof(int));
		breakCx[breakLevel][0] = 0;
		getsym();
		if(sym != SYM_LPAREN)
		{
			printf("expected ( in switch while sym is %d\n",sym);
			exit(1);
		}
		getsym();
		if(sym == SYM_IDENTIFIER)
		{
			int i=position(id);
			if(i)
			{
				mask *mk = (mask *) &table[i];
				gen(LOD,level-mk->level,mk->address);
			}
			else
			{
				printf("the id undeclared!\n");
			}
		}
		else if(sym == SYM_NUMBER)
		{
			gen(LIT,0,num);
		}
		getsym();
		if(sym != SYM_RPAREN)
		{
			printf("expected ) in switch while sym is %d\n",sym);
			printf("Only a number or identifier is permitted\n");
			exit(1);
		}
		getsym();
		if(sym != SYM_BEGIN)
		{
			printf("expected begin in switch while sym is %d\n",sym);
			exit(1);
		}
		getsym();
		SwitchBody(fsys,0);
		for(int i=breakCx[breakLevel][0];i>0;i--)
		{
			printf("i is %d\n",breakCx[breakLevel][i]);
			code[breakCx[breakLevel][i]].a = cx;
		}
		free(breakCx[breakLevel]);
		breakLevel--;

		if(sym != SYM_END)
		{
			printf("expected end in switch \n ");
			exit(1);
		}
		getsym();
		if(sym != SYM_SEMICOLON)
		{
			printf("expected ; in switch end \n");
			exit(1);
		}
		getsym();
	}

	else if(sym == SYM_EXIT)
	{
		getsym();
		if(sym != SYM_LPAREN)
		{
			printf("expected leftsparen after exit \n");
			err++;
		}
		getsym();
		if(sym != SYM_RPAREN)
		{
			printf("expected rightsparen after exit \n");
			err++;
		}
		getsym();
		if(sym != SYM_SEMICOLON)
		{
			printf("expected ; in 976 \n");
			err++;
			getsym();
		}
		else getsym();
		gen(JMP,0,ENDCX);	
	}

	else if (sym == SYM_FOR)
	{
		list *trueList_1 = (list *)malloc(sizeof(list));
		trueList_1->next = NULL;
		trueList_1->tail = trueList_1;
		list *falseList_1 = (list *)malloc(sizeof(list));
		falseList_1->next = NULL;
		falseList_1->tail = falseList_1;	

		loopLevel++;
		breakLevel++;
		breakCx[breakLevel] = (int *)malloc(50*sizeof(int));
		breakCx[breakLevel][0] = 0;
		instruction codeTemp[100];
		int cxTemp,tempCodeCount;
		int CFalseAdd,ENext;
		getsym();
		if(sym != SYM_LPAREN)
		{
			printf("expected '(' after for declaration\n");
			err++;
		}
		else
		{
			getsym();
			if(sym != SYM_IDENTIFIER)
			{
				printf("expected identifier in the first field of for declaration\n");
				err++;
			}
			else  // E1
			{
				int i=position(id);
				mask *mk = (mask *)&table[i];
				if(i)
				{
					if(table[i].kind == ID_VARIABLE)
					{
						getsym();
						if(sym == SYM_BECOMES)
						{
							getsym();
							expr_andbit(fsys);
							gen(STO,level-mk->level,mk->address);
						}
					}
					else // table[i].kind == ID_ARRAY
					{
						getsym();
						gen(LIT,0,0);
						readDim=0;
						calAdd(i);
						if(sym == SYM_BECOMES)
						{
							getsym();
							expr_andbit(fsys);
							gen(STOARR,level-mk->level,mk->address);
						}
					}
				}
				else
				{
					printf("Identifier undeclared!\n");
					err++;
				}
			}
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ';' after the first field in for statement\n");
				err++;
			}
			else  // Condition
			{	
				ENext = cx;
				getsym();
				short_condition_or(fsys,trueList_1,falseList_1);

				loopCx[loopLevel]=cx;
				
				list *p = trueList_1->next;
				while(p != NULL)
				{
					code[p->cx].a = cx;
					p=p->next;
				}

			}
			if(sym != SYM_SEMICOLON)
			{
				printf("expected ';' after the second field in for statement\n");
				err++;
			}
			else  // E2
			{
				cxTemp=cx;
				getsym();
				if(sym != SYM_IDENTIFIER && sym != SYM_DEC && sym != SYM_INC)
				{
					printf("expected identifier in the first field of for declaration\n");
					err++;
				}
				if(sym == SYM_INC)
				{
					getsym();
					if(sym != SYM_IDENTIFIER)
					{
						printf("expected id here \n");
						err++;
						getsym();
					}
					else
					{
						int i=position(id);
						mask *mk=(mask *)&table[i];
						gen(LOD,level-mk->level,mk->address);
						gen(LIT,0,1);
						gen(OPR,0,OPR_ADD);
						gen(STO,level-mk->level,mk->address);
						getsym();
					}
				}
				else if(sym == SYM_DEC)
				{
					getsym();
					if(sym != SYM_IDENTIFIER)
					{
						printf("expected id here \n");
						err++;
						getsym();
					}
					else
					{
						int i=position(id);
						mask *mk=(mask *)&table[i];
						gen(LOD,level-mk->level,mk->address);
						gen(LIT,0,1);
						gen(OPR,0,OPR_MIN);
						gen(STO,level-mk->level,mk->address);
						getsym();
					}
				}
				else
				{
					int i=position(id);
					mask *mk = (mask *)&table[i];
					if(i)
					{
						if(table[i].kind == ID_VARIABLE)
						{
							getsym();
							if(sym == SYM_BECOMES)
							{
								getsym();
								expr_andbit(fsys);
								gen(STO,level-mk->level,mk->address);
							}
							if(sym == SYM_INC)
							{
						
								gen(LOD, level-mk->level, mk->address);
								gen(LIT, 0, 1);
								gen(OPR, 0, OPR_ADD);
								gen(STO, level -mk->level, mk->address);
								getsym();
							}
							else if(sym == SYM_DEC)
							{
								gen(LOD, level-mk->level, mk->address);
								gen(LIT, 0, 1);
								gen(OPR, 0, OPR_MIN);
								gen(STO, level -mk->level, mk->address);
								getsym();
							}
						}
						else // table[i].kind == ID_ARRAY
						{
							getsym();
							gen(LIT,0,0);
							readDim=0;
							calAdd(i);
							mk = (mask*) &table[i];
							if(sym == SYM_BECOMES)
							{
								getsym();
								expr_andbit(fsys);
								gen(STOARR,level-mk->level,mk->address);
							}
						}
					}
					else
					{
						printf("Identifier undeclared!\n");
						err++;
					}
				}
				tempCodeCount=cx - cxTemp ;
				for(int j=0;j<tempCodeCount;j++)
				{
					codeTemp[j].f=code[cxTemp+j].f;
					codeTemp[j].l=code[cxTemp+j].l;
					codeTemp[j].a=code[cxTemp+j].a;
				}
				cx=cxTemp;
			}
			if(sym != SYM_RPAREN)
			{
				printf("expected SYM_RPAREN\n");
				err++;
			}
			else // body
			{
				getsym();
				statement(fsys);
				for(int i=0;i<tempCodeCount;i++)
				{
					code[cx].f=codeTemp[i].f;
					code[cx].l=codeTemp[i].l;
					code[cx++].a=codeTemp[i].a;
				}
				gen(JMP,0,ENext);
				for(int i=breakCx[breakLevel][0];i>0;i--)
				{
					code[i].a = cx;
				}
				free(breakCx[breakLevel]);
				breakLevel--;
				loopLevel--;
				
				list *p = falseList_1->next;
				while(p != NULL)
				{
					code[p->cx].l = cx;
					p=p->next;
				}
			}
		}
	}

	else if(sym == SYM_CONTINUE)
	{
		if(loopLevel == 0)
		{
			printf("Not in a loop !\n");
			exit(1);
		}
		gen(JMP,0,loopCx[loopLevel]);
		getsym();
		if(sym != SYM_SEMICOLON)
		{
			printf("expect ; in continue while sym is %d\n",sym);
			exit(1);
		}
		else getsym();
	}
	
	else if(sym == SYM_BREAK)
	{
		if(breakLevel == 0)
		{
			printf("Not in a Break!\n");
			exit(1);
		}
		breakCx[breakLevel][0]++;
		int count = breakCx[breakLevel][0];
		breakCx[breakLevel][count]=cx;
		gen(JMP,0,0);
		getsym();
		if(sym != SYM_SEMICOLON)
		{
			printf("expected ; in break while sym is %d\n",sym);
			exit(1);
		}
		getsym();
	}

	else if (sym == SYM_IF)
	{ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		int cxTemp;
		list *trueList_1 = (list *)malloc(sizeof(list));
		trueList_1->next = NULL;
		trueList_1->tail = trueList_1;
		list *falseList_1 = (list *)malloc(sizeof(list));
		falseList_1->next = NULL;
		falseList_1->tail = falseList_1;
		if(sym != SYM_LPAREN)
		{
			printf("expect ( after if \n");
			err++;
		}
		else
		{
			getsym(); 
			short_condition_or(set,trueList_1,falseList_1);
			
			if(sym != SYM_RPAREN)
			{
				printf("expect ) in if expression while the sym is %d \n",sym);
				err++;
			}
			else getsym();
			
		}
		destroyset(set1);
		destroyset(set);

		list *p = trueList_1->next;
		while(p != NULL)
		{
			code[p->cx].a = cx;
			p=p->next;
		}
		statement(fsys);
		cxTemp=cx;
		gen(JMP,0,0);

		if(sym == SYM_ELSE)
		{
			getsym();
			
			list *p = falseList_1->next;
			while(p != NULL)
			{
				code[p->cx].l = cx;
				p=p->next;
			}
			statement(fsys);
			code[cxTemp].a=cx;
		}
		else
		{
			list *p = falseList_1->next;
			while(p != NULL)
			{
				code[p->cx].l = cx;
				p=p->next;
			}
			code[cxTemp].a=cx;
		}
	}
	else if (sym == SYM_BEGIN)
	{ // block
		getsym();
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
			// printf("sym in end is %d",sym);
			error(17); // ';' or 'end' expected.
		}
		if(sym != SYM_SEMICOLON && sym != SYM_PERIOD)
		{
			error(17);
		}
		else if(sym == SYM_SEMICOLON)getsym();
	}
	else if (sym == SYM_WHILE)
	{ // while statement
		breakLevel++;
		breakCx[breakLevel] = (int *)malloc(50*sizeof(int));
		breakCx[breakLevel][0] = 0;
		loopLevel++;
		getsym();
		list *trueList = (list*)malloc(sizeof(list));
		list *falseList = (list *)malloc(sizeof(list));
		trueList->next = NULL;
		trueList->tail = trueList;
		falseList->next = NULL;
		falseList->tail = falseList;
		int cxTemp;

		if(sym != SYM_LPAREN)
		{
			printf("expected ( in while but the sym is %d\n",sym);
			exit(1);
		}
		getsym();

		cxTemp = cx;
		loopCx[loopLevel]=cx;
		short_condition_or(fsys,trueList,falseList);
		if(sym != SYM_RPAREN)
		{
			printf("expected ) in while but the sym is %d\n",sym);
			exit(1);
		}
		getsym();
		list *p = trueList->next;
		while(p != NULL)
		{
			code[p->cx].a = cx;
			p=p->next;
		}

		statement(fsys);
		gen(JMP,0,cxTemp);

		p = falseList->next;
		while(p != NULL)
		{
			code[p->cx].l = cx;
			p=p->next;
		}
		loopLevel--;
		for(int i=breakCx[breakLevel][0];i>0;i--)
		{
			code[i].a = cx;
		}
		free(breakCx[breakLevel]);
		breakLevel--;
	}
	// test(fsys, phi, 19);
} // statement

void paraList()
{
	if(presym == SYM_LPAREN)
	{
		if(sym == SYM_IDENTIFIER)
		{
			char idTemp[MAXIDLEN + 1]; 
			strcpy(idTemp, id);
			getsym();
			paraList();
			enterPara(idTemp,ID_VARIABLE);
		}
		else if(sym == SYM_RPAREN)
		{
			getsym();
			return;
		}
		else
		{
			printf("error in paraList 1\n");
			error(26);
		}
	}
	else if(presym == SYM_COMMA)
	{
		if(sym == SYM_IDENTIFIER)
		{
			char idTemp[MAXIDLEN + 1]; 
			strcpy(idTemp, id);
			getsym();
			paraList();
			enterPara(idTemp,ID_VARIABLE);
		}
		else if(sym == SYM_TIMES)
		{
			getsym();
			if(sym == SYM_IDENTIFIER)
			{
				char idTemp[MAXIDLEN+1];
				strcpy(idTemp,id);
				getsym();
				paraList();
				enterPara(idTemp,ID_POINTER);
			}
			else
			{
				printf("expected identifier in paraList\n");
				exit(1);
			}
		}
		else
		{
			printf("error in paraList 2\n");
			error(26);
		}
	}
	else if(presym == SYM_IDENTIFIER)
	{
		if(sym == SYM_RPAREN)
		{
			getsym();
			return;
		}
		else if(sym == SYM_COMMA)
		{
			getsym();
			paraList();
		}
		else
		{
			printf("error in paraList 3\n");
			error(26);
		}
	}
	else
	{
		printf("error in paraList 4\n");
		error(26);
	}
}		

void arrayDecl()
{
	if(presym == SYM_LEFTSPAREN)
	{
		if(sym != SYM_NUMBER)
		{
			printf("expected number when declare array.\n");
			err++;
			return;
		}
		else 	// sym == SYM_LEFTSPAREN
		{
			arrayDim[adx]++;
			arrayDim[adx+arrayDim[adx]]=num;
			getsym();
		}
		if(sym != SYM_RIGHTSPAREN)
		{
			printf("expected rightsparen here while declare array at dim %d\n",arrayDim[adx]);
			err++;
			return;
		}
		else
		{
			getsym();
			arrayDecl();
		}
	}
	else if(presym == SYM_RIGHTSPAREN)
	{
		if(sym == SYM_LEFTSPAREN)
		{
			getsym();
			arrayDecl();
		}
		else // ; or ,
		{
			int count=1;
			for(int i=1;i<=arrayDim[adx];i++)
			{
				count*=arrayDim[adx+i];
			}
			dx+=count-1;
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

//////////////////////////////////////////////////////////////////////
void block(symset fsys)
{
	int cx0; // initial code index
	mask* mk;
	int block_dx;
	int savedTx;
	symset set1, set;

	dx = 3;
	block_dx = dx;
	mk = (mask*) &table[tx];
	mk->address = cx;
	gen(JMP, 0, 0);
	if (level > MAXLEVEL)
	{
		error(32); // There are too many levels.
	}
	if(sym == SYM_LPAREN)
	{
		getsym();
		paraList();
		
		enterPara("return",ID_RETURN);
		// mask *mkTemp;
		// for(int i=1;i<=tx;i++)
		// {
		// 	mkTemp=(mask *)&table[i];
		// 	printf("name = %s, level = %d, address = %d\n",table[i].name,mkTemp->level,(int)(mkTemp->address));
		// }
	}
	do
	{
		if(sym == SYM_ARRAY)
		{
			getsym();
			if(sym == SYM_IDENTIFIER)//SYM_LEFTSPAREN,SYM_RIGHTSPAREN,
			{                            
				enterArray();
				getsym();
				if(sym == SYM_LEFTSPAREN)
				{
					getsym();
					arrayDecl();
				}
			}
		}
		if (sym == SYM_CONST)
		{ // constant declarations
			do
			{
				getsym();
				constdeclaration();   
				while (sym == SYM_COMMA)
				{
					getsym();    //there may be several const vars behind const
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
	/*******10.9勘误，这里之前写的sym == SYM_IDENTIFIER,明显不对*******/
			while (sym == SYM_CONST);
		} // if

		if (sym == SYM_VAR)
		{ // variable declarations
			do
			{
				getsym();
				vardeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					vardeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
	/*******10.9勘误，这里之前写的sym == SYM_IDENTIFIER,明显不对*******/
			while (sym == SYM_VAR);
		} // if
		block_dx = dx; //save dx before handling procedure call!
		while (sym == SYM_PROCEDURE)
		{ // procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER)
			{
				enter(ID_PROCEDURE);
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}


			level++;
			savedTx = tx;
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
			block(set);
			destroyset(set1);
			destroyset(set);
			tx = savedTx;
			level--;

			// if (sym == SYM_SEMICOLON)
			// {
			// 	getsym();
			// 	set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
			// 	set = uniteset(statbegsys, set1);
			// 	// test(set, fsys, 6); 
			// 	destroyset(set1);
			// 	destroyset(set);
			// }
			// else
			// {
			// 	error(5); // Missing ',' or ';'.
			// }
		} // while
		dx = block_dx; //restore dx after handling procedure call!
		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);
		// test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}
	while (inset(sym, declbegsys));

	code[mk->address].a = cx;
	mk->address = cx;
	cx0 = cx;
	gen(INT, 0, block_dx);
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	// test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	// listcode(cx0, cx);
} // block

//////////////////////////////////////////////////////////////////////
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;
	
	while (levelDiff--)
		b = stack[b];
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
void interpret()
{
	listcode(0,cx);
	int pc;        // program counter
	int stack[STACKSIZE];
	int top;       // top of stack
	int b;         // program, base, and top-stack register
	instruction i; // instruction register

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1;
	top = 0;
	for(int i=0;i<STACKSIZE;i++)stack[i]=0;
	stack[1] = stack[2] = stack[3] = 0;
	do
	{
		i = code[pc++];
		switch (i.f)
		{
		case LIT:	// push a count on stack top.
			stack[++top] = i.a;
			break;
		case OPR:
			switch (i.a) // operator
			{
			case OPR_RET:
				top = b - 1;
				pc = stack[top + 3]; // address of next instruction
				b = stack[top + 2]; // old base of caller
				break;
			case OPR_NEG:
				stack[top] = -stack[top];
				break;
			case OPR_ADD:
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN:
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL:
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV:
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD:
				stack[top] %= 2;
				break;
			case OPR_EQU:
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ:
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES:
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				printf("stack[top] = %d\n",stack[top]);
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			/***9.30添加↓↓↓↓*/
			case OPR_AND:
				top--;
				stack[top] = (stack[top] && stack[top + 1]);   
				break;
			case OPR_OR:
				top--;
				stack[top] = (stack[top] || stack[top + 1]);
				break;
			case OPR_NOT:
				stack[top] = !stack[top];
				break;
			case OPR_MOD:
				top--;
				stack[top] = stack[top] % stack[top + 1];
				break;
			case OPR_XOR:
				top--;
				stack[top] = stack[top] ^ stack[top +1];
				break;
			case OPR_ANDBIT:
				top--;
				stack[top] = (stack[top] & stack[top + 1]);
				break;
			} // switch
			break;
		case LOD:
			stack[++top] = stack[base(stack, b, i.l) + i.a]; // push var on stack
			break;
		case STO: // store var on stack
			stack[base(stack, b, i.l) + i.a] = stack[top];
			// for(int k=0;k<20;k++)printf("%-3d ",stack[k]);
			// printf("\n");
			printf("%d\n",stack[top]);
			// printf("the pc now is %d\n",pc);
			top--;
			break;
		case CAL: 
			stack[top + 1] = base(stack, b, i.l);
			// generate new block mark
			stack[top + 2] = b;
			stack[top + 3] = pc;
			// printf("the pc is %d *****&&&&&&&&&&&&&&**\n",pc);
			b = top + 1;
			pc = i.a;
			break;
		case INT:
			top += i.a;
			break;
		case JMP:
			pc = i.a;
			break;
		case JZ:
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		case JZS:
			if(stack[top] == 0)
				pc = i.a;
			break;
		case JNZS:
			if(stack[top] != 0)
				pc =i.a;
			break;
		case JNZ:
			if (stack[top] != 0)
				pc = i.a;
			top--;
			break;
		case RET:
			stack[b+i.a]=stack[b-1];
			pc = stack[b + 2];
			int bTemp=b;
			b = stack[b + 1];
			top=bTemp+i.a;
			break;
		case LODARR:
			stack[top] = stack[base(stack, b, i.l) + i.a + stack[top]];
			// for(int k=0;k<20;k++)printf("%-3d ",stack[k]);
			break;
		case STOARR:
			stack[base(stack, b, i.l) + i.a + stack[top-1]] = stack[top];
			// for(int k=0;k<20;k++)printf("%-3d ",stack[k]);
			// printf("\n");
			printf("%d\n",stack[top]);
			top-=2;
			break;
		/******************1117***********************************/
		case JE:
			if(stack[top-1] == stack[top])
				pc=i.a;
			else pc = i.l;
			top-=2;
			break;
		case JNE:
			if(stack[top-1] != stack[top])
				pc=i.a;
			else pc = i.l;
			top-=2;
			break;
		case JG:
			if(stack[top-1] > stack[top])
				pc=i.a;
			else pc = i.l;
			top-=2;
			break;
		case JGE:
			if(stack[top-1] >= stack[top])
				pc=i.a;
			else pc = i.l;
			top-=2;
			break;
		case JL:
			if(stack[top-1] < stack[top])
				pc=i.a;
			else pc = i.l;
			top-=2;
			break;
		case JLE:
			if(stack[top-1] <= stack[top])
				pc=i.a;
			else pc = i.l;
			top-=2;
			break;
		case BAC:
			top-=i.a;
			break;
		case CPY:
			stack[top]=stack[top+i.a];
			break;
		case STOA:
			stack[stack[top-1]]=stack[top];
			printf("%d\n",stack[top]);
			top-=2;
			break;
		case LODA:
			stack[top] = stack[stack[top]];
			break;
		case LODADD:
			stack[++top] = base(stack,b,i.l)+i.a;
		} // switch
	}
	while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
int main (int argc,char *argv[])
{
	FILE* hbin;
	int i;
	symset set, set1, set2;

	if(argc != 2)
	{
		printf("The argument of program is too few or too much \n");
		printf("please input one file name \n");
		exit(1);
	}
	if ((infile = fopen(argv[1], "r")) == NULL)
	{
		printf("File %s can't be opened.\n", argv[1]);
		exit(1);
	}
	code[ENDCX].f=OPR;
	code[ENDCX].l=0;
	code[ENDCX].a=OPR_RET;

	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);
	
	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_ARRAY, SYM_NULL);
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE,SYM_RETURN, SYM_IDENTIFIER, SYM_EXIT, SYM_FOR, SYM_CONTINUE, SYM_BREAK,SYM_SWITCH,SYM_TIMES, SYM_NULL);
	/*************************9.30添加下面的SYM_NOT****************************/
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_NOT,SYM_ANDBIT,SYM_TIMES, SYM_NULL);

	err = cc = cx = ll = 0; // initialize global variables
	ch = ' ';
	kk = MAXIDLEN;

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL); // end set
	set2 = uniteset(declbegsys, statbegsys); // begin set
	set = uniteset(set1, set2);
	block(set);
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)
		error(9); // '.' expected.
	if (err == 0)
	{
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
			fwrite(&code[i], sizeof(instruction), 1, hbin);
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	// listcode(0, cx);
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
