//strutture per memoria testo e U/R
//lista concatenata -two ways- (meno efficiente)

//idea: e se usassi solo le chiavi per capire se un nodo è uguale ad un altro?
//tanto, alla fine, non devo fare altri controlli, visto che il contenuto deve essere ignorato
//e potrebbe essere richiesto molto più tempo per controllare tutto il contenuto...
#include <stdio.h>
#include <stdlib.h>
#define DIM 1024
#define false 0
#define true 1

//spoiler: se due puntatori puntano alla stessa zona, modificano gli stessi attributi

typedef enum{red, black} color;
typedef int bool;
typedef struct treeNode{
    int key;
    char data[DIM];
    color color;
    struct treeNode* father;
    struct treeNode* left;
    struct treeNode* right;
    struct treeNode* next;          //aiuta ad eseguire la stampa accedendo linearmente agli elementi
} treeNode;
typedef struct textNode{            //suppongo di creare un nuovo textNode per ogni nuova riga di testo inserita/cambiata/eliminata
    int key;                        //salva il numero di riga (sfruttato per identificare una lista composta di nodi soggetti a overwrite, piuttosto che creation)
    char data[DIM];                 //(non sapendo a priori quanto spazio serve)
    struct textNode* nextLine;
} textNode;
typedef struct listNode{            //ogni nodo si intende come SNAPSHOT degli elementi che variano all'interno della struttura
    int opCode;
    int beginKey;                   //salva ind1 dell'operazione associata
    int endKey;                     //salva ind2 dell'operazione associata
    char command;                   //salva command dell'operazione associata
    bool isValid;                   //determina se l'operazione salvata ha effettivamente variato la struttura dati o meno
    bool isOverwrite;               //se command == 'c', indica che l'operazione salvata è di overwrite (ossia sovrascrittura di nodi già esistenti)
    int addedAfterOverwriting;      //se command == 'c && addedAfterOverwriting != 0 allora il "aao" la chiave del primo nodo creato dopo aver finito la overwrite
    int maxKeyBeforeOperation;      //se command == 'd', indica la maxKey che aveva la struttura prima dell'operazione di delete
    int keysToDelete;               //se command == 'd', indica il numero di chiavi che cancellate effettivamente (potrei trovarlo ind2 - ind1 + 1 però diventa difficile se ind2 > maxKeyBO)
    struct textNode* data;          //puntatore al primo nodo della struttura textNode
    struct listNode* following;     //puntatore all'istruzione precedente (il nodo è considerato 'meno recente' se following == URnil)
    struct listNode* previous;      //puntatore all'istruzione successiva (il nodo è considerato 'più recente' se following == URnil)
} listNode;
typedef struct listHead{
    int totalOperations;
    struct listNode* first;
    struct listNode* operator;             //puntatore che segnala quale snapshot è stata annullata/ripristinata sulla struttura dati
    struct listNode* current;              //puntatore che indica, in base al numero di undo/redo, il limite di operatività di operator, in caso di print, change o delete
    struct listNode* last;                 //puntatore che indica l'ultimo elemento inserito (avanza solo quando aggiungo elementi, retrocede solo quando avvengono change o delete e current != last)
} listHead;

struct treeNode RBnil_node;         //nil_node per Red-Black tree
treeNode* RBnil = &RBnil_node;
struct listNode URnil_node;         //nil_node per lista dinamica
listNode* URnil = &URnil_node;
struct textNode TEXTnil_node;       //nil_node per la lista dinamica di testo all'interno di un nodo della lista UR
textNode* Tnil = &TEXTnil_node;

treeNode* root;
listHead* urHead;

//RBnil è una sentinella: sostituisce la necessità
//di un elemeneto inizializzato a NULL per le foglie
//Creo una variabile inutile di tipo treeNode denominata RBnil_node (per creare un puntatore a memoria inutilizzata), 
//poi il suo indirizzo di questa porzione di memoria diventa l'indirizzo della sentinella RBnil.
//se node -> father == RBnil allora node è la radice
//se node -> left/right == RBnil allora node è foglia
//se l'oggetto rootTree == RBnil allora l'albero è vuoto

// Funzioni per la gestione degli alberi
treeNode* newNode(int key){
    treeNode* temp = (treeNode*) malloc(sizeof(treeNode));

    temp -> key = key;
    temp -> color = red;
    temp -> left = RBnil;
    temp -> right = RBnil;
    temp -> father = RBnil;

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

    return temp;
}
textNode* newTextNode(){
    textNode* temp = malloc(sizeof(textNode));
    temp -> nextLine = Tnil;

    return temp;
}
void initializeStructures(){
    root = RBnil;                //inizializzazioni necessarie a far sì che le assunzioni delle parti di codice siano rispettate in qualunque momento
    RBnil -> key = 0;            //es: per RBnil ci si aspetta sempre che la sua chiave sia 0 e il suo colore sia black.
    RBnil -> color = black;      //    Ciò non viene stabilito subito dopo la sua dichiarazione.

    URnil -> opCode = 0;
    URnil -> beginKey = 0;
    URnil -> endKey = 0;
    URnil -> command = '\0';

    urHead = malloc(sizeof(listHead));
    urHead -> totalOperations = 0;
    urHead -> first = URnil;
    urHead -> operator = URnil;
    urHead -> current = URnil;
    urHead -> last = URnil;
}

//Rotation - assists during Insertion and Deletion
void RBLeftRotate(treeNode *x){
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
}
void RBRightRotate(treeNode *x){
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
}
treeNode* RBTreeMinimum(treeNode* x){
    while(x -> left != RBnil) x = x -> left;
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
void RBInsertFixup(treeNode* toAdd){
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
                    RBInsertFixup(x -> father); //in teoria non dovrebbero succedere cose brutte, controllare.
                } else {
                    if(toAdd == x -> right){
                        toAdd = x;
                        RBLeftRotate(toAdd);
                        x = toAdd -> father;
                    }

                    x -> color = black;
                    x -> father -> color = red;
                    RBRightRotate(x -> father);
                }
            } else {
                y = x -> father -> left;

                if(y -> color == red){
                    x -> color = black;
                    y -> color = black;
                    x -> father -> color = red;
                    RBInsertFixup(x -> father); //in teoria non dovrebbero succedere cose brutte, controllare.
                } else {
                    if(toAdd == x -> left){
                        toAdd = x;
                        RBRightRotate(toAdd);
                        x = toAdd -> father;
                    }

                    x -> color = black;
                    x -> father -> color = red;
                    RBLeftRotate(x -> father);
                }
            }
        }
    }
}
void RBInsert(treeNode* toAdd){
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

    /*
    toAdd -> color = red;
    toAdd -> left = RBnil;
    toAdd -> right = RBnil;
    */

    //operazioni già eseguite in newNode

    RBInsertFixup(toAdd);
}

treeNode* RBSearch(treeNode* x, int key){
    while(x != RBnil && key != x -> key){
        if(key < x -> key) x = x -> left;
        else x = x -> right;
    }

    return x;
}

//deletion
void RBDeleteFixup(treeNode* x){
    treeNode* w;

    if(x -> color == red || x -> father == RBnil) x -> color = black;
    else if(x == x -> father -> left) {
        w = x -> father -> right;

        if(w -> color == red){
            w -> color = black;
            x -> father -> color = red;
            RBLeftRotate(x -> father);
            w = x -> father -> right;
        }

        if(w -> left -> color == black && w -> right -> color == black){
            w -> color = red;
            RBDeleteFixup(x -> father);
        } else {
            if(w -> right -> color == black){
                w -> left -> color = black;
                w -> color = red;
                RBRightRotate(w);
                w = x -> father -> right;
            }

            w -> color = x -> father -> color;
            x -> father -> color = black;
            w -> right -> color = black;
            RBLeftRotate(x -> father);
        }
    } else {
        w = x -> father -> left;

        if(w -> color == red){
            w -> color = black;
            x -> father -> color = red;
            //sbagliato sotto: RBLeftRotate(x -> father);
            RBRightRotate(x -> father);
            w = x -> father -> left;
        }

        if(w -> right -> color == black && w -> left -> color == black){
            w -> color = red;
            RBDeleteFixup(x -> father);
        } else {
            if(w -> left -> color == black){
                w -> right -> color = black;
                w -> color = red;
                RBLeftRotate(w);
                w = x -> father -> left;
            }

            w -> color = x -> father -> color;
            x -> father -> color = black;
            w -> left -> color = black;
            RBRightRotate(x -> father);
        }
    }
}
void RBDelete(treeNode* toDelete){
    treeNode* x;    //nodo che sostituisce quello da cancellare
    treeNode* y;    //nodo che viene effettivamente cancellato

    if(toDelete -> left == RBnil || toDelete -> right == RBnil) y = toDelete;
    else y = RBNodeSuccessor(toDelete);

    if(y -> left != RBnil) x = y -> left;
    else x = y -> right;

    x -> father = y -> father;

    if(y -> father == RBnil) root = x;
    //sbagliato sotto: else if(y -> father -> left == x)
    else if(y -> father -> left == y) y -> father -> left = x;
    else y -> father -> right = x;

    if(y != toDelete) toDelete -> key = y -> key;
    if(y -> color == black) RBDeleteFixup(x);

    free(y);
}

//funzioni di debug (prese da Internet) per visualizzare la struttura dell'albero.
int _print_t(treeNode *tree, int is_left, int offset, int depth, char s[20][255])
{
    char b[20];
    int width = 5;

    if (!tree) return 0;

    sprintf(b, "(%03d) - %d", tree->key, tree -> color);

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
    if(x != RBnil){
        RBInorderTreeWalk(x -> left);
        printf("\n%d", x -> key);
        if(x -> color == red) printf(" - R\n");
        else printf(" - B\n");
        for(int i = 0; x -> data[i] != '\0'; i++) putc(x -> data[i], stdout);
        putc('\n', stdout);
        RBInorderTreeWalk(x -> right);
    }
}
void inorderListWalk(){
    listNode* x;
    textNode* y;
    int i;


    x = urHead -> first;

    while(x != URnil){
        printf("\nCOMMAND: [%d,%d %c]\nCONTENT:\n", x -> beginKey, x -> endKey, x -> command);
        y = x -> data;

        while(y != Tnil){
            printf("Riga %d - ", y -> key);
            for(i = 0; y -> data[i] != '\0'; i++) putc(y -> data[i], stdout);
            putc('\n', stdout);
            y = y -> nextLine;
        }
        
        printf("\n----------\n");
        x = x -> following;
    }

    printf("END.");
}

int main() {
    int key = 1;
    int movementsLeft;

    treeNode* toAdd;
    treeNode* realRoot;
    treeNode* test;
    listNode* listNodeToOperate = URnil;
    textNode* textNodeToOperate = Tnil;
    listHead* debugURHead;
    char* input = "Hello there!";
    bool currDiffersOperator = false;
    
    initializeStructures();

    if(urHead -> first == URnil){
        listNodeToOperate = newListNode(1, 15, 'c', 1);    //supponendo che la nuova struttura dati tenga conto
        textNodeToOperate = newTextNode();
        listNodeToOperate -> data = textNodeToOperate;
        
        urHead -> first = listNodeToOperate;
        urHead -> operator = listNodeToOperate;
        urHead -> current = listNodeToOperate;
        urHead -> last = listNodeToOperate;
        urHead -> totalOperations++;

        debugURHead = urHead;
    } else if (currDiffersOperator != false) {
        //codice da eseguire per annullare effettivamente delle operazioni
    }

    for(int i = 1; i <= 15; i++){
        toAdd = newNode(i);
        RBInsert(toAdd);
        textNodeToOperate -> key = i;

        for(int j = 0; input[j] != '\0'; j++){
            toAdd -> data[j] = input[j];
            textNodeToOperate -> data[j] = input[j];
        }

        if(i < 15) {    //creo un nodo nuovo solo per currKey < ind2
            textNodeToOperate -> nextLine = newTextNode();
            textNodeToOperate = textNodeToOperate -> nextLine;
        }
    }

    if(urHead -> first != URnil){
        listNodeToOperate -> following = newListNode(16, 25, 'c', 2);
        listNodeToOperate -> following -> previous = listNodeToOperate;
        listNodeToOperate = listNodeToOperate -> following;
        textNodeToOperate = newTextNode();
        listNodeToOperate -> data = textNodeToOperate;

        urHead -> current = listNodeToOperate;
        urHead -> operator = listNodeToOperate;
        urHead -> last = listNodeToOperate;
        urHead -> totalOperations++;
    }

    for(int i = 16; i <= 25; i++){
        toAdd = newNode(i);
        RBInsert(toAdd);
        textNodeToOperate -> key = i;

        for(int j = 0; input[j] != '\0'; j++){
            toAdd -> data[j] = input[j];
            textNodeToOperate -> data[j] = input[j];
        }

        if(i < 25) {
            textNodeToOperate -> nextLine = newTextNode();
            textNodeToOperate = textNodeToOperate -> nextLine;
        }
    }

    if(urHead -> first != URnil){
        listNodeToOperate -> following = newListNode(26, 30, 'c', 3);
        listNodeToOperate -> following -> previous = listNodeToOperate;
        listNodeToOperate = listNodeToOperate -> following;
        textNodeToOperate = newTextNode();
        listNodeToOperate -> data = textNodeToOperate;

        urHead -> current = listNodeToOperate;
        urHead -> operator = listNodeToOperate;
        urHead -> last = listNodeToOperate;
        urHead -> totalOperations++;
    }

    for(int i = 26; i <= 30; i++){
        toAdd = newNode(i);
        RBInsert(toAdd);
        textNodeToOperate -> key = i;

        for(int j = 0; input[j] != '\0'; j++){
            toAdd -> data[j] = input[j];
            textNodeToOperate -> data[j] = input[j];
        }

        if(i < 30) {
            textNodeToOperate -> nextLine = newTextNode();
            textNodeToOperate = textNodeToOperate -> nextLine;
        }
    }

    if(urHead -> first != URnil){
        listNodeToOperate -> following = newListNode(31, 37, 'c', 4);
        listNodeToOperate -> following -> previous = listNodeToOperate;
        listNodeToOperate = listNodeToOperate -> following;
        textNodeToOperate = newTextNode();
        listNodeToOperate -> data = textNodeToOperate;

        urHead -> current = listNodeToOperate;
        urHead -> operator = listNodeToOperate;
        urHead -> last = listNodeToOperate;
        urHead -> totalOperations++;
    }

    for(int i = 31; i <= 37; i++){
        toAdd = newNode(i);
        RBInsert(toAdd);
        textNodeToOperate -> key = i;

        for(int j = 0; input[j] != '\0'; j++){
            toAdd -> data[j] = input[j];
            textNodeToOperate -> data[j] = input[j];
        }

        if(i < 37) {
            textNodeToOperate -> nextLine = newTextNode();
            textNodeToOperate = textNodeToOperate -> nextLine;
        }
    }

    realRoot = root;
    print_t(root);
    RBInorderTreeWalk(root);
    inorderListWalk();

    //movimento di current in seguito a u
    movementsLeft = 49;
    for(int i = movementsLeft; i != 0 && urHead -> current != urHead -> first; i--) {
        urHead -> current = urHead -> current -> previous;
    }

    //movimento di current in seguito a r
    movementsLeft = 49;
    for(int i = movementsLeft; i != 0 && urHead -> current != urHead -> last; i--) {
        urHead -> current = urHead -> current -> following;
    }



    /* TEST DEL FUNZIONAMENTO DELLA STRUTTURA RB - TUTTI COMPLETATI
    RBInsert(newNode(4));
    print_t(root);
    
    
    realRoot = root;
    RBInorderTreeWalk(root);

    test = RBSearch(root, 3);
    RBDelete(test);
    realRoot = root;
    print_t(root);
    RBInorderTreeWalk(root);

    test = RBSearch(root, 2);
    RBDelete(test);
    //errore: sotto riportavo il comando free(test); che porta a errori in quanto test potrebbe divenire un nuovo nodo utile nella struttura dati
    //per ovviare al problema, è stato introdotto un comando free(y) nel codice di RBDelete in quanto y è il nodo effettivamente da cancellare.
    realRoot = root;
    print_t(root);
    RBInorderTreeWalk(root);

    test = RBSearch(root, 1);
    RBDelete(test);
    realRoot = root;
    print_t(root);
    RBInorderTreeWalk(root);

    
    test = RBSearch(root, 6);
    RBDelete(test);
    realRoot = root;
    print_t(root);
    RBInorderTreeWalk(root);

    test = RBSearch(root, 7);
    RBDelete(test);
    realRoot = root;
    print_t(root);
    RBInorderTreeWalk(root);
    
    test = RBTreeMinimum(root);
    key = 1;
    while(test != RBnil){
        test -> key = key;
        test = RBNodeSuccessor(test);
        key++;
    }

    print_t(root);
    RBInorderTreeWalk(root);
    */

    return 0;
}

