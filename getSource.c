/* getSource.c */
#include <stdio.h>
#include <string.h>
#include "getSource.h"

#define MAXLINE 300             /* 1行の最大文字数 */
#define MAXERROR 30             /* これ以上のエラーがあったら終わり */
#define MAXNUM 14               /* 定数の最大桁数 */
#define TAB 5                   /* タブのスペース */
#define INSERT_C  "#0000FF"     /* 挿入文字の色 */
#define DELETE_C  "#FF0000"     /* 削除文字の色 */
#define TYPE_C  "#00FF00"       /* タイプエラー文字の色 */

static FILE *fpi;               /* ソースファイル */
static FILE *fptex;             /* LaTeXの出力ファイル */
static char line[MAXLINE];      /* 一行分の入力バッファ */
static int lineIndex;           /* 次に読む文字の位置 */
static char ch;                 /* 最後に読んだ文字 */

static Token cToken;            /* 最後に読んだトークン */
static KindT idKind;            /* 現トークンの種類 */
static int spaces;              /* そのトークンの前のスペースの個数 */
static int CR;                  /* その前のCRの個数 */
static int printed;             /* トークンは印字済みか */

static int errorNo = 0;         /* 出力したエラーの数 */
static char nextChar();         /* 次の文字を読む関数 */
static int isKeySym(KeyId k);   /* tは記号か */
static int isKeyWd(keyId k);    /* tは予約語か */
static void printSpace();       /* トークンの前のスペースの印字 */
static void printcToken();      /* トークンの印字 */

struct keyWd {
    /* 予約語や記号と名前 */
    char *word;
    KeyId keyId;
};

static struct keyWd KeyWdT[] = {
    /*　予約語や記号と名前(KeyId)の表　*/
    {"begin", Begin},
    {"end", End},
    {"if", If},
    {"then", Then},
    {"while", While},
    {"do", Do},
    {"return", Ret},
    {"function", Func},
    {"var", Var},
    {"const", Const},
    {"odd", Odd},
    {"write", Write},
    {"writeln",WriteLn},
    {"$dummy1",end_of_KeyWd},
    /*　記号と名前(KeyId)の表　*/
    {"+", Plus},
    {"-", Minus},
    {"*", Mult},
    {"/", Div},
    {"(", Lparen},
    {")", Rparen},
    {"=", Equal},
    {"<", Lss},
    {">", Gtr},
    {"<>", NotEq},
    {"<=", LssEq},
    {">=", GtrEq},
    {",", Comma},
    {".", Period},
    {";", Semicolon},
    {":=", Assign},
    {"$dummy2",end_of_KeySym}
};

int isKeyWd(KeyId k) {
    /* キーkは予約語か？ */
    return (k < end_of_KeyWd);
}

int isKeySym(KeyId k) {
    /* キーは記号か？ */
    if (k < end_of_KeyWd) {
        return 0;
    }
    return (k < end_of_KeySym);
}

static KeyId charClassT[256];   /* 文字の種類を示す表にする */

static void initCharClassT() {
    /* 文字の種類を表す表を作る関数 */
    int i;
    for (i = 0; i < 256; i++) {
        charClassT[i] = others;
    }
    for (i = '0'; i <= '9'; i++) {
        charClassT[i] = digit;
    }
    for (i = 'A'; i <= 'Z'; i++) {
        charClassT[i] = letter;
    }
    for (i = 'a'; i <= 'z'; i++) {
        charClassT[i] = letter;
    }
    charClassT['+'] = Plus;
    charClassT['-'] = Minus;
    charClassT['*'] = Mult;
    charClassT['/'] = Div;
    charClassT['('] = Lparen;
    charClassT[')'] = Rparen;
    charClassT['='] = Equal;
    charClassT['<'] = Lss;
    charClassT['>'] = Gtr;
    charClassT[','] = Comma;
    charClassT['.'] = Period;
    charClassT[';'] = Semicolon;
    charClassT[':'] = colon;
    return;
}

int openSource(char fileName[]) {
    /* ソース・ファイルのopen */
    char fileName0[30];

    if ((fpi = fopen(fileName, "r")) == NULL) {
        printf("can't open %s\n", fileName);
        return 0;
    }
    strcpy(fileNameO, fileName);
    strcat(fileNameO,".html");
    if ((fptex = fopen(fileNameO,"w")) == NULL) {
        /*　.htmlファイルを作る　*/
        printf("can't open %s\n", fileNameO);
        return 0;
    }
    return 1;
}

void closeSource() {
    /* ソースファイルとhtmlファイルをclose */
    fclose(fpi);
    fclose(fptex);
}

void initSource() {
    lineIndex = -1;             /* 初期設定 */
    ch = '\n';
    printed = 1;

    initCharClassT();

    /*　htmlコマンド　*/
    fprintf(fptex,"<HTML>\n");
    fprintf(fptex,"<HEAD>\n<TITLE>compiled source program</TITLE>\n</HEAD>\n");
    fprintf(fptex,"<BODY>\n<PRE>\n");
}

void finalSource() {
    if (cToken.kind==Period) {
        printcToken();
    } else {
        errorInsert(Period);
    }
    fprintf(fptex,"\n</PRE>\n</BODY>\n</HTML>\n");
}

void errorNoCheck() {
    /* エラーの個数のカウント，多すぎたら終わり */
    if (errorNo++ > MAXERROR) {
        fprintf(fptex, "too many errors\n</PRE>\n</BODY>\n</HTML>\n");
        printf("abort compilation\n");
        exit (1);
    }
}

void errorType(char *m) {
    /* 型エラーをhtmlファイルに出力 */
    printSpace();
    fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);
    printcToken();
    errorNoCheck();
}

void errorInsert(KeyId k) {
    /* keyString(k)をhtmlファイルに挿入 */
    fprintf(fptex, "<FONT COLOR=%s><b>%s</b></FONT>", INSERT_C, KeyWdT[k].word);
    errorNoCheck();
}

void errorMissingId() {
    /* 「名前がない」とのメッセージをhtmlファイルに挿入 */
    fprintf(fptex, "<FONT COLOR=%s>Id</FONT>", INSERT_C);
    errorNoCheck();
}

void errorMissingOp() {
    /* 「演算子がない」とのメッセージをhtmlファイルに挿入 */
    fprintf(fptex, "<FONT COLOR=%s>@</FONT>", INSERT_C);
    errorNoCheck();
}

void errorDelete() {
    /* 今読んだトークンを読み捨てる */
    int i = (int)cToken.kind;
    printSpace();
    printed = 1;


}
