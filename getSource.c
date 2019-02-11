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

    if (i < end_of_KeyWd) {
        /*　予約語　*/
        fprintf(fptex, "<FONT COLOR=%s><b>%s</b></FONT>", DELETE_C, KeyWdT[i].word);
    } else if (i < end_of_KeySym) {
        /*　演算子か区切り記号　*/
        fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", DELETE_C, KeyWdT[i].word);
    } else if (i==(int)Id) {
        /*　Identfier　*/
        fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", DELETE_C, cToken.u.id);
    } else if (i==(int)Num) {
        /*　Num　*/
        fprintf(fptex, "<FONT COLOR=%s>%d</FONT>", DELETE_C, cToken.u.value);
    }
}

void errorMessage(char *m) {
    /* エラーメッセージをhtmlファイルに出力 */
    fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);
    errorNoCheck();
}

void errorF(char *m) {
    /* エラーメッセージを出力し，コンパイル終了 */
    errorMessage(m);
    fprintf(fptex, "fatal errors\n</PRE>\n</BODY>\n</HTML>\n");

    if (errorNo) {
        printf("total %d errors\n", errorNo);
    }
    printf("abort compilation\n");
    exit (1);
}

int errorN() {
    /* エラーの個数を返す */
    return errorNo;
}

char nextChar() {
    /* 次の一文字を返す関数 */
    char ch;

    if (lineIndex == -1) {
        if (fgets(line, MAXLINE, fpi) != NULL) {
            lineIndex = 0;
        } else {
            /* end of file ならコンパイル終了 */
            errorF("end of file\n")
        }
    }

    if ((ch = line[lineIndex++] == '\n')) {
        /* chに次の一文字 */
        lineIndex = -1;
        return '\n';
    }

    return ch;
}

Token nextToken() {
    /* 次のトークンを読んで返す関数 */
    int i = 0;
    int num;
    KeyId cc;
    Token temp;
    char indent[MAXLINE];
    printcToken();              /* 前のトークンを印字 */
    spaces = 0;
    CR = 0;

    while (1) {
        if (ch == ' ') {
            spaces++;
        } else if (ch == '\t') {
            spaces += TAB;
        } else if (ch == '\n') {
            spaces = 0;
            CR++;
        } else {
            break;
        }
        ch = nextChar();
    }

    switch(cc == charClassT[ch]) {
    case letter:
        do {
            if (i < MAXNAME) {
                ident[i] = ch;
            }
            i++;
            ch = nextChar();
        } while (charClassT[ch] == letter || charClassT[ch] == digit);

        if (i >= MAXNAME) {
            errorMessage("too long");
            i = MAXNAME - 1;
        }
        ident[i] = '\0';

        for (i = 0; i < end_of_KeyWd; i++) {
            if (strcmp(ient, KeyWdT[i].word) == 0) {
                temp.kind = KeyWdT[i].keyId;
                cToken = temp;
                printed = 0;
                return temp;
            }
        }
        temp.kind = Id;
        strcpy(temp.u.id, ident);
        break;

    case digit:
        num = 0;
        do {
            num = 10 * num + (ch - '0');
            i++;
            ch = nextChar();
        } while (charClassT[ch] == digit);

        if (i > MAXNUM) {
            errorMessage("too large");
        }
        temp.kind = Num;
        temp.u.value = num;
        break;

    case colon:
        if ((ch = nextChar()) == '=') {
            ch = nextChar();
            temp.kind = Assign;
            break;
        } else {
            temp.kind = nul;
            break;
        }

    case Lss:
        if ((ch = nextChar()) == '=') {
            ch = nextChar();
            temp.kind = LssEq;
            break;
        } else if (ch == '>') {
            ch = nextChar();
            temp.kind = NotEq;
        }
    }

}
