// voto conseguito sul task writeOnly: 5/5
// voto conseguito sul task bulkReads: 5/5
// voto conseguito sul task timeForAChange: 5/5
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
#define snapTime 10

typedef enum{red, black} color;
typedef char bool;
typedef struct treeNode{
    int key;
    char* data;
    color color;
    struct treeNode* father;
    struct treeNode* left;
    struct treeNode* right;
    struct treeNode* next; //aggiunto di recente - per stampa lineare
} treeNode;
typedef struct textNode{            //suppongo di creare un nuovo textNode per ogni nuova riga di testo inserita/cambiata/eliminata
    int key;                        //salva il numero di riga (sfruttato per identificare una lista composta di nodi soggetti a overwrite, piuttosto che creation)
    char* data;                     //(non sapendo a priori quanto spazio serve)
    struct textNode* nextLine;
} textNode;
typedef struct listNode{            //ogni nodo si intende come SNAPSHOT degli elementi che variano all'interno della struttura
    int opCode;
    int beginKey;                   //salva ind1 dell'operazione associata
    int endKey;                     //salva ind2 dell'operazione associata
    char command;                   //salva command dell'operazione associata
    bool isValid;                   //determina se l'operazione salvata ha effettivamente variato la struttura dati o meno
    bool isOverwrite;               //se command == 'c', indica che l'operazione salvata è di overwrite (ossia sovrascrittura di nodi già esistenti)
    bool gotSnapshot;
    bool reachesMaxKey;
    int addedAfterOverwriting;      //se command == 'c && addedAfterOverwriting != 0 allora il "aao" la chiave del primo nodo creato dopo aver finito la overwrite
    int keysToDelete;               //se command == 'd', indica il numero di chiavi che cancellate effettivamente (potrei trovarlo ind2 - ind1 + 1 però diventa difficile se ind2 > maxKeyBO)
    struct treeNode* root;          //puntatore alla radice della snapshot associata al nodo di listNode;
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
    temp -> root = RBnil;

    temp -> isValid = true;
    temp -> isOverwrite = false;
    if(command == 'd' || opCode % snapTime == 0) temp -> gotSnapshot = true;
    else temp -> gotSnapshot = false;
    temp -> reachesMaxKey = false;
    temp -> addedAfterOverwriting = 0;
    temp -> keysToDelete = 0;

    return temp;
}
textNode* newTextNode(){
    textNode* temp = malloc(sizeof(textNode));
    temp -> nextLine = Tnil;

    return temp;
}
void initializeStructures(){
    RBnil -> key = 0;              //es: per RBnil ci si aspetta sempre che la sua chiave sia 0 e il suo colore sia black.
    RBnil -> color = black;        //    Ciò non viene stabilito subito dopo la sua dichiarazione.

    URnil -> opCode = 0;
    URnil -> beginKey = 0;
    URnil -> endKey = 0;
    URnil -> command = '\0';
    URnil -> root = RBnil;

    urHead = malloc(sizeof(listHead));
    urHead -> totalOperations = 0;
    urHead -> first = URnil;
    urHead -> operator = URnil;
    urHead -> current = URnil;
    urHead -> last = URnil;
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
    int i;
    int size;

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
        //for(i = 0; toDelete -> data[i] != '\0'; i++) toDelete -> data[i] = '\0';
        size = strlen(y -> data);
        toDelete -> data = realloc(toDelete -> data, size * sizeof(char));
        for(i = 0; i < size; i++) toDelete -> data[i] = y -> data[i];
        toDelete -> next = y -> next;
    }
    if(y -> color == black) root = RBDeleteFixup(root, x);

    free(y -> data);
    free(y);

    return root;
}

//search and print
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
        x = x -> next;
        newKey++;
    }
}
treeNode* cloneTree(treeNode* treeToClone){
    if(treeToClone == RBnil) return RBnil;

    treeNode* RBNodeToOperate = RBTreeMinimum(treeToClone);
    treeNode* RBDestNodeToOperate;
    treeNode* destRoot = RBnil;
    int size;

    while(RBNodeToOperate != RBnil){
        RBDestNodeToOperate = newTreeNode(RBNodeToOperate -> key);
        size = strlen(RBNodeToOperate -> data);
        RBDestNodeToOperate -> data = malloc(size * sizeof(char));
        for(int i = 0; i < size; i++) RBDestNodeToOperate -> data[i] = RBNodeToOperate -> data[i];

        destRoot = RBInsert(destRoot, RBDestNodeToOperate);

        RBNodeToOperate = RBNodeToOperate -> next;
    }

    return destRoot;
}

int main(){
    int ind1;
    int ind2;
    int currentKey;              //salva la chiave corrente del nodo da trattare
    int maxKey = 0;              //registra il numero massimo inserito in ind2
    int keysToDelete;            //registra il numero di chiavi da cancellare
    int numOperazione = 0;
    int i;                   //assiste nella lettura della stringa "input" e nella sostituzione del contenuto dei campi 'data'
    int j;                   //assiste nella scrittura delle stringhe "inputInd1" e "inputInd2"
    int size;
    char inputInd1[10];          //assistente alla lettura del primo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char inputInd2[10];          //assistente alla lettura del secondo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char input[1024];            //multi-purpose string che registra comandi da decodificare e salva temporaneamente i dati da inserire nella struttura dati
    char command;                //contiene il comando da eseguire in seguito alla lettura di una riga di istruzione
    bool foundComma = false;     //aiuta a distinguere i comandi U,R,Q da C,D,P
    bool quitted = false;        //consente di uscire dal ciclo while principale del programma
    treeNode* RBroot = RBnil;
    treeNode* RBNodeToOperate;             //si vede assegnato l'indirizzo del nodo che sarà coinvolto in una qualunque delle istruzioni richieste
    treeNode* RBLastNodeOperated = RBnil;  //aiuta a garantire linearità nella stesura dell'albero (per impiegare meno tempo a cercare il successivo nodo da trattare)
    treeNode* RBMaxNodeOperated = RBnil;   //in concerto con maxKey, aiuta a garantire linearità nella stesura dell'albero, in caso di change consecutive nel caso ind1 = maxKey + 1
    treeNode* RBPredecessor = RBnil;       //assiste nel correggere il campo 'next' dei nodi indirettamente coinvolti in una delete
    treeNode* RBDestNodeToOperate;
    listNode* listNodeToOperate;
    textNode* textNodeToOperate;

    initializeStructures();

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
        /* -- gestione preliminare di undo/redo --
           verifico se la struttura dedicata a undo/redo presenta già degli elementi e procedo alle opportune allocazioni
        */
        if(command == 'c' || command == 'd') numOperazione++;
        if(command != 'u' && command != 'r' && command != 'q'){
            if(urHead -> current != urHead -> last && command == 'c'){
                //operazioni per cancellare gli elementi della lista, prima di eseguire una change
            }

            if(urHead -> current != urHead -> last && command == 'd'){
                //operazioni per cancellare gli elementi della lista, prima di eseguire una delte
            }

            if(urHead -> current != urHead -> last && command == 'p'){
                //operazioni per annullare delle operazioni, senza cancellare i nodi della lista
            }

            if(command != 'p'){
                if(urHead -> last == URnil){
                    listNodeToOperate = newListNode(ind1, ind2, command, numOperazione);    //supponendo che la nuova struttura dati tenga conto
                    textNodeToOperate = newTextNode();
                    listNodeToOperate -> data = textNodeToOperate;

                    urHead -> first = listNodeToOperate;
                    urHead -> operator = listNodeToOperate;
                    urHead -> current = listNodeToOperate;
                    urHead -> last = listNodeToOperate;
                    urHead -> totalOperations++;
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
        }

        //if(listNodeToOperate -> gotSnapshot == true) listNodeToOperate -> root = cloneTree(RBroot);       /*consuma troppo tempo */
        if(listNodeToOperate -> gotSnapshot == true){
            if(RBroot != RBnil) {
                while(RBNodeToOperate != RBnil){
                    RBDestNodeToOperate = newTreeNode(RBNodeToOperate -> key);
                    size = strlen(RBNodeToOperate -> data);
                    RBDestNodeToOperate -> data = malloc(size * sizeof(char));
                    for(int i = 0; i < size; i++) RBDestNodeToOperate -> data[i] = RBNodeToOperate -> data[i];

                    listNodeToOperate -> root = RBInsert(listNodeToOperate -> root, RBDestNodeToOperate);

                    RBNodeToOperate = RBNodeToOperate -> next;
                }
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
            RBLastNodeOperated = RBMaxNodeOperated;             //aiutare nell'assegnare il campo *next

            while(currentKey <= ind2){                          //dovrebbe eseguire ind2 - ind1 +1
                fgets(input, sizeof(input), stdin);             //recupera la stringa da inserire nel campo 'data'

                RBNodeToOperate = newTreeNode(currentKey);      //alloca spazio per un nuovo nodo in memoria e assegna i dati presenti
                textNodeToOperate -> key = currentKey;

                size = strlen(input);                                       //determino e alloco lo spazio minimo necessario a salvare il testo nei nuovi nodo e textNode
                RBNodeToOperate -> data = malloc(size * sizeof(char));
                textNodeToOperate -> data = malloc(size * sizeof(char));
                for(i = 0; i < size; i++) {
                    RBNodeToOperate -> data[i] = input[i];
                    textNodeToOperate -> data[i] = input[i];
                }

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
            fgets(input, sizeof(input), stdin);                 //recupera il contenuto della riga che segue alla fine del blocco di righe da leggere (ossia '.\n')
        } else if(command == 'c'){                              //questa parte di codice suppone di dover sovrascrivere campi 'data' già esistenti (cioè ind1 <= maxKey e ind2 > maxKey)
            currentKey = ind1;                                  //se supera maxKey, svolge le stesse operazioni della parte superiore.
            RBNodeToOperate = RBSearch(RBroot, currentKey);
            listNodeToOperate -> isOverwrite = true;            //indico al nodo di U/R che sta salvando informazioni relative ad un'operazione di overwrite di nodi già esistenti
            if(ind2 > maxKey) listNodeToOperate -> addedAfterOverwriting = maxKey + 1;

            while(currentKey <= ind2){
                fgets(input, sizeof input, stdin);

                if(currentKey <= maxKey){
                    textNodeToOperate -> key = currentKey;

                    size = strlen(RBNodeToOperate -> data);                 //determino e alloco lo spazio minimo necessario a salvare il contenuto vecchio del nodo esistente
                    textNodeToOperate -> data = malloc(size * sizeof(char));
                    for(i = 0; i < size; i++) textNodeToOperate -> data[i] = RBNodeToOperate -> data[i];

                    size = strlen(input);                                   //determino e alloco lo spazio minimo per salvare il nuovo contenuto su nodo esistente e su textNode appena creato
                    RBNodeToOperate -> data = realloc(RBNodeToOperate -> data, size * sizeof(char));
                    for(i = 0; input[i] != '\0'; i++) RBNodeToOperate -> data[i] = input[i];

                    if(currentKey < maxKey) RBNodeToOperate = RBNodeToOperate -> next;      //se sto operando su un range di nodi che esistono già, sfrutto il campo *next per accedere al successivo
                    else RBLastNodeOperated = RBNodeToOperate;                              //altrimenti imposto le condizioni necessarie per operare su nodi nuovi

                    if(currentKey < ind2){
                        textNodeToOperate -> nextLine = newTextNode();
                        textNodeToOperate = textNodeToOperate -> nextLine;
                    }

                    currentKey++;
                } else {                                           //stesse istruzioni del ramo 'true' del primo 'if' legato all'istruzione 'change'
                    RBNodeToOperate = newTreeNode(currentKey);     //alloco spazio per un nuovo nodo in memoria
                    textNodeToOperate -> key = currentKey;

                    size = strlen(input);                                       //determino e alloco spazio per i campi 'data' di nodo e textNode associato
                    RBNodeToOperate -> data = malloc(size * sizeof(char));
                    textNodeToOperate -> data = malloc(size * sizeof(char));
                    for(i = 0; i < size; i++) {                                 //assegno i campi 'data' carattere per carattere
                        RBNodeToOperate -> data[i] = input[i];
                        textNodeToOperate -> data[i] = input[i];
                    }

                    if(RBLastNodeOperated != RBnil && RBNodeToOperate -> key == RBLastNodeOperated -> key + 1) RBLastNodeOperated -> next = RBNodeToOperate;
                    //eseguito solo dopo almeno la creazione di un nodo
                    RBroot = RBInsert(RBroot, RBNodeToOperate);
                    RBLastNodeOperated = RBNodeToOperate;

                    if(currentKey < ind2){
                        textNodeToOperate -> nextLine = newTextNode();
                        textNodeToOperate = textNodeToOperate -> nextLine;
                    }

                    currentKey++;
                    maxKey++;

                    if(currentKey == ind2) RBMaxNodeOperated = RBLastNodeOperated;
                }
            }

            fgets(input, sizeof(input), stdin);
        }

        if(command == 'd'){
            if(ind1 <= maxKey && ind1 >= 1 && RBroot != RBnil){
                if(ind1 > 1) RBPredecessor = RBSearch(RBroot, ind1 - 1);
                else RBPredecessor = RBnil;

                if(ind2 <= maxKey) keysToDelete = ind2 - ind1 + 1;
                else {
                    keysToDelete = maxKey - ind1 + 1;
                    listNodeToOperate -> reachesMaxKey = true;
                }
                listNodeToOperate -> keysToDelete = keysToDelete;

                RBNodeToOperate = RBSearch(RBroot, ind1);
                currentKey = ind1;

                while(currentKey <= ind2 && currentKey <= maxKey){
                    size = strlen(RBNodeToOperate -> data);
                    textNodeToOperate -> data = malloc(size * sizeof(char));
                    for(i = 0; i < size; i++) textNodeToOperate -> data[i] = RBNodeToOperate -> data[i];
                    textNodeToOperate -> key = currentKey;

                    RBroot = RBDelete(RBroot, RBNodeToOperate);

                    currentKey++;                                       //in questo caso non posso sfruttare 'next' perché la struttura dell'albero
                    RBNodeToOperate = RBSearch(RBroot, currentKey);     //potrebbe cambiare rompendo i legami tra puntatori 'next' durante una delete.

                    if(currentKey <= ind2 && currentKey <= maxKey){
                        textNodeToOperate -> nextLine = newTextNode();
                        textNodeToOperate = textNodeToOperate -> nextLine;
                    }
                }

                if(RBroot != RBnil){
                    if(ind1 == 1) fixKeys(RBTreeMinimum(RBroot), 1);
                    else {
                        RBPredecessor -> next = RBNodeSuccessor(RBPredecessor);
                        fixKeys(RBPredecessor -> next, ind1);
                    }
                }

                maxKey = maxKey - keysToDelete;
                RBMaxNodeOperated = RBSearch(RBroot, maxKey);   //probabilmente esiste un modo più efficiente per fare questo
            } else listNodeToOperate -> isValid = false;
        }

        if(command == 'p'){
            RBNodeToOperate = RBSearch(RBroot, ind1);          //cerca il primo nodo da stampare
            currentKey = ind1;

            while(currentKey <= ind2) {
                if(RBNodeToOperate != RBnil){
                    for(i = 0; RBNodeToOperate -> data[i] != '\n'; i++) putc(RBNodeToOperate -> data[i], stdout);
                    putc('\n', stdout);  //fputs printa fino al primo '\0', non la soluzione ideale se il nodo da cui si sta stampando è stato creato in seguito ad una delete
                    RBNodeToOperate = RBNodeToOperate -> next;
                    currentKey++;
                } else {
                    putc_unlocked('.', stdout);
                    putc_unlocked('\n', stdout);
                    currentKey++;
                }
            }
        }

        //end-of-cycle
        if(command == 'q') quitted = true;
        else {
            foundComma = false;             //il prossimo ciclo esegue una nuova riga di istruzione, si potrebbe dover rilevare un nuovo comando di tipo C,D,P
            for(i = 0; i < 10; i++){    //reset degli assistenti per il prossimo comando
                inputInd1[i] = '\0';
                inputInd2[i] = '\0';
            }
        }
    }
    return 0;
}