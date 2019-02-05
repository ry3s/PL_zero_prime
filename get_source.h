#include <stdio.h>

#ifndef TBL
#define TBL
#include "table.h"
#endif

#define MAX_NAME 31 // 名前の最大の長さ

typedef enum keys {
    Begin, End,
    If, Then,
    While, Do,
    Ret, Func,
    Var, Const, Odd,
    Write, WriteLn,
    end_of_KeyWord,
    Plus, Minus,
    Mult, Div,
    Lparen, Rparen,
    Equal, Lss, Gtr,
    NotEq, LssEq, GtrEq,
    Comma, Period, Semicolon,
    Assign,
    end_of_keySymbol,
    Id, Num, nul,
    end_of_Token,
    letter, digit, colon, others
} KeyId;

typedef struct token {
    KeyId kind;
    union {
        char id[MAX_NAME];
        int value;
    } u;
} Token;

// 次のTokenを読んで返す
Token next_token();
// t.kind == k のチェック
// t.kind == k なら，次のトークンを読んで返す
// t.kind != k ならエラーメッセージを出し，tとkがともに記号，または予約語なら
//             tを捨て，次のトークンを読んで返す（tをkで置き換えたことになる）
// それ以外の場合，kを挿入したことにして，tを返す
Token check_get(Token t, KeyId k);

int open_source(char file_name[]);
void close_source();
void init_source();
void final_source();
void error_type(char *m);
void error_insert(KeyId k);
void error_misiingId();
void error_missingOp();
void error_delete();
void error_message(char *m);
void error_final(char *m);
int error_num();

void set_id_kind(KindT k);
