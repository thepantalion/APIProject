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

struct treeNode RBnil_node;
treeNode* RBnil = &RBnil_node;
treeNode* root;
treeNode* maxNodeInserted;
treeNode* lastNodeOperated;
int maxKey;

//allocation
treeNode *newNode(int key){
    treeNode* temp = malloc(sizeof(treeNode));
    int i = 0;

    temp -> key = key;
    temp -> color = red;
    temp -> left = RBnil;
    temp -> right = RBnil;
    temp -> father = RBnil;
    temp -> next = RBnil;
    //while(data[i] != '\0') {temp -> data[i] = data[i]; i++;}

    return temp;
}
void initializeStructures(){
    root = RBnil;                //inizializzazioni necessarie a far sì che le assunzioni delle parti di codice siano rispettate in qualunque momento
    maxNodeInserted = RBnil;
    lastNodeOperated = RBnil;
    RBnil -> key = 0;            //es: per RBnil ci si aspetta sempre che la sua chiave sia 0 e il suo colore sia black.
    RBnil -> color = black;      //    Ciò non viene stabilito subito dopo la sua dichiarazione.
    maxKey = 0;

    /*
    URnil -> opCode = 0;
    URnil -> beginKey = 0;
    URnil -> endKey = 0;
    URnil -> command = '\0';

    urHead = malloc(sizeof(listHead));
    urHead -> totalOperations = 0;
    urHead -> first = URnil;
    urHead -> operator = URnil;
    urHead -> current = URnil;
    urHead -> last = URnil; */
}

//assistants
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

//change
void changeNewLines(int ind1, int ind2, bool freshCall){
    int currentKey = ind1;
    int i;
    char input;
    treeNode* nodeToOperate;

    if(freshCall == true) lastNodeOperated = maxNodeInserted;

    while(currentKey <= ind2){
        input = getc_unlocked(stdin);

        nodeToOperate = newNode(currentKey);
        if(lastNodeOperated != RBnil && nodeToOperate -> key == lastNodeOperated -> key + 1) lastNodeOperated -> next = nodeToOperate;
        for(i = 0; input != '\n'; i++){
            nodeToOperate -> data[i] = input;
            input = getc_unlocked(stdin);

            if(input == '\n') nodeToOperate -> data[i + 1] = input;
        }
        RBInsert(nodeToOperate);
        lastNodeOperated = nodeToOperate;
        currentKey++;
        maxKey++;
    }

    maxNodeInserted = lastNodeOperated;
}
void changeOverwrite(int ind1, int ind2){
    int currentKey = ind1;
    int i;
    char input;
    treeNode* nodeToOperate = RBSearch(root, currentKey);

    while(currentKey <= ind2){
        if(currentKey <= maxKey){
            input = getc_unlocked(stdin);

            for(i = 0; nodeToOperate -> data[i] != '\0'; i++) nodeToOperate -> data[i] = '\0';      //reset del campo "data" per accettare la nuova stringa
            for(i = 0; input != '\n'; i++) {
                nodeToOperate -> data[i] = input;                                                   //la stringa viene copiata carattere per carattere
                input = getc_unlocked(stdin);

                if(input == '\n') nodeToOperate -> data[i + 1] = input;
            }

            if(currentKey < maxKey) nodeToOperate = nodeToOperate -> next;
            else lastNodeOperated = nodeToOperate;

            currentKey++;
        } else {
            changeNewLines(currentKey, ind2, false);
            currentKey = maxKey + 1;
        }
    }
}

//delete
void deleteText(int ind1, int ind2){
    int currentKey = 1;
    int keysToDelete;
    treeNode* nodeToOperate;
    treeNode* predecessor;
    treeNode* debugRoot;

    if(ind1 > 1) predecessor = RBSearch(root, ind1 - 1);
    else predecessor = RBnil;

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

    if(root != RBnil){                             //se non sono stati cancellati tutti i nodi fino a maxKey, devo sistemare i campi 'key' e *next
        if(ind1 == 1) fixKeys(RBTreeMinimum(root), 1);              //se ind1 == 1, non serve correggere *next
        else {
            predecessor -> next = RBNodeSuccessor(predecessor);     //per sistemare il campo *next è necessario solo assegnare il successore di RBPredecessor al campo *next di RBPredecessor
            fixKeys(predecessor -> next, ind1);
        }
    }

    maxKey = maxKey - keysToDelete;
    maxNodeInserted = RBSearch(root, maxKey);   //probabilmente esiste un modo più efficiente per fare questo

    //debug
    debugRoot = root;
}

//print
void printText(int ind1, int ind2){
    int currentKey = ind1;
    int i;
    treeNode* nodeToOperate = RBSearch(root, ind1);

    while(currentKey <= ind2) {
        if(nodeToOperate != RBnil){
            for(i = 0; nodeToOperate -> data[i] != '\n'; i++) putc_unlocked(nodeToOperate -> data[i], stdout);
            putc_unlocked('\n', stdout);
            nodeToOperate = nodeToOperate -> next;
            currentKey++;
        } else {
            putc_unlocked('.', stdout);
            putc_unlocked('\n', stdout);
            currentKey++;
        }
    }
}

int main(){
    int ind1;
    int ind2;
    int i = 0;                   //assiste nella lettura della stringa "input" e nella sostituzione del contenuto dei campi 'data'
    char inputInd1[10];          //assistente alla lettura del primo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char inputInd2[10];          //assistente alla lettura del secondo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char input = '0';                  //multi-purpose char che registra comandi da decodificare e salva temporaneamente i dati da inserire nella struttura dati
    char command;                //contiene il comando da eseguire in seguito alla lettura di una riga di istruzione
    bool foundComma = false;     //aiuta a distinguere i comandi U,R,Q da C,D,P
    bool quitted = false;        //consente di uscire dal ciclo while principale del programma
    //treeNode* lastNodeOperated = RBnil;  //aiuta a garantire linearità nella stesura dell'albero (per impiegare meno tempo a cercare il successivo nodo da trattare)
    //treeNode* maxNodeInserted = RBnil;   //in concerto con maxKey, aiuta a garantire linearità nella stesura dell'albero, in caso di change consecutive nel caso ind1 = maxKey + 1

    initializeStructures();

    while(quitted == false){
        input = getc_unlocked(stdin);
        i = 0;

        /*  -- gestione dell'input --
           'fgets' inserisce nella stringa multi-purpose "input" il comando da eseguire.
           per ipotesi, l'input è sempre corretto, quindi ci si aspetta sempre, all'inizio, un comando seguito dal carattere '\n'
           (che fgets cerca per poi terminare l'inserimento della riga nella stringa "input").

           la stringa "input" viene fatta scorrere alla ricerca degli indirizzi e del comando necessari ad accedere correttamente alla struttura.
       */
        if(input == 'q') command = 'q';
        else {
            for(i = 0; input >= '0' && input <= '9'; i++){
                inputInd1[i] = input;
                input = getc_unlocked(stdin);
            }
            ind1 = strtol(inputInd1, NULL, 10);

            if(input == ','){
                foundComma = true;
                input = getc_unlocked(stdin);

                for(i = 0; input >= '0' && input <= '9'; i++){
                    inputInd2[i] = input;
                    input = getc_unlocked(stdin);
                }
                ind2 = strtol(inputInd2, NULL, 10);
            }

            command = input;
            while(input != '\n') input = getc_unlocked(stdin);
        }

        if(command == 'c'){
            if(ind1 == maxKey + 1) changeNewLines(ind1, ind2, true);
            else changeOverwrite(ind1, ind2);

            input = getc_unlocked(stdin);     //cattura '.' che segue la lista di righe inserite
            input = getc_unlocked(stdin);     //cattura '\n' che segue il punto terminatore
        }

        if(command == 'd') {
            if(ind1 <= maxKey && ind1 >= 1 && root != RBnil) deleteText(ind1, ind2);
            else {}
        }

        if(command == 'p') printText(ind1, ind2);

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