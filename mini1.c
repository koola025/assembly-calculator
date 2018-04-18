#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLEN 256
#define TBLSIZE 65535

typedef enum {UNKNOWN, END, INT, ID, ADDSUB, MULDIV, ASSIGN, LPAREN, RPAREN, ENDFILE, REG} TokenSet;
typedef enum {MISPAREN, NOTNUMID, NOTFOUND, RUNOUT} ErrorType;

typedef struct {
    char Addr[5];
    char name[5];
    int val;
    int init;
} Symbol;

typedef struct _res {
    char s[50];
    int line;
    struct _res *next;
} Result;

typedef struct _line {
    int num;
    int master[3];
    int independent;
    struct _line *next;
} Line;

typedef struct _Node {
    char lexeme[MAXLEN];
    TokenSet data;
    int val;
    int n;
    struct _Node *left, *right;
} BTNode;

typedef struct _reg {
    char Addr[5];
    char name[5];
    int val;
    int n;
    int isUsed;
    int isConst;
} Reg;

BTNode* expr(void);
void evaluateTree(BTNode *root);

static TokenSet lookahead = UNKNOWN;
static char lexeme[MAXLEN];
Symbol table[3] = {{"[0]", "x", 0, 0},
                   {"[4]", "y", 0, 0},
                   {"[8]", "z", 0, 0}};
int sbcount = 0;
int count = 0;  
int behind;
int already;
//int xLast, yLast, zLast;
int last[3] = {0};
int asked[3] = {0};
Result *head, *nr, *nrHead;
Line *lineHead, *nl;
Reg r[20];
Reg *toAssign;


void newRes(void) {
    nr->next = (Result*)malloc(sizeof(Result));
    nr = nr->next;
    nr->next = NULL;
    
}

void newLine(void) {
    nl->next = (Line*)malloc(sizeof(Line));
    nl = nl->next;
    nl->next = NULL;
}

int findUnused(void) {   //找還沒用過的r
    //printf("findUnused\n");
    int i;
    for(i = 3; i < 20; i++) { //直接從r3開始找
        if(!r[i].isUsed) return i;
    }
    return 99;
}

char* getLexeme(void)
{
    return lexeme;
}
void error(ErrorType errorNum)
{
    printf("EXIT 1\n");
    /*switch (errorNum) {
    case MISPAREN:
        fprintf(stderr, "Mismatched parenthesis\n");
        break;
    case NOTNUMID:
        fprintf(stderr, "Number or identifier expected\n");
        break;
    case NOTFOUND:
        fprintf(stderr, "%s not defined\n", getLexeme());
        break;
    case RUNOUT:
        fprintf(stderr, "Out of memory\n");
    }*/
    exit(0);
}



TokenSet getToken(void)
{
    int i;
    char c;

    while ( (c = fgetc(stdin)) == ' ' || c== '\t' );  // �����ťզr��

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i<MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return ADDSUB;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        if (isalpha(c) || isdigit(c) || c == '_') {
            error(0);
            /*lexeme[i] = c;
            ++i;
            c = fgetc(stdin);*/
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void)
{
    lookahead = getToken();
}

int match(TokenSet token)
{
    if (lookahead == UNKNOWN) advance();
    return token == lookahead;
}







int getval(void)
{
    int i, retval, found;

    if (match(INT)) {
        retval = atoi(getLexeme());
    } else if (match(ID)) {
        i = 0; retval = 0; //found = 0;
        while (i < 3) {
            if (strcmp(getLexeme(), table[i].name) == 0) {
                /*if (!behind) {

                }
                else {*/
                    //if(table[i].init) 
                    retval = table[i].val;
                    /*else {
                        table[i].init = 1;
                        sprintf(nr->s, "MOV %s %s\n", r[i].name, table[i].Addr);
                        nr->line = count;
                        newRes();
                    }
                }
                
                */
                //found = 1;
                break;
            } else i++;
        }
        /*if (!found) {
            if (sbcount < TBLSIZE) {
                strcpy(table[sbcount].name, getLexeme());
                table[sbcount].val = 0;
                sbcount++;
            } else error(RUNOUT);
        }*/
    }
    return retval;
}

int setval(char *str, int val)
{
    int i, retval;
    i = 0;
    while (i < sbcount) {
        if (strcmp(str, table[i].name)==0) {
            table[i].val = val; //記錄x, y, z的值
            retval = val;
            break;
        } else {
            i++;
        }
    }
    return retval;
}

/* create a node without any child.*/
BTNode* makeNode(TokenSet tok, const char *lexe){
    BTNode *node = (BTNode*) malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}



/* clean a tree.*/
void freeTree(BTNode *root){
    if (root!=NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}
/*factor := INT | ADDSUB INT | ADDSUB ID | ID ASSIGN expr | ID | LPAREN expr RPAREN*/
BTNode* factor(void)
{
    BTNode* retp = NULL;
    char tmpstr[MAXLEN];

    if (match(INT)) {
        retp =  makeNode(INT, getLexeme());
        retp->val = getval();
        already = 1;
        advance();
    } else if (match(ID)) {
        BTNode* left = makeNode(ID, getLexeme());
        left->val = getval();
        strcpy(tmpstr, getLexeme());
        advance();
        if (match(ID)) error(0);
        else if (match(ASSIGN)) {
            if (already) error(0);
            behind = 1;
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->right = expr();
            retp->left = left;

        } else {
            retp = left;
        }
    } else if (match(ADDSUB)) {
        already = 1;
        strcpy(tmpstr, getLexeme());
        advance();
        if (match(ID) || match(INT)) {
            retp = makeNode(ADDSUB, tmpstr);
            if (match(ID))
                retp->right = makeNode(ID, getLexeme());
            else
                retp->right = makeNode(INT, getLexeme());
            retp->right->val = getval();
            retp->left = makeNode(INT, "0");
            retp->left->val = 0;
            advance();
        } else {
            error(NOTNUMID);
        }
    } else if (match(LPAREN)) {
        already = 1;
        advance();
        retp = expr();
        if (match(RPAREN)) {
            advance();
        } else {
            error(MISPAREN);
        }
    } else {
        error(NOTNUMID);
    }
    return retp;
}

/*  term        := factor term_tail
    term_tail := MULDIV factor term_tail | NIL*/
BTNode* term(void)
{
    BTNode *retp, *left;
    retp = left = factor();

    while (match(MULDIV)) {
        already = 1;
        retp = makeNode(MULDIV, getLexeme());
        advance();
        retp->right = factor();
        retp->left = left;
        left = retp;
    }
    //這樣算錯ㄇ
    if (match(ID) || match(INT)) error(0);
    return retp;
}

/*  expr        := term expr_tail
  expr_tail   := ADDSUB term expr_tail | NIL*/
BTNode* expr(void)
{
    BTNode *retp, *left;
    //int retval;
    retp = left = term();
    while (match(ADDSUB)) {
        already = 1;
        retp = makeNode(ADDSUB, getLexeme());
        advance();
        retp->right = term();
        retp->left = left;
        left = retp;
    }
    //這樣算錯ㄇ
    if (match(ID) || match(INT)) error(0);
    return retp;
}

/* print a tree by pre-order. */
void printPrefix(BTNode *root)
{
    if (root != NULL)
    {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

void printList(void)
{   
    Result *tmp;
    tmp = head;
    while (tmp != NULL)
    {   
        if ((tmp->line == last[0] || tmp->line == last[1] || tmp->line == last[2]) && tmp->next != NULL)
        printf("%s", tmp->s);
        tmp = tmp->next;
    }
}

void reArrange(BTNode *root) {
    
    BTNode *a, *b;
    if(root->left) reArrange(root->left);
    if(root->right) reArrange(root->right);
    if (root->data == INT || root->data == ID) return;
    if (strcmp(root->lexeme, root->left->lexeme) != 0 && strcmp(root->lexeme, root->right->lexeme) != 0) return;
    //while(root->data == ASSIGN) root = root->right;
    if (strcmp(root->lexeme, "+") == 0) {
        if(strcmp(root->left->lexeme, "+") == 0) {
            if (root->right->data == INT) {
                if(root->left->left->data != INT && root->left->right->data == INT) {
                    //printf("he\n");
                    root->right->val = root->right->val + root->left->right->val;
                    a = root->left;
                    root->left = root->left->left;
                    a->left = NULL;
                    b = a->right;
                    a->right = NULL;
                    free(b);
                    free(a);
                }
                else if(root->left->right->data != INT && root->left->left->data == INT) {
                    //printf("she\n");
                    root->right->val = root->right->val + root->left->left->val;
                    a = root->left;
                    root->left = root->left->right;
                    a->right = NULL;
                    b = a->left;
                    a->left = NULL;
                    free(b);
                    free(a);
                }
            }
        }
        else if(strcmp(root->right->lexeme, "+") == 0) {
            if (root->left->data == INT) {
                if(root->right->left->data != INT && root->right->right->data == INT) {
                    //printf("hee\n");
                    root->left->val = root->left->val + root->right->right->val;
                    a = root->right;
                    root->right = root->right->left;
                    a->left = NULL;
                    b = a->right;
                    a->right = NULL;
                    free(b);
                    free(a);
                }
                else if(root->right->right->data != INT && root->right->left->data == INT) {
                    root->left->val = root->left->val + root->right->left->val;
                    //printf("hey %d\n", root->left->val);
                    //printf("shee\n");
                    a = root->right;
                    root->right = root->right->right;
                    a->right = NULL;
                    b = a->left;
                    a->left = NULL;
                    free(b);
                    free(a);
                }
            }
            
        }
        
        
    }
    
    

}

void freeList(Result *tar) {
    Result *tmp, *n;
    tmp = tar;
    //printf("!!!\n");
    while(tmp != NULL) {
        n = tmp;
        tmp = tmp->next;
        free(n);
    }
}

void treeToList(BTNode *root) {
    int equal = 0;
    while (root->data == ASSIGN) {
        equal++;
        //nl->num = count;
        for(int i = 0; i < 3; i++) {
            if(strcmp(root->left->lexeme, table[i].name) == 0) {
                nl->master[i] = 1;
                break;
            }
        }
        root = root->right;
        //init assign master
    }
    if (root->left) treeToList(root->left);
    if (root->right) treeToList(root->right);
    if (root->data == INT) {
        //printf("int here\n");
        int n = findUnused();
        /*sprintf(nr->s, "MOV %s %d\n", r[n].name, root->val);
        nr->line = count;
        newRes();*/
        r[n].isConst = 1;
        r[n].isUsed = 1;
        r[n].val = root->val;
        root->data = REG;
        root->n = n;
    }
    else if (root->data == ID) {
        for(int i = 0; i < 3; i++) {
            if(strcmp(root->lexeme, table[i].name) == 0) {
                if(!table[i].init) {
                    table[i].init = 1;
                    asked[i] = count;
                    sprintf(nr->s, "MOV %s %s\n", r[i].name, table[i].Addr);
                    nr->line = count;
                    newRes();
                }
                root->data = REG;
                root->n = i;
                break;
            }
        }
    }
    else if (root->data == MULDIV || root->data == ADDSUB) {
        //用完要釋出reg
        if (strcmp(root->lexeme, "+") == 0 || strcmp(root->lexeme, "*") == 0) {
            if(strcmp(root->lexeme, "*") == 0 && (r[root->left->n].isConst && root->left->val == 1)) {
                root->n = root->right->n;
                root->data = REG;
                root->val = root->right->val;
                if (root->left->n > 2) {
                    r[root->left->n].isUsed = 0;
                    r[root->left->n].isConst = 0;
                    r[root->left->n].val = 0;
                }
            }
            else if(strcmp(root->lexeme, "*") == 0 && (r[root->right->n].isConst && root->right->val == 1)) {
                root->n = root->left->n;
                root->data = REG;
                root->val = root->left->val;
                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }

            }
            else if (strcmp(root->lexeme, "*") == 0 && (r[root->right->n].isConst && root->right->val == 0)) {
                root->n = root->left->n;
                r[root->left->n].val = 0;
                r[root->left->n].isConst = 1;
                root->data = REG;
                root->val = 0;
                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }

            }
            else if (strcmp(root->lexeme, "*") == 0 && (r[root->left->n].isConst && root->left->val == 0)) {
                root->n = root->right->n;
                r[root->right->n].val = 0;
                r[root->right->n].isConst = 1;
                root->data = REG;
                root->val = 0;
                if (root->left->n > 2) {
                    r[root->left->n].isUsed = 0;
                    r[root->left->n].isConst = 0;
                    r[root->left->n].val = 0;
                }

            }
            else if (strcmp(root->lexeme, "+") == 0 && (r[root->right->n].isConst && root->right->val == 0)) {
                root->n = root->left->n;
                root->data = REG;
                root->val = root->left->val;
                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }

            }
            else if (strcmp(root->lexeme, "+") == 0 && (r[root->left->n].isConst && root->left->val == 0)) {
                root->n = root->right->n;
                root->data = REG;
                root->val = root->right->val;
                if (root->left->n > 2) {
                    r[root->left->n].isUsed = 0;
                    r[root->left->n].isConst = 0;
                    r[root->left->n].val = 0;
                }

            }
            else if (root->left->n <= 2 && root->right->n <= 2) {
                int n = findUnused();
                if (!r[root->left->n].isConst || !r[root->right->n].isConst) {
                    sprintf(nr->s, "MOV %s %s\n", r[n].name, r[root->left->n].name);
                    nr->line = count;
                    newRes();
                    if (strcmp(root->lexeme, "+") == 0)
                        sprintf(nr->s, "ADD %s %s\n", r[n].name, r[root->right->n].name);
                    if (strcmp(root->lexeme, "*") == 0)
                        sprintf(nr->s, "MUL %s %s\n", r[n].name, r[root->right->n].name);
                    nr->line = count;
                    newRes();


                }
                r[n].isConst = r[root->left->n].isConst && r[root->right->n].isConst;
                r[n].isUsed = 1;
                if (strcmp(root->lexeme, "+") == 0)
                    r[n].val = root->left->val + root->right->val;
                if (strcmp(root->lexeme, "*") == 0)
                    r[n].val = root->left->val * root->right->val;
                root->data = REG;
                root->val = r[n].val;
                root->n = n;
            }
            else if (root->left->n > 2) {
                if (!r[root->left->n].isConst || !r[root->right->n].isConst) {
                    if (r[root->left->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->left->n].name, root->left->val);
                        nr->line = count;
                        newRes();
                    }
                    if (r[root->right->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->right->n].name, root->right->val);
                        nr->line = count;
                        newRes();
                    }
                
                    if (strcmp(root->lexeme, "+") == 0)
                        sprintf(nr->s, "ADD %s %s\n", r[root->left->n].name, r[root->right->n].name);
                    if (strcmp(root->lexeme, "*") == 0)
                        sprintf(nr->s, "MUL %s %s\n", r[root->left->n].name, r[root->right->n].name);
                    nr->line = count;
                    newRes();
                }

                root->data = REG;
                if (strcmp(root->lexeme, "+") == 0)
                    root->val = r[root->left->n].val + r[root->right->n].val;
                if (strcmp(root->lexeme, "*") == 0)
                    root->val = r[root->left->n].val * r[root->right->n].val;
                
                root->n = root->left->n;
                r[root->n].val = root->val;
                r[root->n].isConst = r[root->left->n].isConst && r[root->right->n].isConst;

                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }
            }
            else if (root->right->n > 2) {
                if (!r[root->left->n].isConst || !r[root->right->n].isConst) {
                    if (r[root->left->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->left->n].name, root->left->val);
                        nr->line = count;
                        newRes();
                    }
                    if (r[root->right->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->right->n].name, root->right->val);
                        nr->line = count;
                        newRes();
                    }
                    if (strcmp(root->lexeme, "+") == 0)
                        sprintf(nr->s, "ADD %s %s\n", r[root->right->n].name, r[root->left->n].name);
                    if (strcmp(root->lexeme, "*") == 0)
                        sprintf(nr->s, "MUL %s %s\n", r[root->right->n].name, r[root->left->n].name);
                    nr->line = count;
                    newRes();
                }
                

                /*if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }*/
                root->n = root->right->n;
                root->data = REG;
                if (strcmp(root->lexeme, "+") == 0)
                    root->val = r[root->left->n].val + r[root->right->n].val;
                if (strcmp(root->lexeme, "*") == 0)
                    root->val = r[root->left->n].val * r[root->right->n].val;
                //printf("wht %d %d\n",root->val, r[root->left->n].val);
                r[root->n].val = root->val;
                r[root->n].isConst = r[root->left->n].isConst && r[root->right->n].isConst;
            }
        }
        else if (strcmp(root->lexeme, "-") == 0 || strcmp(root->lexeme, "/") == 0) {
            //printf("%s %s\n" ,root->left->lexeme, root->left->lexeme);
            //printf("plzz\n");
            if (strcmp(root->lexeme, "/") == 0 && root->right->val == 0 && r[root->right->n].isConst == 1){
                printf("EXIT 1\n");
                exit(0);
            }
            else if (strcmp(root->lexeme, "/") == 0 && r[root->right->n].isConst && root->right->val == 1) {
                root->n = root->left->n;
                root->data = REG;
                root->val = root->left->val;
                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }
            }
            else if (strcmp(root->lexeme, "-") == 0 && r[root->right->n].isConst && root->right->val == 0) {
                root->n = root->left->n;
                root->data = REG;
                root->val = root->left->val;
                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }
            }
            else if (r[root->left->n].isConst && r[root->right->n].isConst && root->left->val == root->right->val && root->left->n > 2) {
                //printf("ga\n");
                if (strcmp(root->lexeme, "-") == 0) root->val = 0;
                if (strcmp(root->lexeme, "/") == 0) root->val = 1;
                root->n = root->left->n;
                r[root->n].val = root->val;
                r[root->n].isConst = 1;
                root->data = REG;
            }
            else if (r[root->left->n].isConst && r[root->right->n].isConst && root->left->val == root->right->val) {
                //printf("gaaa\n");
                int n = findUnused();
                if (strcmp(root->lexeme, "-") == 0) root->val = 0;
                if (strcmp(root->lexeme, "/") == 0) root->val = 1;
                root->data = REG;
                r[n].isConst = 1;
                r[n].isUsed = 1;
                r[n].val = root->val;
                root->n = n;
            }
            else if (root->left->n > 2) {
                //printf("plz\n");
                if (!r[root->left->n].isConst || !r[root->right->n].isConst) {
                    if (r[root->left->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->left->n].name, root->left->val);
                        nr->line = count;
                        newRes();
                    }
                    if (r[root->right->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->right->n].name, root->right->val);
                        nr->line = count;
                        newRes();
                    }
                
                    if (strcmp(root->lexeme, "-") == 0)
                        sprintf(nr->s, "SUB %s %s\n", r[root->left->n].name, r[root->right->n].name);
                    if (strcmp(root->lexeme, "/") == 0)
                        sprintf(nr->s, "DIV %s %s\n", r[root->left->n].name, r[root->right->n].name);
                    nr->line = count;
                    newRes();
                }

                root->data = REG;
                if (strcmp(root->lexeme, "-") == 0)
                    root->val = r[root->left->n].val - r[root->right->n].val;
                if (strcmp(root->lexeme, "/") == 0 && r[root->right->n].isConst)
                    root->val = r[root->left->n].val / r[root->right->n].val;
                
                root->n = root->left->n;
                r[root->n].val = root->val;
                r[root->n].isConst = r[root->left->n].isConst && r[root->right->n].isConst;

                if (root->right->n > 2) {
                    r[root->right->n].isUsed = 0;
                    r[root->right->n].isConst = 0;
                    r[root->right->n].val = 0;
                }
            }
            else {
                int n = findUnused();
                if (!r[root->left->n].isConst || !r[root->right->n].isConst) {

                    sprintf(nr->s, "MOV %s %s\n", r[n].name, r[root->left->n].name);
                    nr->line = count;
                    newRes();
                    if (r[root->right->n].isConst) {
                        sprintf(nr->s, "MOV %s %d\n", r[root->right->n].name, root->right->val);
                        nr->line = count;
                        newRes();
                    }
                    if (strcmp(root->lexeme, "-") == 0)
                        sprintf(nr->s, "SUB %s %s\n", r[n].name, r[root->right->n].name);
                    if (strcmp(root->lexeme, "/") == 0 )
                        sprintf(nr->s, "DIV %s %s\n", r[n].name, r[root->right->n].name);
                    nr->line = count;
                    newRes();
                }
                
                r[n].isConst = r[root->left->n].isConst && r[root->right->n].isConst;
                r[n].isUsed = 1;
                if (strcmp(root->lexeme, "-") == 0)
                    r[n].val = root->left->val - root->right->val;
                if (strcmp(root->lexeme, "/") == 0 && r[root->right->n].isConst)
                    r[n].val = root->left->val / root->right->val;
                //printf("%d %d %d\n", r[n].val, root->left->val, root->right->val);
                root->data = REG;
                root->val = r[n].val;
                root->n = n;
            }

        }
        /*else if (strcmp(root->lexeme, "/") == 0) {

        }*/
    }
    
    toAssign = &r[root->n];
    
    
    

}

/*statement   := END | expr END*/
void statement(void)
{
    BTNode* retp;
    
    int preInit[3];
    if (match(ENDFILE)) {
        
        for(int i = 0; i < 3; i++) {
            //printf("last %d %d\n", i , last[i]);
            if ((last[i] == 0 && asked[i] == 0)||(asked[i]!=0 && asked[i]!= last[0] && asked[i]!= last[1] && asked[i]!= last[2]))  {
                printf("MOV r%d %s\n", i, table[i].Addr);
            }
        }
        printList();
        printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) {
        //printf(">> ");
        advance();
    } else {
        nrHead = nr;
        already = 0;                                                                                                                                                     
        retp = expr();
        if (match(END)) {
            
            count++;
            newLine();
            nl->master[0] = 0;
            nl->master[1] = 0;
            nl->master[2] = 0;
            nl->num = count;
            for(int i = 0; i < 3; i++) {
                preInit[i] = table[i].init;
            }
            evaluateTree(retp);
            reArrange(retp);
            treeToList(retp);
            if (toAssign->isConst) {
                Result *rtmp, *del;
                //printf("this %s\n",nrHead->s);
                rtmp = nrHead;
                //printf("b %s\n", nrHead->s);
                //while(rtmp->line != count) rtmp = rtmp->next;
                
                if(rtmp->next != NULL)  {
                    //printf("???\n");
                    del = rtmp->next;
                    rtmp->next = NULL;
                    freeList(del);
                }
                //else printf("ya\n");
                
                nr = rtmp;
                //newRes();
                
                for(int i = 0; i < 3; i++) {
                    if(nl->master[i] == 1) {
                        //printf("to %d\n", toAssign->val);
                        r[i].isConst = 1;
                        r[i].val = toAssign->val;
                        table[i].init = 1;
                        table[i].val = r[i].val;
                        last[i] = count;
                        sprintf(nr->s, "MOV %s %d\n", r[i].name, table[i].val);
                        nr->line = count;
                        newRes();
                    }
                    else if (preInit[i] == 0 && table[i].init == 1) {
                        
                        sprintf(nr->s, "MOV %s %s\n", r[i].name, table[i].Addr);
                        nr->line = count;
                        newRes();
                    }
                }
                //printf("a %s\n", nrHead->s);            
            }
            else {
                for(int i = 0; i < 3; i++) {
                    if(nl->master[i] == 1 && toAssign->n != i) {
                        r[i].val = toAssign->val;
                        //printf("%d %d \n", r[i].val, toAssign->val);
                        table[i].init = 1;
                        table[i].val = r[i].val;
                        last[i] = count;
                        if (toAssign->isConst) {
                            
                        } 
                        else {
                            sprintf(nr->s, "MOV %s %s\n", r[i].name, toAssign->name);
                            nr->line = count;
                            newRes();
                            r[i].isConst = 0;
                        }
                    
                    }
                }
            }
            toAssign->isUsed = 0;
            toAssign->val = 0;
            toAssign->isConst = 0;
            //printf("%d\n", );
            //printPrefix(retp); printf("\n");
            freeTree(retp);

            //printf(">> ");
            advance();
        }
    }
}


void evaluateTree(BTNode *root)
{
    int retval = 0, lv, rv;
    BTNode *tmp;
    char lexe[MAXLEN];
    
    if (root != NULL)
    {
        //printPrefix(root);

        //printf("rooot lex %s\n", root->lexeme);
        if (root->data == INT) return;
        else if (root->data == ID) {
            //compare錯 其實沒用到
            for (int i = 0; i < 3; i++) {
                //printf("rooot %s %s\n", root->lexeme, r[i].name);
                if (strcmp(root->lexeme, r[i].name) == 0) {
                    if (r[i].isConst) {
                        //printf("yo\n");
                        root->data = INT;
                        root->val = r[i].val;
                        sprintf(lexe, "%d", root->val);
                        strcpy(root->lexeme, lexe);
                    }
                    break;
                }
            }
            return;
        }
        if (root->left) evaluateTree(root->left);
        if (root->right) evaluateTree(root->right);

        if (root->left->data == INT && root->right->data == INT) {
            if (strcmp(root->lexeme, "+") == 0) {
                root->val = root->left->val + root->right->val;
                sprintf(lexe, "%d", root->val);
                strcpy(root->lexeme, lexe);
                //printf("root %d\n", root->val);
            }
                
            else if (strcmp(root->lexeme, "-") == 0) {
                root->val = root->left->val - root->right->val;
                sprintf(lexe, "%d", root->val);
                strcpy(root->lexeme, lexe);
            }
            else if (strcmp(root->lexeme, "*") == 0) {
                root->val = root->left->val * root->right->val;
                sprintf(lexe, "%d", root->val);
                strcpy(root->lexeme, lexe);
            }
            else if (strcmp(root->lexeme, "/") == 0) {
                //要補除以0的狀況
                if (root->right->val == 0){
                printf("EXIT 1\n");
                exit(0);
                }
                root->val = root->left->val / root->right->val;
                sprintf(lexe, "%d", root->val);
                strcpy(root->lexeme, lexe);
            }
            /*else if (strcmp(root->lexeme, "=") == 0)
                retval = setval(root->left->lexeme, rv);*/
            root->data = INT;
            tmp = root->left;
            root->left = NULL;
            free(tmp);
            tmp = root->right;
            root->right = NULL;
            free(tmp);
        }
        /*switch (root->data)
        {
        case ID:
        case INT:
            retval = root->val;
            break;
        case ASSIGN:
        case ADDSUB:
        case MULDIV:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            if (strcmp(root->lexeme, "+") == 0)
                retval = lv + rv;
            else if (strcmp(root->lexeme, "-") == 0)
                retval = lv - rv;
            else if (strcmp(root->lexeme, "*") == 0)
                retval = lv * rv;
            else if (strcmp(root->lexeme, "/") == 0)
                retval = lv / rv;
            else if (strcmp(root->lexeme, "=") == 0)
                retval = setval(root->left->lexeme, rv);
            break;
        default:
            retval = 0;
        }*/
    }
    //return retval;
}





int main()
{
    

    char name[5];
    for(int i = 0; i < 20; i++) {
        sprintf(name, "r%d", i);
        strcpy(r[i].name, name);
        r[i].n = i; //有用到ㄇ？
    }

    head = (Result*)malloc(sizeof(Result));
    sprintf(head->s, "");
    head->line = count;
    head->next = NULL;
    nr = head;
    //newRes();

    lineHead = (Line*)malloc(sizeof(Line));
    lineHead->num = count;
    lineHead->master[0] = 0;
    lineHead->master[1] = 0;
    lineHead->master[2] = 0;
    lineHead->independent = 0;
    nl = lineHead;

    while (1) {
        behind = 0;
        statement();
        
    }
    
    return 0;
}

