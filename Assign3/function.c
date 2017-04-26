// this is code collection for function
#include <stdbool.h>


int size(tableNode* root){
    if (root == NULL){
        return 0;
    }
    else{
        return size(root->leftNode) + size(root->rightNode) + 1;
    }
}


// function of counting parameter for a function call
int countParam(nodeType * p){
    if (p == NULL){
        return 0;
    }else if(p->type != typeOpr){
        return 1;
    }else if(p->opr.oper == ','){
        // case of oprator = ','
        return countParam(p->opr.op[0]) + countParam(p->opr.op[1]);
    }else{
        return 1;
    }
}

int findLabel(char* funcName, int paramCnt, functionNode* root){
    if (root == NULL){
        return -1;
    }else{
        int flag = strcmp(root->funcName, funcName);
        if (flag == 0 && root->paramCount == paramCnt){
            return root->label;
        }else{
            return (flag<0)?findLabel(funcName, paramCnt, root->leftNode):
                            findLabel(funcName, paramCnt, root->rightNode);
        }
    }
}

void updateFuncTable(char* funcName, int label, int paramCnt, functionNode** root){
    if (*root == NULL){
        functionNode * newOne = (functionNode*)malloc(sizeof(functionNode));
        strcpy(newOne->funcName, funcName);
        newOne->label = label;
        newOne->defined = 0; // currently no definition
        newOne->paramCount = paramCnt;
        newOne->leftNode = NULL;
        newOne->rightNode = NULL;
        *root = newOne;
    }else{
        int flag = strcmp((*root)->funcName, funcName);
        (flag<0)?updateFuncTable(funcName, label, paramCnt, &((*root)->leftNode)):
                 updateFuncTable(funcName, label, paramCnt, &((*root)->rightNode));
    }
}
void localMemAlloc(int size){
    printf("\tpush\tsp\n");
    printf("\tpush\t%d\n", size);
    printf("\tadd\n");
    printf("\tpop\tsp\n");
}


void updateVarType(nodeType * p, int type){
    if (type == -1){
        return;
    }else{
        #ifdef DEBUG
        checkTableNode(typeTable);
        printf("starting updating type node table for offset: %d\n", p->var.offset);
        #endif

        updateNodeType(p->var.offset, type, typeTable);
        #ifdef DEBUG
        printf("finish updating type node table\n");
        #endif
    }

}

void construct(char* varName, int offset, tableNode** root){
    #ifdef DEBUG
    printf("enter the contruct function\n");
    #endif
    if (*root == NULL){
        tableNode * newOne = (tableNode*)malloc(sizeof(tableNode));
        strcpy(newOne->varName, varName);
        newOne->varType = typeVar;
        newOne->lineNo = -1;
        newOne->offset = offset;
        newOne->leftNode = NULL;
        newOne->rightNode = NULL;
        *root = newOne;
    }else{
        int flag = strcmp((*root)->varName, varName);
        if(flag < 0){
            construct(varName, offset, &((*root)->leftNode));
        }else{
            construct(varName, offset, &((*root)->rightNode));
        }
    }
}



// if param isParam is true, then traverse
// right node first
void traverse(nodeType* p, bool isParam){
    if (!p) return;
    if(isParam){
        // for parameter
        if(p->type >= typeVar && p->type <= typeVarStr){
            #ifdef DEBUG
            printf("construct for var:%s\n", p->var.varName);
            #endif
            construct(p->var.varName, -4-funcVarCount++, &funcVarTable);
        }
        else if(p->type == typeOpr){
            traverse(p->opr.op[1], isParam);
            traverse(p->opr.op[0], isParam);
        }

    }else{
        // for normal statements
        if(p->type >= typeVar && p->type <= typeVarStr){
            int offset = getOffsetFromTable(p->var.varName, funcVarTable);
            if (offset == -1){
                #ifdef DEBUG
                printf("construct for var:%s\n", p->var.varName);
                #endif
                construct(p->var.varName, funcVarCount++, &funcVarTable);
            }
        }else if(p->type == typeOpr){
            int i;
            for(i = 0; i < p->opr.nops; ++i){
                traverse(p->opr.op[i], isParam);
            }
        }
    }
}

// using static varible funcVarTable to construct variable tree
// for function inner variables
void constructFuncVarTable(nodeType* p){
    // first construct for parameters
    traverse(p->opr.op[1], true);
    funcVarCount = 0;
    traverse(p->opr.op[2], false);
}

void recordFunctionCall(nodeType* p){
    if(!p) return;
    if(p->type == typeOpr && p->opr.oper == FUNCCALL){
        int paraCnt = countParam(p->opr.op[1]);
        // try to get the function
        int label = findLabel(p->opr.op[0]->var.varName, paraCnt, functionTable);
        if (label == -1){
            // set for new
            label = lbl++;
            // if first call, update for the table by creating node
            updateFuncTable(p->opr.op[0]->var.varName, label, paraCnt, &functionTable);
        }else{
            #ifdef DEBUG
                printf("this function has been called first time\n");
            #endif
        }
    }else if(p->type == typeOpr){
        int i;
        for(i=0; i < p->opr.nops;++i){
            recordFunctionCall(p->opr.op[i]);
        }
    }
}


void destruct(tableNode* root){
    if(!root){
        return;
    }else{
        destruct(root->leftNode);
        destruct(root->rightNode);
    }
    free(root);
}


void destructFuncVarTable(){
    #ifdef DEBUG
        printf("destructFuncVarTable\n");
    #endif
    destruct(funcVarTable);
    funcVarTable = NULL;
    funcVarCount = 0;
    #ifdef DEBUG
        printf("finish destruct funcVarTable\n");
    #endif
}