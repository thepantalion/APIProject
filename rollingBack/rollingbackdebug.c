// voto conseguito sul task writeOnly: 5/5
// voto conseguito sul task bulkReads: 5/5
// voto conseguito sul task timeForAChange: -/-
#include <stdio.h>
#include <stdlib.h>
#define DIM 1024
#define true 1
#define false 0

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

//allocation - RB and List
treeNode* newTreeNode(int key){
    treeNode* temp = malloc(sizeof(treeNode));
    int i = 0;

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

    temp -> isValid = true;
    temp -> isOverwrite = false;
    temp -> addedAfterOverwriting = 0;
    temp -> maxKeyBeforeOperation = 0;
    temp -> keysToDelete = 0;

    return temp;
}
textNode* newTextnode(){
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

//assistants - RB
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

//insertion - RB
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

    //operazioni già eseguite in newTreeNode

    RBInsertFixup(toAdd);
}

//deletion - RB
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
    treeNode* x;
    treeNode* y;
    int i;

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
        for(i = 0; toDelete -> data[i] != '\0'; i++) toDelete -> data[i] = '\0';
        for(i = 0; y -> data[i] != '\0'; i++) toDelete -> data[i] = y -> data[i];
        toDelete -> next = y -> next;
    }
    if(y -> color == black) RBDeleteFixup(x);

    free(y);
}

//search and print - RB
treeNode* RBSearch(treeNode* x, int key){
    while(x != RBnil && key != x -> key){
        if(key < x -> key) x = x -> left;
        else x = x -> right;
    }

    return x;
}

//project's specific assistants - RB
void fixKeys(treeNode* x, int newKey){   //cambia le chiavi dei nodi per renderle incrementali, in seguito ad una delete
    while(x != RBnil){
        x -> key = newKey;
        x = x -> next;
        newKey++;
    }
}

//debug
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
    int i;

    if(x != RBnil){
        RBInorderTreeWalk(x -> left);
        printf("\n(%d", x -> key);
        if(x -> color == red) printf(" - R)  ");
        else printf(" - B)  ");
        for(i = 0; x -> data[i] != '\n'; i++) putc(x -> data[i], stdout);
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

int main(){
    int ind1;
    int ind2;
    int currentKey;              //salva la chiave corrente del nodo da trattare
    int maxKey = 0;              //registra il numero massimo inserito in ind2
    int keysToDelete;            //registra il numero di chiavi da cancellare
    int numOperazione = 0;
    int i = 0;                   //assiste nella lettura della stringa "input" e nella sostituzione del contenuto dei campi 'data'
    int j = 0;                   //assiste nella scrittura delle stringhe "inputInd1" e "inputInd2"
    char inputInd1[10];          //assistente alla lettura del primo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char inputInd2[10];          //assistente alla lettura del secondo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char input[1024];            //multi-purpose string che registra comandi da decodificare e salva temporaneamente i dati da inserire nella struttura dati
    char command;                //contiene il comando da eseguire in seguito alla lettura di una riga di istruzione
    bool foundComma = false;     //aiuta a distinguere i comandi U,R,Q da C,D,P
    bool quitted = false;        //consente di uscire dal ciclo while principale del programma
    treeNode* RBNodeToOperate;             //si vede assegnato l'indirizzo del nodo che sarà coinvolto in una qualunque delle istruzioni richieste
    treeNode* RBLastNodeOperated = RBnil;  //aiuta a garantire linearità nella stesura dell'albero (per impiegare meno tempo a cercare il successivo nodo da trattare)
    treeNode* RBMaxNodeInserted = RBnil;   //in concerto con maxKey, aiuta a garantire linearità nella stesura dell'albero, in caso di change consecutive nel caso ind1 = maxKey + 1
    treeNode* RBPredecessor = RBnil;       //assiste nel correggere il campo 'next' dei nodi indirettamente coinvolti in una delete
    listNode* listNodeToOperate;
    textNode* textNodeToOperate;

    initializeStructures();      //inizializza tutte le strutture globali sfruttate all'interno del programma (albero, lista e nodi NIL associati);

    //debug
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);
    treeNode* debugRoot;
    listHead* debugURHead;

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
        }

        //debug
        if(command == 'q') printf("[n. %d] %c", numOperazione, command);
        else if(foundComma == true) printf("[n. %d] %d,%d %c\n\n", numOperazione, ind1, ind2, command);
        else printf("[n. %d] %d %c\n\n", numOperazione, ind1, command);
        fflush(stdout);

        /* -- gestione preliminare di undo/redo --
           verifico se la struttura dedicata a undo/redo presenta già degli elementi e procedo alle opportune allocazioni
        */
        if(command != 'u' && command != 'r' && command != 'q'){
            numOperazione++;

            if(urHead -> current != urHead -> last && command == 'c'){
                //operazioni per cancellare gli elementi della lista, prima di eseguire una change
            }

            if(urHead -> current != urHead -> last && command == 'd'){
                //operazioni per cancellare gli elementi della lista, prima di eseguire una delte
            }

            if(urHead -> current != urHead -> last && command == 'p'){
                //operazioni per annullare delle operazioni, senza cancellare i nodi della lista 
            }

            if(urHead -> last == URnil){
                listNodeToOperate = newListNode(ind1, ind2, command, numOperazione);    //supponendo che la nuova struttura dati tenga conto
                textNodeToOperate = newTextNode();
                listNodeToOperate -> data = textNodeToOperate;
                
                urHead -> first = listNodeToOperate;
                urHead -> operator = listNodeToOperate;
                urHead -> current = listNodeToOperate;
                urHead -> last = listNodeToOperate;
                urHead -> totalOperations++;
                
                debugURHead = urHead;
            } else {
                listNodeToOperate -> following = newListNode(ind1, ind2, command, numOperazione);
                listNodeToOperate -> following -> previous = listNodeToOperate;
                listNodeToOperate = listNodeToOperate -> following;
                textNodeToOperate = newTextNode();
                listNodeToOperate -> data = textNodeToOperate;

                urHead -> current = listNodeToOperate;
                urHead -> operator = listNodeToOperate;
                urHead -> last = listNodeToOperate;
                urHead -> totalOperations++;
            }
        }

        /*  -- gestione di 'change' --
            il codice si suddivide in vari casi da gestire:
            1. tutte le righe richieste esistono già e devono solo essere modificate
                -> viene eseguito il lato 'else' dell'if sottostante, dove il programma cerca il primo nodo interessato, sostituisce il contenuto del campo 'data'
                   e procede all'elemento successivo servendosi del campo 'next'.
            2. alcune righe esistono già e ne devono essere create alcune
                -> viene eseguito il lato 'else' dell'if sottostante, dove il programma cerca il primo nodo interessato, sostituisce il contenuto del campo 'data'
                   e procede all'elemento successivo servendosi del campo 'next'. Arrivato al punto in cui servono creare nuovi nodi, esegue lo stesso codice
                   del terzo caso (anche se non implementato in maniera efficiente allo stato attuale - è ridondante).
            3. tutte le righe richieste devono essere create
                -> viene eseguito il lato 'then' dell'if sottostante (considero la generazione di nuovi elementi un caso generale), dove il programma alloca memoria
                   per il nuovo nodo da inserire, aggiorna il suo campo 'data' e procede all'inserimento nell'albero, secondo gli algoritmi forniti durante il corso.
        */
        if(command == 'c' && (ind1 == maxKey + 1)) {            //questo particolare codice viene eseguito se la change avviene senza sovrascrittura di campi 'data' già esistenti
            currentKey = ind1;                                  //salvo la chiave corrente della riga da operare in una nuova variabile (si potrebbe usare ind1 ma diventerebbe incomprensibile)
            RBLastNodeOperated = RBMaxNodeInserted;             //serve ad aiutare nell'assegnare il campo *next

            while(currentKey <= ind2){                          //dovrebbe eseguire ind2 - ind1 +1
                fgets(input, sizeof(input), stdin);             //recupera la stringa da inserire nel campo 'data'
                RBNodeToOperate = newTreeNode(currentKey);      //alloca spazio per un nuovo nodo in memoria e assegna i dati presenti
                textNodeToOperate -> key = currentKey;
                
                for(i = 0; input[i] != '\0'; i++){              //salvataggio della riga sia su memoria testo (RBtree) che su U/R (lista)
                    RBNodeToOperate -> data[i] = input[i];
                    textNodeToOperate -> data[i] = input[i];
                }

                if(RBLastNodeOperated != RBnil && RBNodeToOperate -> key == RBLastNodeOperated -> key + 1){
                    RBLastNodeOperated -> next = RBNodeToOperate;   //eseguito solo dopo almeno la creazione di un nodo
                }

                RBInsert(RBNodeToOperate);
                RBLastNodeOperated = RBNodeToOperate;
                currentKey++;
                maxKey++;

                if(currentKey < ind2){                                      //creo un nuovo nodo per salvare testo in U/R se la riga appena salvata non è l'ultima
                    textNodeToOperate -> nextLine = newTextNode();
                    textNodeToOperate = textNodeToOperate -> nextLine;
                }

                //debug
                debugRoot = root;
            }

            RBMaxNodeInserted = RBLastNodeOperated;
            fgets(input, sizeof(input), stdin);                 //recupera il contenuto della riga che segue alla fine del blocco di righe da leggere (ossia '.\n')
        } else if(command == 'c'){                              //questa parte di codice suppone di dover sovrascrivere campi 'data' già esistenti (cioè ind1 <= maxKey e ind2 > maxKey)
            currentKey = ind1;                                  //se supera maxKey, svolge le stesse operazioni della parte superiore.
            RBNodeToOperate = RBSearch(root, currentKey);
            listNodeToOperate -> isOverwrite = true;            //il nodo di U/R tiene conto del fatto che l'operazione in corso è una sovrascrittura di memoria già esistente

            while(currentKey <= ind2){
                fgets(input, sizeof input, stdin);

                if(currentKey <= maxKey){
                    textNodeToOperate -> key = currentKey;
                    for(i = 0; RBNodeToOperate -> data[i] != '\0'; i++) {       //salvataggio su U/R e wipe del campo 'data' da sostituire
                        textNodeToOperate -> data[i] = RBNodeToOperate -> data[i];
                        RBNodeToOperate -> data[i] = '\0';
                    }

                    textNodeToOperate -> nextLine = newTextNode();
                    textNodeToOperate = textNodeToOperate -> nextLine;

                    for(i = 0; input[i] != '\0'; i++) {                         //salvataggio della nuova stringa carattere per carattere su memoria testo e U/R
                        RBNodeToOperate -> data[i] = input[i];
                        textNodeToOperate -> data[i] = input[i];
                    }                  //la stringa viene copiata carattere per carattere

                    if(currentKey < maxKey) RBNodeToOperate = RBNodeToOperate -> next;
                    else RBLastNodeOperated = RBNodeToOperate;

                    currentKey++;
                } else {
                    RBNodeToOperate = newTreeNode(currentKey);      //alloca spazio per un nuovo nodo in memoria e assegna i dati presenti
                    textNodeToOperate -> key = currentKey;
                    
                    for(i = 0; input[i] != '\0'; i++){              //salvataggio della riga sia su memoria testo (RBtree) che su U/R (lista)
                        RBNodeToOperate -> data[i] = input[i];
                        textNodeToOperate -> data[i] = input[i];
                    }

                    if(RBLastNodeOperated != RBnil && RBNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBNodeToOperate;
                    //eseguito solo dopo almeno la creazione di un nodo
                    RBInsert(RBNodeToOperate);
                    RBLastNodeOperated = RBNodeToOperate;
                    currentKey++;
                    maxKey++;

                    if(currentKey == ind2) RBMaxNodeInserted = RBLastNodeOperated;
                }

                if(currentKey < ind2){
                    textNodeToOperate -> nextLine = newTextNode();
                    textNodeToOperate = textNodeToOperate -> nextLine;
                }

            }

            fgets(input, sizeof(input), stdin);
        }

        /*  -- gestione di 'delete' --
            la condizione iniziale di gestione di una delete permette di ignorare diversi casi particolari che potrebbero occorrere durante l'esecuzione del programma:
            1. ind1 è maggiore rispetto alla chiave massima inserita con una change, di conseguenza anche ind2. Quindi non c'è bisogno di fare nulla;
            2. è ammesso il comando (0,Xd) sui file di test, al quale non corrisponde alcuna reazione da parte del programma (in quanto non esiste una riga di valore '0');
            3. Se non è avvenuta alcuna change oppure una delete precedente ha cancellato tutti i nodi esistenti in precedenza (quindi root == NIL), non è possibile cancellare altri nodi.

            Superati i controlli sopra, si procede a rilevare il caso particolare in cui rientra l'operazione richiesta:
            1. ind1,ind2 <= maxKey - si procede ad una normale delete dei nodi, correzione delle chiavi coinvolte, correzione dei campi *next;
            2. ind1 == 1, ind2 <= maxKey - cancello i nodi, correggo le chiavi coinvolte, non svolgo alcuna operazione sui campi *next;
            3. ind1 <= maxKey && ind1 != 1, ind2 > maxKey - cancello tutti i nodi da ind1 in poi, non correggo le chiavi coinvolte (non ce ne sono) e no
        */
        if(command == 'd'){
            if(ind1 <= maxKey && ind1 >= 1 && root != RBnil) {          //condizione da rispettare perché sia considerata operabile una 'delete', altrimenti U/R salverebbe info sbagliate.
                if(ind1 > 1) RBPredecessor = RBSearch(root, ind1 - 1);  //è necessario salvare il nodo precedente a quello di chiave ind1 per collegare correttamente i campi *next dopo la delete
                else RBPredecessor = RBnil;                             //se ind1 == 1, ovviamente non esiste un RBPredecessore

                if(ind2 <= maxKey) keysToDelete = ind2 - ind1 + 1;      //keysToDelete aiuta a calcolare il nuovo maxKey in seguito alla sequenza di delete
                else keysToDelete = maxKey - ind1 + 1;                  //ovviamente non posso cancellare più nodi di quelli che esistono: pongo un limite a keysToDelete

                RBNodeToOperate = RBSearch(root, ind1);                   //cerco il primo nodo su cui operare
                currentKey = ind1;                                      //aiuta a tenere traccia di quante chiavi devono ancora essere eliminate

                while(currentKey <= ind2 && currentKey <= maxKey){      //svolto finché non raggiunge uno tra ind2 o maxKey (nel caso in cui ind2 > maxKey)
                    RBDelete(RBNodeToOperate);
                    currentKey++;                                       //sono costretto a cercare tutte le volte il nodo successivo perché la struttura dell'albero potrebbe cambiare durante
                    RBNodeToOperate = RBSearch(root, currentKey);         //la precedente delete (a causa delle azioni di RBDeleteFixup())       

                    //debug
                    debugRoot = root;
                }

                if(root != RBnil){                             //se non sono stati cancellati tutti i nodi fino a maxKey, devo sistemare i campi 'key' e *next
                    if(ind1 == 1) fixKeys(RBTreeMinimum(root), 1);              //se ind1 == 1, non serve correggere *next
                    else {
                        RBPredecessor -> next = RBNodeSuccessor(RBPredecessor);     //per sistemare il campo *next è necessario solo assegnare il successore di RBPredecessor al campo *next di RBPredecessor
                        fixKeys(RBPredecessor -> next, ind1);
                    }
                }

                maxKey = maxKey - keysToDelete;
                RBMaxNodeInserted = RBSearch(root, maxKey);   //probabilmente esiste un modo più efficiente per fare questo

                //debug
                debugRoot = root;
            } else listNodeToOperate -> isValid = false;
        }

        if(command == 'p'){
            RBNodeToOperate = RBSearch(root, ind1);          //cerca il primo nodo da stampare
            currentKey = ind1;

            while(currentKey <= ind2) {
                if(RBNodeToOperate != RBnil){
                    for(i = 0; RBNodeToOperate -> data[i] != '\n'; i++) putc(RBNodeToOperate -> data[i], stdout);
                    putc('\n', stdout);     //fputs printa fino al primo '\0', non la soluzione ideale se il nodo da cui si sta stampando è stato creato in seguito ad una delete
                    RBNodeToOperate = RBNodeToOperate -> next;
                    currentKey++;
                } else {fputs(".\n", stdout); currentKey++;}
            }
        }

        if(command == 'u' && urHead -> first != urHead -> current){
            for(i = ind1; i != 0 && urHead -> current != urHead -> first; i--) urHead -> current = urHead -> current -> previous;
        }

        if(command == 'r' && urHead -> last != urHead -> current){
            for(i = ind1; i != 0 && urHead -> current != urHead -> last; i--) urHead -> current = urHead -> current -> following;
        }

        //end-of-cycle
        if(command == 'q') quitted = true;
        else {
            foundComma = false;             //il prossimo ciclo esegue una nuova riga di istruzione, si potrebbe dover rilevare un nuovo comando di tipo C,D,P
            for(i = 0; i < 10; i++){        //reset degli assistenti per il prossimo comando
                inputInd1[i] = '\0';
                inputInd2[i] = '\0';
            }                               //error-prone: potrebbe essere necessario flushare input in caso di rimasugli di precedenti valori non sovrascritti es: input
        }

        //debug
        printf("\n---------\n\n");
        RBInorderTreeWalk(root);
        printf("---\n");
        debugRoot = root;
        if(maxKey < 20) print_t(root);
        fflush(stdout);
    }

    //debug
    fclose(stdin);
    fclose(stdout);

    return 0;
}