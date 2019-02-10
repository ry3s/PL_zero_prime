/* main.c */

#include <stdio.h>
#include <stdlib.h>
#include "getSource.h"

int main(void) {
    char fileName[30];          /* ソースプログラムファイルの名前 */
    printf("enter source file name\n");
    scanf("%s", fileName);
    if (!openSource(fileName)) {
        exit(1);                /* 失敗したら終わり */
    }

    if (compile()) {
        execute();              /* エラーが無ければ実行 */
    }

    closeSource();              /* ソースプログラムファイルのclose */
    return 0;
}
