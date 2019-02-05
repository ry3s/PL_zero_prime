#include <stdio.h>
#include <string.h>

#include "get_source.h"

#define MAX_LINE 200
#define MAX_ERROR 30
#define MAX_NUM 14
#define TAB 5
#define INSERT_C "#0000FF"
#define DELETE_C "#FF0000"
#define TYPE_C "00FF00"

static FILE *fp_input;
static FILE *fp_tex;
static char line[MAX_LINE];
static int line_index;
static char ch;

static Token c_token;
static KindT id_kind;
static int space;
static int CR;
static int printed;

static int error_num = 0;
static char next_char();
static int is_key_symbol(KeyId k);
static int is_key_word(KeyId k);
static void print_spaces();
static void print_c_token();

struct KeyWord {
    char *word;
    KeyId key_id;
};

static struct KeyWord KeyWordT[] {
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

int is_key_word(KeyId k) {
    return (k < end_of_KeyWord);
}

int is_key_symbol(KeyId k) {
    if (k < end_of_KeyWord) return 0;

    return (k < end_of_keySymbol);
}

static KeyId char_class_table[256];

static vid init_char_class_table() {
    int i;

    for (i = 0; i < 256; i++) char_class_table[i] = others;
    for (i = '0'; i <= '9'; i++) char_class_table[i] = digit;
    for (i = 'A'; i <= 'Z'; i++) char_class_table[i] = letter;
    for (i = 'a'; i <= 'z'; i++) char_class_table[i] = letter;

    char_class_table['+'] = Plus;
    char_class_table['-'] = Minus;
    char_class_table['*'] = Mult;
    char_class_table['/'] = Div;
    char_class_table['('] = Lparen;
    char_class_table[')'] = Rparen;
    char_class_table['='] = Equal;
    char_class_table['<'] = Lss;
    char_class_table['>'] = Gtr;
    char_class_table['.'] = Period;
    char_class_table[';'] = Semicolon;
    char_class_table[':'] = colon;
}

int open_source(char file_name[]) {

    char file_name0[30];

    if ((fp_input = fopen(file_name, "r")) == NULL) {
        printf("can't open %s\n", file_name);
        return 0;
    }

    strcpy(file_name0, file_name);
    strcat(file_name0, ".html");
    if ((fp_tex = fopen(file_name0, "w")) == NULL) {
        printf("can't open %s\n", file_name0);
        return 0;
    }
    return 1;
}

void close_source() {

    fclose(fp_input);
    fclose(fp_tex);
}

void init_source() {
    line_index = -1;
    ch = '\n';
    is_printed = 1;

    init_char_class_table();

    fprintf(fp_tex, "<HTML>\n");
    fprintf(fp_tex, "<HEAD>\n<TITLE>compiled source program</TITLE>\n</HEAD>\n");
    fprintf(fp_tex,"<BODY>\n<PRE>\n");
}

void final_source() {

    if (c_token.kind == Period) print_c_token();
    else error_insert(Period);

    fprintf(fp_tex, "\n</PRE>\n</BODY>\n</HTML>\n");
}

void error_no_check() {
    if (error_num++ > MAX_ERROR) {
        fprintf(fp_tex, "too many errors\n</PRE>\n</BODY>\n</HTML>\n");

        printf("abort compilation\n");
        exit(1);
    }
}

void error_type(char *m) {
    print_spaces();
    fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);

    print_token();
    error_no_check();
}

void error_insert(KeyId k) {
    fprintf(fptex, "<FONT COLOR=%s><b>%s</b></FONT>", INSERT_C, KeyWordT[k].word);

    error_no_check();
}

void error_missing_id() {
    fprintf(fptex, "<FONT COLOR=%s>Id</FONT>", INSERT_C);

    error_no_check();
}

void error_missing_op() {
    fprintf(fptex, "<FONT COLOR=%s>@</FONT>", INSERT_C);

    error_no_check();
}

void error_delete() {
    int i = (int)c_token.kind;
    print_spaces();
    printed = 1;

    if (i < end_of_KeyWord) {
        fprintf(fptex, "<FONT COLOR=%s><b>%s</b></FONT>", DELETE_C, KeyWordT[i].word);
    } else if (i < end_of_keySymbol) {
        fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", DELETE_C, KeyWordT[i].word);
    } else if (i == (int)Id) {
        fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", DELETE_C, c_token.u.id);
    } else if (i == (int)Num) {
        fprintf(fptex, "<FONT COLOR=%s>%d</FONT>", DELETE_C, c_token.u.value);
    }
}

void error_message(char *m) {
    fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);

    error_no_check();
}

void error_final(char *m) {
    error_message(m);
    fprintf(fp_tex, "fatal errors\n</PRE>\n</BODY>\n</HTML>\n");

    if (error_num) printf("total %d errors\n", error_num);

    printf("abort compilation\n");
    exit(1);
}

int error_num() {
    return error_num;
}

char next_char() {
    char ch;

    if (line_index == -1) {
        if (fgets(line, MAX_LINE, fp_input) != NULL) {
            line_index = 0;
        } else {
            error_final("end_of_file\n");
        }
    }

    if ((ch = line[line_index++]) == '\n') {
        line_index = -1;
        return '\n';
    }
    return ch;
}

Token next_token() {

    int i = 0;
    int num;
    keyId cc;
    char ident[MAX_NAME];
    print_c_token();
    spaces = 0;
    CR = 0;

    while (1) {
        if (ch == ' ') {
            spaces++;
        } else if (ch == '\t') {
            space += TAB;
        } else if (ch == '\n') {
            space = 0;
            CR++;
        } else {
            break;
        }

        ch = next_char();
    }

    switch(cc = char_class_table[ch]) {
    case letter:
    }
}
