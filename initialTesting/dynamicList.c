#include <stdio.h>
#include <stdlib.h>
#define DIM 1024

typedef enum{red, black} color;
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
    char data[DIM];                 //(non sapendo a priori quanto spazio serve)
    struct textNode* nextLine;
} textNode;
typedef struct listNode{            //ogni nodo si intende come SNAPSHOT degli elementi che variano all'interno della struttura
    int beginKey;                   //salva ind1 dell'operazione associata
    int endKey;                     //salva ind2 dell'operazione associata
    char command;                   //salva command dell'operazione associata
    textNode* data;                 //puntatore al primo nodo della struttura textNode
    struct listNode* following;     //puntatore all'istruzione precedente (il nodo è considerato 'meno recente' se following == URnil)
    struct listNode* previous;      //puntatore all'istruzione successiva (il nodo è considerato 'più recente' se following == URnil)
} listNode;
typedef struct listHead{
    listNode* operator;             //puntatore che segnala quale snapshot è stata annullata/ripristinata sulla struttura dati
    listNode* current;              //puntatore che indica, in base al numero di undo/redo, il limite di operatività di operator, in caso di print, change o delete
    listNode* last;                 //puntatore che indica l'ultimo elemento inserito (avanza solo quando aggiungo elementi, retrocede solo quando avvengono change o delete e current != last)
} listHead;

struct treeNode RBnil_node;         //nil_node per Red-Black tree
treeNode* RBnil = &RBnil_node;
struct listNode URnil_node;         //nil_node per lista dinamica
listNode* URnil = &URnil_node;
struct textNode TEXTnil_node;       //nil_node per la lista dinamica di testo all'interno di un nodo della lista UR
textNode* Tnil = &TEXTnil_node;

treeNode* root;
listHead* urHead;

void initializeStructures(){
    root = RBnil;                //inizializzazioni necessarie a far sì che le assunzioni delle parti di codice siano rispettate in qualunque momento
    RBnil -> key = 0;            //es: per RBnil ci si aspetta sempre che la sua chiave sia 0 e il suo colore sia black.
    RBnil -> color = black;      //    Ciò non viene stabilito subito dopo la sua dichiarazione.

    urHead = malloc(sizeof(listHead));
    urHead -> operator = URnil;
    urHead -> current = URnil;
    urHead -> last = URnil;
}

int main(){
    

    return 0;
}