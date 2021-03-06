// this is code collection for function
#include <stdbool.h>


int size(tableNode* root){
    if (root == NULL){
        return 0;
    }
    else{
        int size_ = 1;
        if (root->varType == typeArray || root->varType == typeGlobalArray){
            size_ = calculateArraySize(root->arrayDim);
        }
        return size(root->leftNode) + size(root->rightNode) + size_;
    }
}

int calculateArraySize(int* arrayDim){
    if (!arrayDim){
        reportInvalidIndex();
    }else{
        int dim = arrayDim[0];
        int i = 1, acc = 1;
        for (i; i <= dim; ++i){
            acc *= arrayDim[i];
        }
        return acc;
    }
}


// function of counting parameter for a function call
int countParam(nodeType * p){
    if (!p){
        return 0;
    }else if (p->type == typeOpr && p->opr.oper == ','){
        // case of oprator = ','
        return countParam(p->opr.op[0]) + countParam(p->opr.op[1]);
    }else{
        return 1;
    }
}

int findLabel(char* funcName, int paramCnt, functionNode* root){
    if (!root){
        return -1;
    }else{
        int flag = strcmp(root->funcName, funcName);
        if (flag == 0 && root->paramCount == paramCnt){
            return root->label;
        }else{
            // support function overloading 
            if (flag == 0){
                return (paramCnt < root->paramCount)?findLabel(funcName, paramCnt, root->leftNode):
                                                   findLabel(funcName, paramCnt, root->rightNode);
            }else{
                return (flag<0)?findLabel(funcName, paramCnt, root->leftNode):
                                findLabel(funcName, paramCnt, root->rightNode);
            }
        }
    }
}


functionNode* findFuncNode(char* funcName, int paramCnt, functionNode* root){
    if (!root){
        return NULL;
    }else{
        int flag = strcmp(root->funcName, funcName);
        if (flag == 0 && root->paramCount == paramCnt){
            return root;
        }else{
            // support function overloading 
            if (flag == 0){
                return (paramCnt < root->paramCount)?findFuncNode(funcName, paramCnt, root->leftNode):
                                                   findFuncNode(funcName, paramCnt, root->rightNode);
            }else{
                return (flag<0)?findFuncNode(funcName, paramCnt, root->leftNode):
                                findFuncNode(funcName, paramCnt, root->rightNode);
            }
        }
    }
}
void updateFuncTable(char* funcName, int label, int paramCnt, int arrayCnt, int** arrayDimList, functionNode** root){
    if (!(*root)){
        functionNode * newOne = (functionNode*)malloc(sizeof(functionNode));
        strcpy(newOne->funcName, funcName);
        newOne->label = label;
        newOne->defined = 0; // currently no definition
        newOne->paramCount = paramCnt;
        newOne->arrayCount = arrayCnt;
        newOne->arrayDimList = arrayDimList;
        newOne->leftNode = NULL;
        newOne->rightNode = NULL;
        *root = newOne;
    }else{
        int flag = strcmp((*root)->funcName, funcName);
        if (flag == 0){
            // support function overloading 
            (paramCnt < (*root)->paramCount)?updateFuncTable(funcName, label, paramCnt, arrayCnt, arrayDimList, &((*root)->leftNode)):
                                        updateFuncTable(funcName, label, paramCnt, arrayCnt, arrayDimList, &((*root)->rightNode));
        }else{
            (flag<0)?updateFuncTable(funcName, label, paramCnt, arrayCnt, arrayDimList, &((*root)->leftNode)):
                     updateFuncTable(funcName, label, paramCnt, arrayCnt, arrayDimList, &((*root)->rightNode));
        }
        
    }
}

void recordDef(char* funcName, int paraCnt, functionNode* root){
    if (root==NULL){
        // this cannot happen
        return;
    }else{
        int flag = strcmp(root->funcName, funcName);
        if (flag == 0 && paraCnt == root->paramCount){
            #ifdef DEBUG
                printf("mark one of the function as defined\n");
            #endif
            if (root->defined == 0){
                root->defined = 1;
            }else{
                reportFuncRedefined(funcName, paraCnt);
            }
        }else{
            if (flag == 0){
                paraCnt < root->paramCount?recordDef(funcName, paraCnt, root->leftNode):
                                           recordDef(funcName, paraCnt, root->rightNode);
            }else{
                (flag<0)?recordDef(funcName, paraCnt, root->leftNode):
                         recordDef(funcName, paraCnt, root->rightNode);
            }
        }
    }
}
void localMemAlloc(int size){
    printf("\tpush\tsp\n");
    printf("\tpush\t%d\n", size);
    printf("\tadd\n");
    printf("\tpop\tsp\n");
}



void construct(nodeType* nodePtr, int offset, tableNode** root){
    #ifdef DEBUG
    printf("enter the contruct function\n");
    #endif
    if (*root == NULL){
        tableNode * newOne = (tableNode*)malloc(sizeof(tableNode));
        strcpy(newOne->varName, nodePtr->var.varName);
        newOne->varType = nodePtr->type;
        newOne->lineNo = -1;
        newOne->offset = offset;
        newOne->arrayDim = nodePtr->var.arrayDim;
        newOne->leftNode = NULL;
        newOne->rightNode = NULL;
        *root = newOne;
    }else{
        int flag = strcmp((*root)->varName, nodePtr->var.varName);
        if(flag < 0){
            construct(nodePtr, offset, &((*root)->leftNode));
        }else{
            construct(nodePtr, offset, &((*root)->rightNode));
        }
    }
}



// if param isParam is true, then traverse
// right node first
static functionNode* temp = NULL;
static counter = 0;
void traverse(nodeType* p, bool isParam, bool isMain){
    if (!p) return;
    if(isParam){
        // for parameter
        if(p->type >= typeVar && p->type <= typeVarStr){
            #ifdef DEBUG
            printf("construct for var:%s\n", p->var.varName);
            #endif
            tableNode* nodePtr = getNodeFromTable(p->var.varName, (isMain || p->type == typeGlobalArray)?
                                                                        mainVarTable:funcVarTable);
            if (!nodePtr) {
                construct(p, -4-funcVarCount++, &funcVarTable);
            }
            else if (nodePtr->varType == typeArray || nodePtr->varType == typeGlobalArray){
                // now no variable has type of this array
                // we would traverse the whole function to confirm 
                // that the variable is typeArray or typeGlobalArray
                // then second time traverse will tell us to record arrayDim
                nodePtr->lineNo = 0;
                nodePtr->arrayDim = temp->arrayDimList[counter];
                counter++;
            }
        }else if (p->type == typeArray || p->type == typeGlobalArray){

        }else if (p->type == typeOpr){
            traverse(p->opr.op[1], isParam, isMain);
            traverse(p->opr.op[0], isParam, isMain);
        }

    }else{
        // for normal statements
        if(p->type >= typeVar && p->type <= typeVarStr){
            tableNode* nodePtr = getNodeFromTable(p->var.varName, isMain?mainVarTable:funcVarTable);
            if (!nodePtr){
                #ifdef DEBUG
                printf("construct for var:%s\n", p->var.varName);
                #endif
                construct(p, funcVarCount++, isMain?(&mainVarTable):(&funcVarTable));
            }else{
                if(nodePtr->varType == typeArray || nodePtr->varType == typeGlobalArray){
                    p->type = nodePtr->varType;
                }
            }
        }else if(p->type == typeArray || p->type == typeGlobalArray){
            tableNode* nodePtr = getNodeFromTable(p->var.varName, (isMain || p->type == typeGlobalArray)?
                                                                    mainVarTable:funcVarTable);
            if (!nodePtr){
                // find corresponding variable with same name
                // assume that array must be declared first
                // no matter where the declaration is
                if(!p->var.arrayDim){
                    reportArrayUndeclared(p->var.varName);
                }
                construct(p, funcVarCount, isMain?(&mainVarTable):(&funcVarTable));
                funcVarCount += calculateArraySize(p->var.arrayDim);
            }else{
                if (p->var.arrayDim && !nodePtr->arrayDim){
                    // In the function, we will find the array that are actually used in array form
                    // then we overwrite all the type of the variables with the same name to be array
                    nodePtr->arrayDim = p->var.arrayDim;
                    nodePtr->varType = p->type;
                }
                // if (p->var.arrayDim && nodePtr->arrayDim){
                //     reportArrayDuplicatedDeclaration(p->var.varName);
                // }
                if (nodePtr->varType == typeVar || nodePtr->varType == typeGlobalVar){
                    nodePtr->varType = p->type;
                    #ifdef DEBUG
                        printf("update the variable: %s to type %d\n", p->var.varName, p->type);
                    #endif
                }
            }
            

        }else if(p->type == typeOpr){
            int i;
            for(i = 0; i < p->opr.nops; ++i){
                traverse(p->opr.op[i], isParam, isMain);
            }
        }
    }
}

// using static varible funcVarTable to construct variable tree
// for function inner variables
void constructFuncVarTable(nodeType* p, int typeFunc){
    if(typeFunc == funcDef ||
       typeFunc == funcReDef){
        // first construct for parameters
        traverse(p->opr.op[1], true, false);
        funcVarCount = 0;
        // then construct for variables in statements
        traverse(p->opr.op[2], false, false);
        int paraCnt = countParam(p->opr.op[1]);
        functionNode* nodePtr = findFuncNode(p->opr.op[0]->var.varName, paraCnt, functionTable);
        if (nodePtr->arrayCount > 0){
            // traverse parameters second time and then add arrayDim field
            temp = nodePtr;
            traverse(p->opr.op[1], true, false);
            if (counter != temp->arrayCount){
                printf("%d\n", counter);
                printf("%d\n", temp->arrayCount);
                reportInvalidArrayPara();
            }
            temp = NULL;
            counter = 0;
        }
    }else if(typeFunc == funcMain){
        // parse for main function
        funcVarCount = 0;
        traverse(p, false, true);
        funcVarCount = 0;
    }
    
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
            int arrayCnt = countArrayParam(p->opr.op[1], funcDef);
            int** arrayDimList = (int**)malloc(sizeof(int*)*arrayCnt);
            recordArrayDim(p->opr.op[1], funcDef);
            updateFuncTable(p->opr.op[0]->var.varName, label, paraCnt, arrayCnt, arrayDimList, &functionTable);
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

// this is used by checking function used but not defined error

int checkFuncFromTable(functionNode* root){
    if (!root){
        return 0;
    }else{
        int return1 = checkFuncFromTable(root->leftNode);
        int return2 = checkFuncFromTable(root->rightNode);
        int return3;
        if (!root->defined){
            fprintf(stderr, "function %s with %d argument(s) is used but undefined!!\n",
                    root->funcName, root->paramCount);
            return3 = 1;
        }else {
            return3 = 0;
        }
        return (return1 || return2 || return3);
    }
}


void checkGlobalVarDefFromTable(nodeType* p){
    if(!p){
        return;
    }
    if(p->type == typeGlobalVar){
        #ifdef DEBUG
            printf("find a global variable %s\n", p->var.varName);
        #endif
        tableNode* nodePtr = getNodeFromTable(p->var.varName, mainVarTable);
        if (!nodePtr){
            // record global variable definition
            #ifdef DEBUG
            printf("construct for var:%s\n", p->var.varName);
            #endif
            construct(p, funcVarCount++, &mainVarTable);
        }
    }else if(p->type == typeGlobalArray){
        tableNode* nodePtr = getNodeFromTable(p->var.varName, mainVarTable);
        // if first declared in the function
        if(!nodePtr){
            #ifdef DEBUG
            printf("construct for var:%s\n", p->var.varName);
            #endif
            construct(p, funcVarCount, &mainVarTable);
            funcVarCount += calculateArraySize(p->var.arrayDim);
        }else{
            if (nodePtr->arrayDim && p->var.arrayDim){
                reportArrayDuplicatedDeclaration(nodePtr->varName);
            }
        }

    }

    else if(p->type == typeOpr){
        int i;
        for(i=0; i < p->opr.nops;++i){
            checkGlobalVarDefFromTable(p->opr.op[i]);
        }
    }
}
// check all other function to see
// if there is any global variable definition
// if it is the case, add the variable into 
// main function stack memory allocation
// if not, do nothing
void checkGlobalVarDef(int currentCnt){
    functionDefNode* temp = funcDefList;
    nodeType* funcDefNode = NULL;
    funcVarCount = currentCnt;
    
    while(temp){
        #ifdef DEBUG
            printf("check global variable definition once\n");
            checkNode(temp->p);
        #endif
        funcDefNode = temp->p;
        checkGlobalVarDefFromTable(funcDefNode);
        temp = temp->next;
    }
    funcVarCount = 0;
}


int checkVarNameDuplicate(char* funcName, tableNode* root){
    if(!root){
        return 0;
    }else{
        int flag = strcmp(root->varName, funcName);
        if (!flag){
            return 1;
        }else{
            return flag<0?checkVarNameDuplicate(funcName, root->leftNode):
                          checkVarNameDuplicate(funcName, root->rightNode);
        }
    }
}

// function that will go through all nodes
// to check if there is clash with other variable
int hasConflict(char* funcName){
    return checkVarNameDuplicate(funcName, Table);
}

//function to count the array parameter
int countArrayParam(nodeType* p, int funcType){
    if (p){
        if (p->type == typeOpr && p->opr.oper == ','){
            // case of oprator = ','
            return countArrayParam(p->opr.op[0], funcType) + countArrayParam(p->opr.op[1], funcType);
        }else if (p->type == typeVar || p->type == typeGlobalVar){
            tableNode* nodePtr = getNodeFromTable(p->var.varName, (funcType == funcMain || p->type == typeGlobalVar)?mainVarTable:funcVarTable);
            if (nodePtr->varType == typeArray || nodePtr->varType == typeGlobalArray){
                return 1;
            }
        }else if (p->type == typeArray || p->type == typeGlobalArray){
            return 1;
        }
    }
    return 0;
}

// function to record all array dimention in sequence
static int arrayDimCounter = 0;
void recordArrayDim(nodeType* p, int** arrayDimList, int funcType){
    if (p){
        if (p->type == typeOpr && p->opr.oper == ','){
            // case of oprator = ','
            recordArrayDim(p->opr.op[0], arrayDimList, funcType);
            recordArrayDim(p->opr.op[1], arrayDimList, funcType);
        }else if (p->type == typeVar || p->type == typeGlobalVar){
            tableNode* nodePtr = getNodeFromTable(p->var.varName, (funcType == funcMain || p->type == typeGlobalVar)?mainVarTable:funcVarTable);
            if (nodePtr->varType == typeArray || nodePtr->varType == typeGlobalArray){
                arrayDimList[arrayDimCounter] = nodePtr->arrayDim;
                arrayDimCounter++;
            }
        }else if (p->type == typeArray || p->type == typeGlobalArray){
            tableNode* nodePtr = getNodeFromTable(p->var.varName, (funcType == funcMain || p->type == typeGlobalVar)?mainVarTable:funcVarTable);
            arrayDimList[arrayDimCounter] = nodePtr->arrayDim;
            arrayDimCounter++;
        }
    }
    return 0;
}