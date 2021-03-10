// voto conseguito sul task writeOnly: 5/5
// voto conseguito sul task bulkReads: 5/5
// voto conseguito sul task timeForAChange: 5/5
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
    struct treeNode* next; //aggiunto di recente - per stampa lineare
} treeNode;

struct treeNode T_nil_node;
treeNode* T_nil = &T_nil_node;
treeNode* root;

//allocation
treeNode *newNode(int key, const char* data){
    treeNode* temp = malloc(sizeof(treeNode));
    int i = 0;

    temp -> key = key;
    temp -> color = red;
    temp -> left = T_nil;
    temp -> right = T_nil;
    temp -> father = T_nil;
    temp -> next = T_nil;
    while(data[i] != '\0') {temp -> data[i] = data[i]; i++;}

    return temp;
}

//assistants
void RBLeftRotate(treeNode *x){
    treeNode *y;

    y = x -> right;
    x -> right = y -> left; //il SASX di Y diventa SADX di x

    if(y -> left != T_nil) y -> left -> father = x;

    y -> father = x -> father; //padre di X diventa padre di Y

    if(x -> father == T_nil) root = y;
    else if(x == x -> father -> left) x -> father -> left = y;
    else x -> father -> right = y;

    y -> left = x;
    x -> father = y;
}
void RBRightRotate(treeNode *x){
    treeNode *y;

    y = x -> left;
    x -> left = y -> right;

    if(y -> right != T_nil) y -> right -> father = x;

    y -> father = x -> father;

    if(x -> father == T_nil) root = y;
    else if(x == x -> father -> left) x -> father -> left = y;
    else x -> father -> right = y;

    y -> right = x;
    x -> father = y;
}
treeNode* RBTreeMinimum(treeNode* x){
    while(x -> left != T_nil) x = x -> left;
    return x;
}
treeNode* RBNodeSuccessor(treeNode* x){
    if(x -> right != T_nil) return RBTreeMinimum(x -> right);

    treeNode *y;
    y = x -> father;

    while(y != T_nil && x == y -> right){
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

    y = T_nil;
    x = root;

    while(x != T_nil){
        y = x;

        if(toAdd -> key < x -> key) x = x -> left;
        else x = x -> right;
    }

    toAdd -> father = y;
    if(y == T_nil) root = toAdd;
    else if(toAdd -> key < y -> key) y -> left = toAdd;
    else y -> right = toAdd;

    /*
    toAdd -> color = red;
    toAdd -> left = T_nil;
    toAdd -> right = T_nil;
    */

    //operazioni già eseguite in newNode

    RBInsertFixup(toAdd);
}

//deletion
void RBDeleteFixup(treeNode* x){
    treeNode* w;

    if(x -> color == red || x -> father == T_nil) x -> color = black;
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

    if(toDelete -> left == T_nil || toDelete -> right == T_nil) y = toDelete;
    else y = RBNodeSuccessor(toDelete);

    if(y -> left != T_nil) x = y -> left;
    else x = y -> right;

    x -> father = y -> father;

    if(y -> father == T_nil) root = x;
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

//search and print
treeNode* RBSearch(treeNode* x, int key){
    while(x != T_nil && key != x -> key){
        if(key < x -> key) x = x -> left;
        else x = x -> right;
    }

    return x;
}

//project's specific assistants
void fixKeys(treeNode* x, int newKey){   //cambia le chiavi dei nodi per renderle incrementali, in seguito ad una delete
    while(x != T_nil){
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

    if(x != T_nil){
        RBInorderTreeWalk(x -> left);
        printf("\n(%d", x -> key);
        if(x -> color == red) printf(" - R)  ");
        else printf(" - B)  ");
        for(i = 0; x -> data[i] != '\n'; i++) putc(x -> data[i], stdout);
        putc('\n', stdout);
        RBInorderTreeWalk(x -> right);
    }
}

int main(){
    int ind1;
    int ind2;
    int currentKey;              //salva la chiave corrente del nodo da trattare
    int maxKey = 0;              //registra il numero massimo inserito in ind2
    int keysToDelete;            //registra il numero di chiavi da cancellare
    int i = 0;                   //assiste nella lettura della stringa "input" e nella sostituzione del contenuto dei campi 'data'
    int j = 0;                   //assiste nella scrittura delle stringhe "inputInd1" e "inputInd2"
    char inputInd1[10];          //assistente alla lettura del primo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char inputInd2[10];          //assistente alla lettura del secondo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char input[1024];            //multi-purpose string che registra comandi da decodificare e salva temporaneamente i dati da inserire nella struttura dati
    char command;                //contiene il comando da eseguire in seguito alla lettura di una riga di istruzione
    bool foundComma = false;     //aiuta a distinguere i comandi U,R,Q da C,D,P
    bool quitted = false;        //consente di uscire dal ciclo while principale del programma
    treeNode* nodeToOperate;             //si vede assegnato l'indirizzo del nodo che sarà coinvolto in una qualunque delle istruzioni richieste
    treeNode* lastNodeOperated = T_nil;  //aiuta a garantire linearità nella stesura dell'albero (per impiegare meno tempo a cercare il successivo nodo da trattare)
    treeNode* maxNodeInserted = T_nil;   //in concerto con maxKey, aiuta a garantire linearità nella stesura dell'albero, in caso di change consecutive nel caso ind1 = maxKey + 1
    treeNode* predecessor = T_nil;       //assiste nel correggere il campo 'next' dei nodi indirettamente coinvolti in una delete

    root = T_nil;                //inizializzazioni necessarie a far sì che le assunzioni delle parti di codice siano rispettate in qualunque momento
    T_nil -> key = 0;            //es: per T_nil ci si aspetta sempre che la sua chiave sia 0 e il suo colore sia black.
    T_nil -> color = black;      //    Ciò non viene stabilito subito dopo la sua dichiarazione.

    //debug
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);
    int numOperazione = 0;
    treeNode* debugRoot;

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
        numOperazione++;
        if(command == 'q') printf("[n. %d] %c", numOperazione, command);
        else if(foundComma == true) printf("[n. %d] %d,%d %c\n\n", numOperazione, ind1, ind2, command);
        else printf("[n. %d] %d %c\n\n", numOperazione, ind1, command);
        fflush(stdout);

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
            lastNodeOperated = maxNodeInserted;                 //serve ad aiutare nell'assegnare il campo *next

            while(currentKey <= ind2){                          //dovrebbe eseguire ind2 - ind1 +1
                fgets(input, sizeof(input), stdin);             //recupera la stringa da inserire nel campo 'data'
                nodeToOperate = newNode(currentKey, input);     //alloca spazio per un nuovo nodo in memoria e assegna i dati presenti
                if(lastNodeOperated != T_nil && nodeToOperate -> key == lastNodeOperated -> key + 1){
                    lastNodeOperated -> next = nodeToOperate;   //eseguito solo dopo almeno la creazione di un nodo
                }
                RBInsert(nodeToOperate);
                lastNodeOperated = nodeToOperate;
                currentKey++;
                maxKey++;

                //debug
                debugRoot = root;
            }

            maxNodeInserted = lastNodeOperated;
            fgets(input, sizeof(input), stdin);                 //recupera il contenuto della riga che segue alla fine del blocco di righe da leggere (ossia '.\n')
        } else if(command == 'c'){                              //questa parte di codice suppone di dover sovrascrivere campi 'data' già esistenti (cioè ind1 <= maxKey e ind2 > maxKey)
            currentKey = ind1;                                  //se supera maxKey, svolge le stesse operazioni della parte superiore.
            nodeToOperate = RBSearch(root, currentKey);

            while(currentKey <= ind2){
                fgets(input, sizeof input, stdin);

                if(currentKey <= maxKey){
                    for(i = 0; nodeToOperate -> data[i] != '\0'; i++) nodeToOperate -> data[i] = '\0';      //reset del campo "data" per accettare la nuova stringa
                    for(i = 0; input[i] != '\0'; i++) nodeToOperate -> data[i] = input[i];                  //la stringa viene copiata carattere per carattere

                    if(currentKey < maxKey) nodeToOperate = nodeToOperate -> next;
                    else lastNodeOperated = nodeToOperate;

                    currentKey++;
                } else {
                    nodeToOperate = newNode(currentKey, input);     //alloca spazio per un nuovo nodo in memoria e assegna i dati presenti

                    if(lastNodeOperated != T_nil && nodeToOperate -> key == lastNodeOperated -> key + 1) lastNodeOperated -> next = nodeToOperate;
                    //eseguito solo dopo almeno la creazione di un nodo
                    RBInsert(nodeToOperate);
                    lastNodeOperated = nodeToOperate;
                    currentKey++;
                    maxKey++;

                    if(currentKey == ind2) maxNodeInserted = lastNodeOperated;
                }
            }

            fgets(input, sizeof(input), stdin);
        }

        if(command == 'd' && ind1 <= maxKey && ind1 >= 1 && root != T_nil){
            if(ind1 > 1) predecessor = RBSearch(root, ind1 - 1);
            else predecessor = T_nil;

            if(ind2 <= maxKey) keysToDelete = ind2 - ind1 + 1;
            else keysToDelete = maxKey - ind1 + 1;

            nodeToOperate = RBSearch(root, ind1);
            currentKey = ind1;

            while(currentKey <= ind2 && currentKey <= maxKey){
                RBDelete(nodeToOperate);
                currentKey++;                                       //in questo caso non posso sfruttare 'next' perché la struttura dell'albero
                nodeToOperate = RBSearch(root, currentKey);     //potrebbe cambiare rompendo i legami tra puntatori 'next' durante una delete.

                //debug
                debugRoot = root;
            }

            if(root != T_nil && ind2 < maxKey){
                if(ind1 == 1) fixKeys(RBTreeMinimum(root), 1);
                else {
                    predecessor -> next = RBNodeSuccessor(predecessor);
                    fixKeys(predecessor -> next, ind1);
                }
            }

            maxKey = maxKey - keysToDelete;
            maxNodeInserted = RBSearch(root, maxKey);   //probabilmente esiste un modo più efficiente per fare questo

            //debug
            debugRoot = root;
        }

        if(command == 'p'){
            nodeToOperate = RBSearch(root, ind1);          //cerca il primo nodo da stampare
            currentKey = ind1;

            while(currentKey <= ind2) {
                if(nodeToOperate != T_nil){
                    for(i = 0; nodeToOperate -> data[i] != '\n'; i++) putc(nodeToOperate -> data[i], stdout);
                    putc('\n', stdout);  //fputs printa fino al primo '\0', non la soluzione ideale se il nodo da cui si sta stampando è stato creato in seguito ad una delete
                    fflush(stdout);         //potrebbero essere rimasti rimasugli di memoria precedente alla malloc considerati erroneamente di proprietà del campo 'data'
                    nodeToOperate = nodeToOperate -> next;
                    currentKey++;
                } else {fputs(".\n", stdout); fflush(stdout); currentKey++;}
            }
        }

        //end-of-cycle
        if(command == 'q') quitted = true;
        else {
            foundComma = false;             //il prossimo ciclo esegue una nuova riga di istruzione, si potrebbe dover rilevare un nuovo comando di tipo C,D,P
            for(i = 0; i < 10; i++){    //reset degli assistenti per il prossimo comando
                inputInd1[i] = '\0';
                inputInd2[i] = '\0';
            }                           //error-prone: potrebbe essere necessario flushare input in caso di rimasugli di precedenti valori non sovrascritti es: input
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
