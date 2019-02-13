
/********* main.c *********/

#include <stdio.h>
#include "getSource.h"

main()
{
	char fileName[30]; /*　ソースプログラムファイルの名前　*/
	printf("enter source file name\n");
	scanf("%s", fileName);
	if (!openSource(fileName)) /*　ソースプログラムファイルのopen　*/
		return 0;			   /*　openに失敗すれば終わり　*/
	if (compile())			   /*　コンパイルして　*/
		execute();			   /*　エラーがなければ実行　*/
	closeSource();			   /*　ソースプログラムファイルのclose　*/
	return 0;
}
