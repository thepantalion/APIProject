#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
#define snaptime 300

typedef enum{red, black} color;
typedef char bool;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
typedef struct treeNode{
    int key;
    char* data;                     //non viene mai cancellato effettivamente: è sempre puntato da qualcosa (memoria testo e/o U/R)
    color color;
    struct treeNode* father;
    struct treeNode* left;
    struct treeNode* right;
    struct treeNode* next;          //serve a gestire la struttura dati in maniera lineare
} treeNode;
#pragma clang diagnostic pop
typedef struct textNode{            //suppongo di creare un nuovo textNode per ogni nuova riga di testo inserita/cambiata/eliminata
    int key;                        //salva il numero di riga (sfruttato per identificare una lista composta di nodi soggetti a overwrite, piuttosto che creation)
    char* data;                     //puntatore alla stessa zona di memoria del campo 'data' di treeNode (aiuta a risparmiare memoria).
    struct textNode* nextLine;      //il programma non cancella effettivamente nessuna riga, fino a quando non sono chiamate 'c'/'d' e U/R -> current != U/R -> last.
} textNode;
typedef struct listNode{            //ogni nodo si intende come SNAPSHOT degli elementi che variano all'interno della struttura
    int opCode;
    int beginKey;                   //salva ind1 dell'operazione associata
    int endKey;                     //salva ind2 dell'operazione associata
    char command;                   //salva command dell'operazione associata
    bool isValid;                   //determina se l'operazione salvata ha effettivamente variato la struttura dati o meno
    bool isOverwrite;               //se command == 'c', indica che l'operazione salvata è di overwrite (ossia sovrascrittura di nodi già esistenti)
    int addedAfterOverwriting;      //se command == 'c && addedAfterOverwriting != 0 allora il "aao" la chiave del primo nodo creato dopo aver finito la overwrite
    int keysToDelete;               //se command == 'd', indica il numero di chiavi che cancellate effettivamente (potrei trovarlo ind2 - ind1 + 1 però diventa difficile se ind2 > maxKeyBO)
    struct textNode* data;          //puntatore al primo nodo della struttura textNode
    struct listNode* previous;      //puntatore all'istruzione precedente (il nodo è considerato 'meno recente' se previous == URnil)
    struct listNode* following;     //puntatore all'istruzione successiva (il nodo è considerato 'più recente' se following == URnil)
    struct snapNode* refSnap;       //puntatore alla snapshot più recente eseguita rispetto al nodo in fase di creazione
} listNode;
typedef struct snapNode{
    int maxKey;
    struct treeNode* root;
    struct listNode* associatedListNode;
    struct snapNode* previous;
    struct snapNode* following;
} snapNode;
typedef struct listHead{
    int totalOperations;
    int UR;
    struct listNode* first;
    struct listNode* operator;             //puntatore che segnala quale snapshot è stata annullata/ripristinata sulla struttura dati
    struct listNode* current;              //puntatore che indica, in base al numero di undo/redo, il limite di operatività di operator, in caso di print, change o delete
    struct listNode* last;                 //puntatore che indica l'ultimo elemento inserito (avanza solo quando aggiungo elementi, retrocede solo quando avvengono change o delete e current != last)
    struct snapNode* firstSnap;
    struct snapNode* lastSnap;
} listHead;

struct treeNode RBnil_node;         //nil_node per Red-Black tree
treeNode* RBnil = &RBnil_node;
struct listNode URnil_node;         //nil_node per lista dinamica
listNode* URnil = &URnil_node;
struct textNode TEXTnil_node;       //nil_node per la lista dinamica di testo all'interno di un nodo della lista UR
textNode* Tnil = &TEXTnil_node;
struct snapNode SNAPnil_node;       //nil_node per la lista dinamica di testo all'interno di un nodo della lista UR
snapNode* Snil = &SNAPnil_node;

treeNode* RBMaxNodeOperated;
listHead* urHead;
int maxKey;                         //registra il numero massimo inserito in ind2

//allocation - RB and List
treeNode* newTreeNode(int key){
    treeNode* temp = malloc(sizeof(treeNode));

    temp -> key = key;
    temp -> color = red;
    temp -> left = RBnil;
    temp -> right = RBnil;
    temp -> father = RBnil;
    temp -> next = RBnil;

    return temp;
}
listNode* newListNode(int beginKey, int endkey, char command, int opCode){
    listNode* temp = malloc(sizeof(listNode));

    temp -> beginKey = beginKey;
    temp -> endKey = endkey;
    temp -> command = command;
    temp -> opCode = opCode;
    temp -> data = Tnil;
    temp -> previous = URnil;
    temp -> following = URnil;
    temp -> refSnap = Snil;

    temp -> isValid = true;
    temp -> isOverwrite = false;
    temp -> addedAfterOverwriting = 0;
    temp -> keysToDelete = 0;

    return temp;
}
textNode* newTextNode(){
    textNode* temp = malloc(sizeof(textNode));
    temp -> nextLine = Tnil;

    return temp;
}
snapNode* newSnapNode(){
    snapNode* temp = malloc(sizeof(snapNode));
    temp -> root = RBnil;
    temp -> associatedListNode = URnil;
    temp -> previous = Snil;
    temp -> following = Snil;

    return temp;
}
void initializeStructures(){
    listNode* listNodeToOperate;
    snapNode* snapNodeToOperate;

    RBnil -> key = 0;              //es: per RBnil ci si aspetta sempre che la sua chiave sia 0 e il suo colore sia black.
    RBnil -> color = black;        //    Ciò non viene stabilito subito dopo la sua dichiarazione.
    RBMaxNodeOperated = RBnil;

    URnil -> opCode = -1;
    URnil -> beginKey = -1;
    URnil -> endKey = -1;
    URnil -> command = '\0';

    Snil -> maxKey = -1;
    Tnil -> key = -1;

    listNodeToOperate = newListNode(0, 0, '@', 0);     //questo è il nodo di default
    snapNodeToOperate = newSnapNode();
    snapNodeToOperate -> maxKey = 0;
    snapNodeToOperate -> associatedListNode = listNodeToOperate;
    listNodeToOperate -> refSnap = snapNodeToOperate;

    urHead = malloc(sizeof(listHead));
    urHead -> firstSnap = snapNodeToOperate;
    urHead -> lastSnap = snapNodeToOperate;
    urHead -> first = listNodeToOperate;
    urHead -> last = listNodeToOperate;
    urHead -> operator = listNodeToOperate;
    urHead -> current = listNodeToOperate;

    maxKey = 0;
}

//assistants
treeNode* RBLeftRotate(treeNode* root, treeNode *x){
    treeNode *y;

    y = x -> right;
    x -> right = y -> left; //il SASX di Y diventa SADX di x

    if(y -> left != RBnil) y -> left -> father = x;

    y -> father = x -> father; //padre di X diventa padre di Y

    if(x -> father == RBnil) root = y;
    else if(x == x -> father -> left) x -> father -> left = y;
    else x -> father -> right = y;

    y -> left = x;
    x -> father = y;

    return root;
}
treeNode* RBRightRotate(treeNode* root, treeNode *x){
    treeNode *y;

    y = x -> left;
    x -> left = y -> right;

    if(y -> right != RBnil) y -> right -> father = x;

    y -> father = x -> father;

    if(x -> father == RBnil) root = y;
    else if(x == x -> father -> left) x -> father -> left = y;
    else x -> father -> right = y;

    y -> right = x;
    x -> father = y;

    return root;
}
treeNode* RBTreeMinimum(treeNode* x){
    while(x -> left != RBnil)
        x = x -> left;
    return x;
}
treeNode* RBNodeSuccessor(treeNode* x){
    if(x -> right != RBnil) return RBTreeMinimum(x -> right);

    treeNode *y;
    y = x -> father;

    while(y != RBnil && x == y -> right){
        x = y;
        y = y -> father;
    }

    return y;
}

//insertion
treeNode* RBInsertFixup(treeNode* root, treeNode* toAdd){
    treeNode* x;
    treeNode* y;

    if(toAdd == root) root -> color = black;
    else {
        x = toAdd -> father;

        if(x -> color == red){
            if(x == x -> father -> left){
                y = x -> father -> right;

                if(y -> color == red){
                    x -> color = black;
                    y -> color = black;
                    x -> father -> color = red;
                    root = RBInsertFixup(root, x -> father); //in teoria non dovrebbero succedere cose brutte, controllare.
                } else {
                    if(toAdd == x -> right){
                        toAdd = x;
                        root = RBLeftRotate(root, toAdd);
                        x = toAdd -> father;
                    }

                    x -> color = black;
                    x -> father -> color = red;
                    root = RBRightRotate(root, x -> father);
                }
            } else {
                y = x -> father -> left;

                if(y -> color == red){
                    x -> color = black;
                    y -> color = black;
                    x -> father -> color = red;
                    root = RBInsertFixup(root, x -> father); //in teoria non dovrebbero succedere cose brutte, controllare.
                } else {
                    if(toAdd == x -> left){
                        toAdd = x;
                        root = RBRightRotate(root, toAdd);
                        x = toAdd -> father;
                    }

                    x -> color = black;
                    x -> father -> color = red;
                    root = RBLeftRotate(root, x -> father);
                }
            }
        }
    }

    return root;
}
treeNode* RBInsert(treeNode* root, treeNode* toAdd){
    treeNode* x;
    treeNode* y;

    y = RBnil;
    x = root;

    while(x != RBnil){
        y = x;

        if(toAdd -> key < x -> key) x = x -> left;
        else x = x -> right;
    }

    toAdd -> father = y;
    if(y == RBnil) root = toAdd;
    else if(toAdd -> key < y -> key) y -> left = toAdd;
    else y -> right = toAdd;

    return RBInsertFixup(root, toAdd);
}

//deletion
treeNode* RBDeleteFixup(treeNode* root, treeNode* x){
    treeNode* w;

    if(x -> color == red || x -> father == RBnil) x -> color = black;
    else if(x == x -> father -> left) {
        w = x -> father -> right;

        if(w -> color == red){
            w -> color = black;
            x -> father -> color = red;
            root = RBLeftRotate(root, x -> father);
            w = x -> father -> right;
        }

        if(w -> left -> color == black && w -> right -> color == black){
            w -> color = red;
            root = RBDeleteFixup(root, x -> father);
        } else {
            if(w -> right -> color == black){
                w -> left -> color = black;
                w -> color = red;
                root = RBRightRotate(root, w);
                w = x -> father -> right;
            }

            w -> color = x -> father -> color;
            x -> father -> color = black;
            w -> right -> color = black;
            root = RBLeftRotate(root, x -> father);
        }
    } else {
        w = x -> father -> left;

        if(w -> color == red){
            w -> color = black;
            x -> father -> color = red;
            //sbagliato sotto: RBLeftRotate(x -> father);
            root = RBRightRotate(root, x -> father);
            w = x -> father -> left;
        }

        if(w -> right -> color == black && w -> left -> color == black){
            w -> color = red;
            root = RBDeleteFixup(root, x -> father);
        } else {
            if(w -> left -> color == black){
                w -> right -> color = black;
                w -> color = red;
                root = RBLeftRotate(root, w);
                w = x -> father -> left;
            }

            w -> color = x -> father -> color;
            x -> father -> color = black;
            w -> left -> color = black;
            root = RBRightRotate(root, x -> father);
        }
    }

    return root;
}
treeNode* RBDelete(treeNode* root, treeNode* toDelete){
    treeNode* x;
    treeNode* y;

    if(toDelete -> left == RBnil || toDelete -> right == RBnil) y = toDelete;
    else y = RBNodeSuccessor(toDelete);

    if(y -> left != RBnil) x = y -> left;
    else x = y -> right;

    x -> father = y -> father;

    if(y -> father == RBnil) root = x;
        //sbagliato sotto: else if(y -> father -> left == x)
    else if(y -> father -> left == y) y -> father -> left = x;
    else y -> father -> right = x;

    if(y != toDelete) {
        toDelete -> key = y -> key;   //!!sono ritardato!! ovviamente mi sono dimenticato di copiare i dati satellite
        toDelete -> data = y -> data;
        toDelete -> next = y -> next;
    }
    if(y -> color == black) root = RBDeleteFixup(root, x);

    free(y);    //potrei evitare di fare la free, volendo

    return root;
}

//search
treeNode* RBSearch(treeNode* x, int key){
    while(x != RBnil && key != x -> key){
        if(key < x -> key) x = x -> left;
        else x = x -> right;
    }

    return x;
}

//project's specific assistants
void fixKeys(treeNode* x, int newKey){   //cambia le chiavi dei nodi per renderle incrementali, in seguito ad una delete
    while(x != RBnil){
        x -> key = newKey;
        if(x -> next != RBnil){
            x = x -> next;
            newKey++;
        }
        else x = RBnil;
    }
}
treeNode* undo(treeNode* root){
    int ind2;
    int currentKey;
    treeNode* RBNodeToOperate;
    treeNode* RBLastNodeOperated;
    treeNode* RBPredecessor = RBnil;
    listNode* listNodeToOperate;
    textNode* textNodeToOperate;

    while(urHead -> current != urHead -> operator){
        listNodeToOperate = urHead -> operator;     //undo viene chiamato su un nodo la cui operazione associata è già eseguita sulla memoria dati: serve annullare il nodo corrente.

        if(listNodeToOperate -> isValid == true){
            if(listNodeToOperate -> command == 'c' && listNodeToOperate -> isOverwrite == true){
                textNodeToOperate = listNodeToOperate -> data;
                currentKey = listNodeToOperate -> beginKey;
                if(listNodeToOperate -> addedAfterOverwriting != 0) maxKey = listNodeToOperate -> addedAfterOverwriting - 1;
                RBNodeToOperate = RBSearch(root, currentKey);

                while(currentKey <= listNodeToOperate -> endKey){
                    if(currentKey <= maxKey){
                        RBNodeToOperate -> data = textNodeToOperate -> data;

                        if(currentKey != listNodeToOperate -> endKey) {
                            if (currentKey < maxKey) {
                                RBNodeToOperate = RBNodeToOperate->next;
                                textNodeToOperate = textNodeToOperate->nextLine->nextLine;
                            } else {
                                RBMaxNodeOperated = RBNodeToOperate;
                                RBNodeToOperate = RBNodeToOperate->next;
                                RBMaxNodeOperated->next = RBnil;
                            }
                        }
                        currentKey++;
                    } else {
                        root = RBDelete(root, RBNodeToOperate);     //il ramo sopra viene svolto almeno una volta, quindi il nodo da eliminare qui esiste sempre, se accedo a questo ramo.

                        currentKey++;
                        RBNodeToOperate = RBSearch(root, currentKey);
                    }
                }
            } else if(listNodeToOperate -> command == 'c'){
                currentKey = listNodeToOperate -> beginKey;
                RBNodeToOperate = RBSearch(root, currentKey);

                while(currentKey <= listNodeToOperate -> endKey){
                    root = RBDelete(root, RBNodeToOperate);

                    currentKey++;
                    maxKey--;
                    RBNodeToOperate = RBSearch(root, currentKey);
                }

                RBMaxNodeOperated = RBSearch(root, maxKey);
                if(RBMaxNodeOperated != RBnil) RBMaxNodeOperated -> next = RBnil;
            }   //change

            if(listNodeToOperate -> command == 'd') {
                ind2 = listNodeToOperate -> keysToDelete + listNodeToOperate -> beginKey - 1;   //devo usare questo "reverse engineering" perché endKey potrebbe essere più alta di maxKey
                textNodeToOperate = listNodeToOperate -> data;
                currentKey = listNodeToOperate -> beginKey;
                RBLastNodeOperated = RBnil;

                if(root != RBnil){
                    if(currentKey == 1){                                //se il primo nodo è quello con chiave uno (1), "sposto" tutti i nodi verso la posizione designata
                        RBPredecessor = RBnil;
                        fixKeys(RBTreeMinimum(root), ind2 + 1);
                    } else {                                            //altrimenti trovo quello che precede e faccio lo stesso, solo con i
                        RBPredecessor = RBSearch(root, listNodeToOperate -> beginKey - 1);  //controllare se avrà successo questa operazione (servono delete su nodi con ind1, ind2 < maxKey)
                        fixKeys(RBPredecessor -> next, ind2 + 1);
                    }
                }   //preparativi per l'accettazione di nuovi nodi

                while(currentKey <= ind2){
                    RBNodeToOperate = newTreeNode(currentKey);
                    RBNodeToOperate -> data = textNodeToOperate -> data;
                    if(RBLastNodeOperated != RBnil && RBNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBNodeToOperate;
                    if(RBPredecessor != RBnil && currentKey == listNodeToOperate -> beginKey) RBPredecessor -> next = RBNodeToOperate;

                    root = RBInsert(root, RBNodeToOperate);
                    RBLastNodeOperated = RBNodeToOperate;

                    if(currentKey == ind2) RBNodeToOperate -> next = RBNodeSuccessor(RBNodeToOperate);
                    else textNodeToOperate = textNodeToOperate -> nextLine;

                    currentKey++;
                }

                maxKey = maxKey + listNodeToOperate -> keysToDelete;
                RBMaxNodeOperated = RBSearch(root, maxKey);
            }
        }

        urHead -> operator = urHead -> operator -> previous;
    }

    return root;
}
treeNode* redo(treeNode* root){
    int currentKey;
    treeNode* RBNodeToOperate;
    treeNode* RBLastNodeToOperate = RBnil;
    treeNode* RBPredecessor;
    listNode* listNodeToOperate;
    textNode* textNodeToOperate;

    while(urHead -> current != urHead -> operator){
        urHead -> operator = urHead -> operator -> following;
        listNodeToOperate = urHead -> operator;

        if(listNodeToOperate -> isValid == true){
            if(listNodeToOperate -> command == 'c' &&listNodeToOperate -> isOverwrite == true){
                textNodeToOperate = listNodeToOperate -> data -> nextLine;      //accedo alle righe in posizione "pari", dove troverò il contenuto nuovo
                currentKey = listNodeToOperate -> beginKey;
                RBNodeToOperate = RBSearch(root, textNodeToOperate -> key);

                while(currentKey <= listNodeToOperate -> endKey){
                    if(currentKey <= maxKey){
                        RBNodeToOperate -> data = textNodeToOperate -> data;

                        currentKey++;
                        if(currentKey <= listNodeToOperate -> endKey){
                            if(currentKey <= maxKey){
                                RBNodeToOperate = RBNodeToOperate -> next;
                                textNodeToOperate = textNodeToOperate -> nextLine -> nextLine;
                            } else {
                                RBLastNodeToOperate = RBNodeToOperate;
                                textNodeToOperate = textNodeToOperate -> nextLine;
                            }
                        }
                    } else {
                        RBNodeToOperate = newTreeNode(currentKey);
                        RBNodeToOperate -> data = textNodeToOperate -> data;
                        if(RBLastNodeToOperate != RBnil && RBNodeToOperate -> key == RBLastNodeToOperate -> key + 1) RBLastNodeToOperate -> next = RBNodeToOperate;

                        root = RBInsert(root, RBNodeToOperate);
                        RBLastNodeToOperate = RBNodeToOperate;

                        if(currentKey == listNodeToOperate -> endKey) RBMaxNodeOperated = RBLastNodeToOperate;
                        if(currentKey < listNodeToOperate -> endKey) textNodeToOperate = textNodeToOperate -> nextLine;    //in teoria dovrebbe limitarsi ad accedere una volta a Tnil
                        currentKey++;
                        maxKey++;
                    }
                }
            } else if(listNodeToOperate -> command == 'c'){
                textNodeToOperate = listNodeToOperate -> data;
                currentKey = listNodeToOperate -> beginKey;
                RBLastNodeToOperate = RBMaxNodeOperated;

                while(currentKey <= listNodeToOperate -> endKey){
                    RBNodeToOperate = newTreeNode(currentKey);
                    RBNodeToOperate -> data = textNodeToOperate -> data;
                    if(RBLastNodeToOperate != RBnil && RBNodeToOperate -> key == RBLastNodeToOperate -> key + 1) RBLastNodeToOperate -> next = RBNodeToOperate;

                    root = RBInsert(root, RBNodeToOperate);
                    RBLastNodeToOperate = RBNodeToOperate;

                    if(currentKey < listNodeToOperate -> endKey) textNodeToOperate = textNodeToOperate -> nextLine;
                    currentKey++;
                    maxKey++;
                }

                RBMaxNodeOperated = RBLastNodeToOperate;
            }

            if(listNodeToOperate -> command == 'd'){
                if(listNodeToOperate -> beginKey != 1) RBPredecessor = RBSearch(root, listNodeToOperate -> beginKey - 1);
                else RBPredecessor = RBnil;

                currentKey = listNodeToOperate -> beginKey;
                RBNodeToOperate = RBSearch(root, currentKey);

                while(currentKey <= listNodeToOperate -> endKey && currentKey <= maxKey){
                    root = RBDelete(root, RBNodeToOperate);

                    currentKey++;
                    RBNodeToOperate = RBSearch(root, currentKey);
                }

                if(root != RBnil){
                    if(listNodeToOperate -> beginKey == 1) fixKeys(RBTreeMinimum(root), 1);
                    else{
                        RBPredecessor -> next = RBNodeSuccessor(RBPredecessor);
                        fixKeys(RBPredecessor -> next, listNodeToOperate -> beginKey);
                    }
                }

                maxKey = maxKey - listNodeToOperate -> keysToDelete;
            }
        }
    }

    return root;
}

//debug
int _print_t(treeNode *tree, int is_left, int offset, int depth, char s[20][255])
{
    char b[20];
    int width = 5;

    if (!tree) return 0;

    sprintf(b, "(%03d)", tree->key);
    fflush(stdout);

    int left  = _print_t(tree->left,  1, offset,                depth + 1, s);
    int right = _print_t(tree->right, 0, offset + left + width, depth + 1, s);

#ifdef COMPACT
    for (int i = 0; i < width; i++)
        s[depth][offset + left + i] = b[i];

    if (depth && is_left) {

        for (int i = 0; i < width + right; i++)
            s[depth - 1][offset + left + width/2 + i] = '-';

        s[depth - 1][offset + left + width/2] = '.';

    } else if (depth && !is_left) {

        for (int i = 0; i < left + width; i++)
            s[depth - 1][offset - width/2 + i] = '-';

        s[depth - 1][offset + left + width/2] = '.';
    }
#else
    for (int i = 0; i < width; i++)
        s[2 * depth][offset + left + i] = b[i];

    if (depth && is_left) {

        for (int i = 0; i < width + right; i++)
            s[2 * depth - 1][offset + left + width/2 + i] = '-';

        s[2 * depth - 1][offset + left + width/2] = '+';
        s[2 * depth - 1][offset + left + width + right + width/2] = '+';

    } else if (depth && !is_left) {

        for (int i = 0; i < left + width; i++)
            s[2 * depth - 1][offset - width/2 + i] = '-';

        s[2 * depth - 1][offset + left + width/2] = '+';
        s[2 * depth - 1][offset - width/2 - 1] = '+';
    }
#endif

    return left + width + right;
}
void print_t(treeNode *tree)
{
    char s[20][255];
    for (int i = 0; i < 20; i++)
        sprintf(s[i], "%80s", " ");

    _print_t(tree, 0, 0, 0, s);

    for (int i = 0; i < 20; i++)
        printf("%s\n", s[i]);
}
void RBInorderTreeWalk(treeNode* x){
    int i;

    if(x != RBnil){
        RBInorderTreeWalk(x -> left);
        printf("\n  (%d", x -> key);
        if(x -> color == red) printf(" - R)  ");
        else printf(" - B)  ");
        for(i = 0; x -> data[i] != '\n'; i++) putc(x -> data[i], stdout);
        putc('\n', stdout);
        RBInorderTreeWalk(x -> right);
    }
}
void RBInorderNextTreeWalk(treeNode* root){
    int i;
    treeNode* RBNodeToOperate = RBnil;

    if(root != RBnil) RBNodeToOperate = RBTreeMinimum(root);
    printf("\n  -> maxKey = %d", maxKey);

    while(RBNodeToOperate != RBnil){
        printf("\n  (%d", RBNodeToOperate -> key);
        if(RBNodeToOperate -> color == red) printf(" - R)  ");
        else printf(" - B)  ");
        for(i = 0; RBNodeToOperate -> data[i] != '\n'; i++) putc(RBNodeToOperate -> data[i], stdout);
        putc('\n', stdout);

        RBNodeToOperate = RBNodeToOperate -> next;
    }
}
void inorderListWalk(){
    listNode* x;
    textNode* y;
    int i;

    x = urHead->first;

    while (x != URnil) {
        printf("\nKEY: %d", x->opCode);
        printf("\nCOMMAND: [%d,%d %c]\n", x->beginKey, x->endKey, x->command);
        if (x == urHead->current) printf("STATE: CURRENT\n");
        if (x == urHead->operator) printf("STATE: OPERATOR\n");
        printf("CONTENT:\n");
        if (x->isOverwrite == true) {
            printf("   isOverwrite = true\n");
            if (x->addedAfterOverwriting != 0) printf("   addedAfterOverwriting = %d\n", x->addedAfterOverwriting);
            printf("\n");
        }
        if(x -> keysToDelete != 0) printf("   keysToDelete = %d\n", x -> keysToDelete);
        if (x->isValid == false) printf("   isValid = false\n");
        printf("\n");

        if (x->isValid == true) {
            y = x->data;

            while (y != Tnil) {
                printf("   Riga %d - ", y->key);
                for (i = 0; y->data[i] != '\n'; i++) putc(y->data[i], stdout);
                putc('\n', stdout);
                y = y->nextLine;
            }
        }

        printf("\n----------\n");
        fflush(stdout);
        x = x->following;
    }

    printf("END.\n\n");
}
void inorderSnapListWalk(){
    snapNode* x = urHead -> firstSnap;
    int i = 1;

    printf("List of snapshots: \n");
    while(x != Snil){
        printf("  Snapshot n°%d referred to KEY %d - COMMAND [%d,%d %c]\n  - maxKey = %d\n", i, x->associatedListNode->opCode, x->associatedListNode->beginKey, x->associatedListNode->endKey, x->associatedListNode->command, x -> maxKey);
        RBInorderNextTreeWalk(x -> root);
        printf("\n");
        x = x -> following;
        i++;
    }

    printf("\n\n");
} //stampa la lista
treeNode* freeTree(treeNode* root){
    treeNode* x;

    for(int i = 1; i <= maxKey; i++){
        x = RBSearch(root, i);
        root = RBDelete(root, x);
    }

    return root;
}

int main(){
    int ind1;
    int ind2;
    int currentKey;              //salva la chiave corrente del nodo da trattare
    int keysToDelete;            //registra il numero di chiavi da cancellare
    int operationCounter = 0;
    int i;                       //assiste nella lettura della stringa "input" e nella sostituzione del contenuto dei campi 'data'
    int j;                       //assiste nella scrittura delle stringhe "inputInd1" e "inputInd2"
    int size;
    char inputInd1[10];          //assistente alla lettura del primo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char inputInd2[10];          //assistente alla lettura del secondo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char input[1024];            //multi-purpose string che registra comandi da decodificare e salva temporaneamente i dati da inserire nella struttura dati
    char command;                //contiene il comando da eseguire in seguito alla lettura di una riga di istruzione
    bool foundComma = false;     //aiuta a distinguere i comandi U,R,Q da C,D,P
    bool isWriteOnly = true;     //evita di registrare snapshot se sono eseguite solo change di nuove righe
    bool quitted = false;        //consente di uscire dal ciclo while principale del programma
    treeNode* RBroot = RBnil;
    treeNode* RBNodeToOperate;             //si vede assegnato l'indirizzo del nodo che sarà coinvolto in una qualunque delle istruzioni richieste
    treeNode* RBLastNodeOperated = RBnil;  //aiuta a garantire linearità nella stesura dell'albero (per impiegare meno tempo a cercare il successivo nodo da trattare)
    treeNode* RBPredecessor = RBnil;       //assiste nel correggere il campo 'next' dei nodi indirettamente coinvolti in una delete
    treeNode* RBDestNodeToOperate;
    listNode* listNodeToOperate;
    textNode* textNodeToOperate;
    snapNode* snapNodeToOperate;

    initializeStructures();

    //debug
    freopen("Altering_History_2_input.txt", "r", stdin);
    freopen("output2.txt", "w", stdout);
    int numOpDebug = 0;

    while(quitted == false){
        /*  -- gestione dell'input --
            'fgets' inserisce nella stringa multi-purpose "input" il comando da eseguire.
            per ipotesi, l'input è sempre corretto, quindi ci si aspetta sempre, all'inizio, un comando seguito dal carattere '\n'
            (che fgets cerca per poi terminare l'inserimento della riga nella stringa "input").

            la stringa "input" viene fatta scorrere alla ricerca degli indirizzi e del comando necessari ad accedere correttamente alla struttura.
        */
        fgets(input, sizeof(input), stdin);
        i = 0;
        j = 0;

        if(input[0] == 'q') command = 'q';
        else {
            while(input[i] >= '0' && input[i] <= '9'){
                inputInd1[j] = input[i];    //si sfrutta un'array di caratteri di supporto "inputInd1" per rilevare il primo indirizzo e convertirlo in intero
                j++;                        //da sfruttare per confronti e operazioni sulle chiavi della struttura
                i++;
            }
            ind1 = strtol(inputInd1, NULL, 10);         //atoi(char*) permette la conversione da stringa a intero di un numero contenuto nella stringa "inputInd1"

            if(input[i] == ','){            //nel caso di C,D,P ci si aspetta il formato (ind1,ind2command): trovare una virgola dopo il primo numero implica
                i++;                        //trattare uno dei casi evidenziati sopra, altrimenti si registra subito il comando U,R,Q e si procede senza ind2
                j = 0;                      //(non necessario nei comandi appena citati).
                foundComma = true;
                while(input[i] >= '0' && input[i] <= '9'){
                    inputInd2[j] = input[i];
                    j++;
                    i++;
                }
                ind2 = strtol(inputInd2, NULL, 10);
            }

            command = input[i];
        } //input

        //debug
        numOpDebug++;
        if(command == 'q') printf("[n. %d] %c", numOpDebug, command);
        else if(foundComma == true) printf("[n. %d] %d,%d %c\n\n", numOpDebug, ind1, ind2, command);
        else printf("[n. %d] %d %c\n\n", numOpDebug, ind1, command);
        fflush(stdout);

        /*  -- gestione di U/R --
         *  U/R è una lista dinamica composta da quattro puntatori: il puntatore al primo elemento, quello all'ultimo elemento inserito, current e operator:
         *  -> CURRENT rappresenta fino a quale operazione si dovrebbe annullare/ripristinare se dovesse essere necessario farlo
         *  -> OPERATOR indica precisamente a quale operazione è aggiornata la memoria dati in un dato momento
         *  Se i due puntatori non coincidono, vuol dire che vi sono delle operazioni di Undo/Redo pendenti, che verranno eseguite quando sarà richiesta
         *  una qualunque operazione di scrittura/lettura sulla memoria.
         *
         *  Per velocizzare il raggiungimento di current dalla posizione di operator, sfrutto una lista che contiene snapshot della struttura dati acquisite durante l'esecuziona.
         *  NOTA - Una snapshot viene scattata ogni volta che numOperazione (variabile che conta quante 'change' e 'delete' sono state svolte) è divisibile per 'snaptime' (vedi #define)
         *  Cerco la snapshot più vicina alla struttura dati richiesta e ripristino il suo contenuto nella memoria dati principale, infine eseguo le restanti operazioni.
         *  La memoria di U/R viene visualizzata (per esempio) come segue:
         *
         *            FIRST              OPERATOR  ->  ->  ->  ->  ->  ->  ->  CURRENT                        LAST
         *              V-------------------V-------------------------------------V-----------------------------v
         *              |default|       |   ^   |       |       |       |       | ^     |       |       |       |
         *              ^-------^-------^-------^-------^-------^-------^-------^-------^-------^-------^-------^
         *            snap1   snap1   snap2   snap3   snap4   snap5   snap6   snap7   snap8   snap9   snap10  snap11
         *
         *  Ogni snapshot segna l'inizio di "un nuovo blocco" di puntatori che fanno riferimento alla snapshot immediatamente precedente ad essi (mediante 'refSnap').
         *  -> Mediante accorgimenti, decido se saltare verso la snapshot più vicina a current e ripristinare, oppure operare direttamente sulla memoria dati esistente.
         *
         *  Nel caso di una 'change', 'delete' richieste mentre erano pendenti delle operazioni di Undo/Redo, elimino i riferimenti alle operazioni successive a 'current'
         *  e creo nuovi listNodes a partire da quel punto (portando avanti anche current e operator). Se invece è richiesta una 'print', non faccio nulla.
         */
        if(command == 'c' || command == 'd') operationCounter++;
        if(command != 'u' && command != 'r' && command != 'q'){
            if(urHead -> current != urHead -> operator){
                size = urHead -> operator -> opCode - urHead -> current -> opCode;  //calcolo la distanza tra i due nodi e confronto con il

                if(size < 0) size = size * -1;  //non può assumere valore zero (0) perché altrimenti non mi dovrei trovare dentro il ramo per la gestione U/R
                if(size <= snaptime / 2){
                    if(urHead -> operator -> opCode > urHead -> current -> opCode) RBroot = undo(RBroot);
                    else RBroot = redo(RBroot);
                } else {
                    snapNodeToOperate = urHead -> current -> refSnap;
                    if(snapNodeToOperate -> following != Snil && urHead -> current -> opCode % snaptime > snaptime / 2) snapNodeToOperate = snapNodeToOperate -> following;
                    urHead -> operator = snapNodeToOperate -> associatedListNode;

                    //debug
                    if(RBroot != RBnil) RBroot = freeTree(RBroot);

                    RBroot = RBnil;
                    maxKey = 0;

                    if(snapNodeToOperate -> root != RBnil){
                        RBNodeToOperate = RBTreeMinimum(snapNodeToOperate -> root);
                        RBLastNodeOperated = RBnil;

                        while(RBNodeToOperate != RBnil){
                            RBDestNodeToOperate = newTreeNode(RBNodeToOperate -> key);
                            RBDestNodeToOperate -> data = RBNodeToOperate -> data;
                            if(RBLastNodeOperated != RBnil && RBDestNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBDestNodeToOperate;

                            RBroot = RBInsert(RBroot, RBDestNodeToOperate);
                            RBLastNodeOperated = RBDestNodeToOperate;

                            RBNodeToOperate = RBNodeToOperate -> next;
                            if(RBNodeToOperate == RBnil) RBMaxNodeOperated = RBLastNodeOperated;
                        }

                        maxKey = snapNodeToOperate -> maxKey;
                    }   //ripristino di una snapshot

                    if(urHead -> operator != urHead -> current){   //dopo il salto potrebbe essere ancora necessario far muovere OPERATOR verso la posizione di CURRENT
                        if(urHead -> operator -> opCode > urHead -> current -> opCode) RBroot = undo(RBroot);
                        else RBroot = redo(RBroot);
                    }
                }

                if((command == 'c' || command == 'd') && urHead -> current != urHead -> last){
                    operationCounter = urHead -> current -> opCode + 1;
                    urHead -> totalOperations = urHead -> current -> opCode;
                    urHead -> last = urHead -> current;
                    urHead -> current -> following = URnil;
                    urHead -> current -> refSnap -> following = Snil;
                    urHead -> lastSnap = urHead -> current -> refSnap;
                }
            }

            if(command != 'p'){
                listNodeToOperate = urHead -> last;
                listNodeToOperate -> following = newListNode(ind1, ind2, command, operationCounter);
                listNodeToOperate -> following -> previous = listNodeToOperate;
                listNodeToOperate = listNodeToOperate -> following;

                listNodeToOperate -> refSnap = urHead -> lastSnap;

                urHead -> current = listNodeToOperate;
                urHead -> operator = listNodeToOperate;
                urHead -> last = listNodeToOperate;
                urHead -> totalOperations++;
            }
        }

        if(command == 'c' && (ind1 == maxKey + 1)) {            //questo particolare codice viene eseguito se la change avviene senza sovrascrittura di campi 'data' già esistenti
            currentKey = ind1;                                  //salvo la chiave corrente della riga da operare in una nuova variabile (si potrebbe usare ind1 ma diventerebbe incomprensibile)
            RBLastNodeOperated = RBMaxNodeOperated;             //aiutare nell'assegnare il campo *next

            textNodeToOperate = newTextNode();
            listNodeToOperate -> data = textNodeToOperate;

            while(currentKey <= ind2){                          //dovrebbe eseguire ind2 - ind1 +1
                fgets(input, sizeof(input), stdin);             //recupera la stringa da inserire nel campo 'data'

                RBNodeToOperate = newTreeNode(currentKey);      //alloca spazio per un nuovo nodo in memoria e assegna i dati presenti
                textNodeToOperate -> key = currentKey;
                size = strlen(input);                                        //determino e alloco lo spazio minimo necessario a salvare il testo nei nuovi nodo e textNode
                RBNodeToOperate -> data = malloc(size * sizeof(char));  //alloco un solo spazio di memoria che sarà puntato da due puntatori (RBNode -> data e textNode -> data)
                for(i = 0; i < size; i++) RBNodeToOperate -> data[i] = input[i];
                textNodeToOperate -> data = RBNodeToOperate -> data;         //i due puntatori puntano alla stasse zona di memoria

                if(RBLastNodeOperated != RBnil && RBNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBNodeToOperate;   //eseguito solo dopo almeno la creazione di un nodo
                RBroot = RBInsert(RBroot, RBNodeToOperate);
                RBLastNodeOperated = RBNodeToOperate;

                if(currentKey < ind2){
                    textNodeToOperate -> nextLine = newTextNode();      //se currentKey == ind2, non dovrei allocare nuovo spazio per un textNode, in quanto non esiste altro
                    textNodeToOperate = textNodeToOperate -> nextLine;  //testo da salvare per currentKey > ind2.
                }

                currentKey++;
                maxKey++;
            }

            RBMaxNodeOperated = RBLastNodeOperated;
            RBLastNodeOperated = RBnil;
            fgets(input, sizeof(input), stdin);                 //recupera il contenuto della riga che segue alla fine del blocco di righe da leggere (ossia '.\n')
        } //change - aggiunta di nuovi nodi
        else if(command == 'c'){                              //questa parte di codice suppone di dover sovrascrivere campi 'data' già esistenti (cioè ind1 <= maxKey e ind2 > maxKey)
            if(isWriteOnly == true) isWriteOnly = false;

            textNodeToOperate = newTextNode();
            listNodeToOperate -> data = textNodeToOperate;

            currentKey = ind1;                                  //se supera maxKey, svolge le stesse operazioni della parte superiore.
            RBNodeToOperate = RBSearch(RBroot, currentKey);
            listNodeToOperate -> isOverwrite = true;            //indico al nodo di U/R che sta salvando informazioni relative ad un'operazione di overwrite di nodi già esistenti
            if(ind2 > maxKey) listNodeToOperate -> addedAfterOverwriting = maxKey + 1;

            while(currentKey <= ind2){
                fgets(input, sizeof input, stdin);

                if(currentKey <= maxKey){
                    textNodeToOperate -> key = currentKey;                  //salvo la vecchia riga
                    textNodeToOperate -> data = RBNodeToOperate -> data;

                    textNodeToOperate -> nextLine = newTextNode();
                    textNodeToOperate = textNodeToOperate -> nextLine;
                    textNodeToOperate -> key = currentKey;

                    size = strlen(input);                                               //determino e alloco lo spazio minimo per salvare il nuovo contenuto su nodo esistente
                    RBNodeToOperate -> data = malloc(size * sizeof(char));        //non uso realloc perché rischio di rompere i puntatore della struttura U/R
                    for(i = 0; input[i] != '\0'; i++) RBNodeToOperate -> data[i] = input[i];
                    textNodeToOperate -> data = RBNodeToOperate -> data;

                    if(currentKey < maxKey) RBNodeToOperate = RBNodeToOperate -> next;      //se sto operando su un range di nodi che esistono già, sfrutto il campo *next per accedere al successivo
                    else RBLastNodeOperated = RBNodeToOperate;                              //altrimenti imposto le condizioni necessarie per operare su nodi nuovi

                    if(currentKey != ind2){
                        textNodeToOperate -> nextLine = newTextNode();
                        textNodeToOperate = textNodeToOperate -> nextLine;
                    }

                    currentKey++;
                } else {                                                             //stesse istruzioni del ramo 'true' del primo 'if' legato all'istruzione 'change'
                    RBNodeToOperate = newTreeNode(currentKey);                       //alloco spazio per un nuovo nodo in memoria
                    textNodeToOperate -> key = currentKey;
                    size = strlen(input);                                            //determino e alloco spazio per i campi 'data' di nodo e textNode associato
                    RBNodeToOperate -> data = malloc(size * sizeof(char));
                    for(i = 0; i < size; i++) RBNodeToOperate -> data[i] = input[i]; //assegno i campi 'data' carattere per carattere
                    textNodeToOperate -> data = RBNodeToOperate -> data;

                    if(RBLastNodeOperated != RBnil && RBNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBNodeToOperate; //eseguito solo dopo almeno la creazione di un nodo
                    RBroot = RBInsert(RBroot, RBNodeToOperate);
                    RBLastNodeOperated = RBNodeToOperate;

                    if(currentKey < ind2){
                        textNodeToOperate -> nextLine = newTextNode();
                        textNodeToOperate = textNodeToOperate -> nextLine;
                    }

                    //questa istruzione era sotto maxKey, verificare se la logica del progetto di rompe in questo modo
                    if(currentKey == ind2) RBMaxNodeOperated = RBLastNodeOperated;

                    currentKey++;
                    maxKey++;
                }
            }

            RBLastNodeOperated = RBnil;
            fgets(input, sizeof(input), stdin);
        } //change - sostituzione di nodi esistenti e eventuale aggiunta di nodi nuovi

        /* -- gestione di 'delete' --
         * La prima condizione verifica la validità dell'operazione di delete che si tenta di avviare.
         * In questo modo contrassegno come invalida l'operazione sul nodo U/R associato (in caso la suddetta condizione sia falsa).
         *
         * Assegno il puntatore RBPredecessor se la chiave di partenza è diversa da uno (1), per poter correggere le chiavi, come da specifica, e i campi *next.
         * Determino se il totale di chiavi da cancellare sia minore o uguale a maxKey
         * e assegno di conseguenza (perché, ovviamente, non posso cancellare più chiavi di quelle esistenti).
         *
         * Per ogni nodo da eliminare è necessaria una nuova ricerca sulla struttura perché la sua posizione in memoria potrebbe variare
         * se la funzione RBDelete() ritiene non bilanciato l'albero che forma la memoria dati per programma.
         * Ovviamente salvo chiave e stringa su un nodo di testo per tenere traccia in caso sia necessario ripristinare.
         *
         * In seguito alla serie di eliminazioni, se la struttura dati non è vuota, correggo le chiavi e/o il campo *next
         * del nodo che precedeva il primo nodo eliminato.
         * Infine aggiorno maxKey con il nuovo numero di nodi effettivamente esistenti e aggiorno il puntatore al nodo più alto (RBMaxNodeOperated).
         */
        if(command == 'd'){
            if(isWriteOnly == true) isWriteOnly = false;

            if(ind1 <= maxKey && ind1 >= 1 && RBroot != RBnil){     //se queste condizioni non sono verificate, non eseguo la delete e il nodo U/R associato viene segnato come "useless"
                if(ind1 > 1) RBPredecessor = RBSearch(RBroot, ind1 - 1);    //risalgo al predecessore del primo nodo da cancellare, per poi "attaccarlo" al nodo con chiave ind2 + 1
                else RBPredecessor = RBnil;                                                       //altrimenti non serve a nulla xoxo

                if(ind2 <= maxKey) keysToDelete = ind2 - ind1 + 1;      //calcolo quante righe eliminare per sapere quanto varia maxKey
                else keysToDelete = maxKey - ind1 + 1;
                listNodeToOperate -> keysToDelete = keysToDelete;       //questo campo aiuta a ristabilire il valore precedente di maxKey, nel caso di una Undo

                RBNodeToOperate = RBSearch(RBroot, ind1);   //cerco il primo nodo di chiave ind1
                currentKey = ind1;

                textNodeToOperate = newTextNode();
                listNodeToOperate -> data = textNodeToOperate;

                while(currentKey <= ind2 && currentKey <= maxKey){
                    textNodeToOperate -> key = currentKey;                  //copio le infomazioni essenziali del nodo (dati e chiave) sul nodo di testo
                    textNodeToOperate -> data = RBNodeToOperate -> data;    //questo verrà usato per ripristinare le info cancellate in caso di Undo

                    RBroot = RBDelete(RBroot, RBNodeToOperate);         //cancello effettivamente il nodo e bilancio correttamente l'albero

                    currentKey++;                                       //sono sempre costretto a cercare il nodo successivo perché *next potrebbe puntare ad un nodo
                    RBNodeToOperate = RBSearch(RBroot, currentKey);     //che è stato cancellato, oppure ha assunto valori nuovi rispetto a quelli che ci si aspetta (colpa di RBDelete(); ).

                    if(currentKey <= ind2 && currentKey <= maxKey){
                        textNodeToOperate -> nextLine = newTextNode();      //se il nodo corrente è minore di ind2 e maxKey,
                        textNodeToOperate = textNodeToOperate -> nextLine;  //creo un nuovo nodo di testo per salvare le info successive
                    }
                }

                if(RBroot != RBnil){                                                                //in seguito alla cancellazione, rimangono da fixare indici e campo *next dei nodi successivi/precedenti al range coinvolto
                    if(ind1 == 1) fixKeys(RBTreeMinimum(RBroot), 1);                        //se ho cancellato dal primo in poi, trovo il primo e correggo la chiave numerando da uno (1) in poi...
                    else {
                        RBPredecessor -> next = RBNodeSuccessor(RBPredecessor);                     //..altrimenti considero il nodo precedente al primo eliminato e trovo il suo successore
                        if(RBPredecessor -> next != RBnil) fixKeys(RBPredecessor -> next, ind1);    //se il succcessore esiste, correggo le chiavi da ind1 in poi.
                    }
                }

                maxKey = maxKey - keysToDelete;                                     //aggiorno maxKey sottraendo il numero totale di chiavi eliminate
                RBPredecessor = RBnil;                                              //resetto per questioni di sicurezza RBPredecessor
                RBMaxNodeOperated = RBSearch(RBroot, maxKey);   //cerco il nodo più alto possibile in seguito alla cancellazione
            } else listNodeToOperate -> isValid = false;
        } //delete

        /* -- gestione di 'undo' --
         * Undo si occupa solo di spostare il puntatore 'current' a sinistra della lista U/R (o verso l'inizio)
         * per indicare lo spostamento verso elementi meno recenti.
         *
         * -Non viene eseguito se 'current' punta già all'elemento meno recente
         * -Se 'current' punta ad un elemento della lista, questo non è ancora stato eseguito
         */
        if(command == 'u' && urHead -> first != urHead -> current){
            if(isWriteOnly == true) isWriteOnly = false;
            for(i = ind1; i != 0 && urHead -> current != urHead -> first; i--) urHead -> current = urHead -> current -> previous;
        } //undo

        /* -- gestione di 'redo' --
         * Redo si occupa solo di spostare il puntatore 'current' a destra della lista U/R (o verso la fine)
         * per indicare lo spostamento verso elementi più recenti.
         *
         * -Non viene eseguito se 'current' punta già all'elemento più recente
         */
        if(command == 'r' && urHead -> last != urHead -> current){
            if(isWriteOnly == true) isWriteOnly = false;
            for(i = ind1; i != 0 && urHead -> current != urHead -> last; i--) urHead -> current = urHead -> current -> following;
        } //redo

        if(command == 'p'){
            RBNodeToOperate = RBSearch(RBroot, ind1);          //cerca il primo nodo da stampare
            currentKey = ind1;

            while(currentKey <= ind2) {
                if(RBNodeToOperate != RBnil){
                    for(i = 0; RBNodeToOperate -> data[i] != '\n'; i++) putc_unlocked(RBNodeToOperate -> data[i], stdout);
                    putc_unlocked('\n', stdout);  //fputs printa fino al primo '\0', non la soluzione ideale se il nodo da cui si sta stampando è stato creato in seguito ad una delete
                    RBNodeToOperate = RBNodeToOperate -> next;
                    currentKey++;
                } else {
                    putc_unlocked('.', stdout);
                    putc_unlocked('\n', stdout);
                    currentKey++;
                }
            }
        } //print

        if(command == 'q') quitted = true;
        else {
            foundComma = false;             //il prossimo ciclo esegue una nuova riga di istruzione, si potrebbe dover rilevare un nuovo comando di tipo C,D,P
            for(i = 0; i < 10; i++){    //reset degli assistenti per il prossimo comando
                inputInd1[i] = '\0';
                inputInd2[i] = '\0';
            }    //reset degli assistenti per il prossimo comando

            if(command != 'p' && command != 'u' && command != 'r' && isWriteOnly == false){
                snapNodeToOperate = urHead -> lastSnap;
                if(operationCounter % snaptime == 0){
                    snapNodeToOperate -> following = newSnapNode();
                    snapNodeToOperate -> following -> previous = snapNodeToOperate;
                    snapNodeToOperate = snapNodeToOperate -> following;
                    snapNodeToOperate -> maxKey = maxKey;
                    snapNodeToOperate -> associatedListNode = listNodeToOperate;
                    urHead -> lastSnap = snapNodeToOperate;

                    if(RBroot != RBnil){
                        RBLastNodeOperated = RBnil;
                        RBNodeToOperate = RBTreeMinimum(RBroot);

                        while(RBNodeToOperate != RBnil){
                            RBDestNodeToOperate = newTreeNode(RBNodeToOperate -> key);
                            RBDestNodeToOperate -> data = RBNodeToOperate -> data;
                            if(RBLastNodeOperated != RBnil && RBDestNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBDestNodeToOperate;

                            snapNodeToOperate -> root = RBInsert(snapNodeToOperate -> root, RBDestNodeToOperate);
                            RBLastNodeOperated = RBDestNodeToOperate;

                            RBNodeToOperate = RBNodeToOperate -> next;
                        }
                    }

                    //debug
                    inorderSnapListWalk();
                }
                listNodeToOperate -> refSnap = snapNodeToOperate;
            }   //acquisizione di una snapshot
        }   //reset per una nuova operazione + snapshot

        //debug
        printf("\n---------\n\n");
        printf("---\n");
        inorderListWalk();
        //RBInorderTreeWalk(RBroot);
        RBInorderNextTreeWalk(RBroot);
        if(maxKey < 10) print_t(RBroot);
        printf("\n\n\n");
        fflush(stdout);
    }
}
#pragma clang diagnostic pop