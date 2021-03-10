#include <stdlib.h>
#include <stdio.h>

typedef struct student{
    char nome[20];
    char cognome[20];
    int numeroMatricola;
} student;
typedef struct list{
    struct student* data;   //contiene i dati di un singolo studente
    struct list* nextNode;  //punta al nodo dello studente successivo. 
} list;                     //Se non esiste un elemento successivo, punta a NULL

student* immetti_dati(){
    student* temp = malloc(sizeof(student));

    scanf("%s", temp -> nome);
    scanf("%s", temp -> cognome);
    scanf("%d", &temp -> numeroMatricola);

    return temp;
}

void inserisci_in_lista(list* list, student* student){
    struct list* tempNode;
    int found = 0;

    while(student -> numeroMatricola < list -> data -> numeroMatricola && found == 0){
        if(student -> numeroMatricola < list -> data -> numeroMatricola) list = list -> nextNode;
        else {
            tempNode = malloc(sizeof(list));
            tempNode -> data = student;
            tempNode -> nextNode = list -> nextNode;
            list -> nextNode = tempNode;
            found = 1;
        }
    }
}

void stampa_lista(list* list){
    if(list == NULL) printf("La lista è vuota...\n\n");
    else {
        while(list != NULL){
            printf("NOME: ");
            fputs(list -> data -> nome, stdin);
            printf("\nCOGNOME: ");
            fputs(list -> data -> cognome, stdin);
            printf("\nNUMERO MATRICOLA: %d", list -> data -> numeroMatricola);

            list = list -> nextNode;
        }
    }
}

int main(){
    int lato;
    int i, j;

    do scanf("%d", &lato); 
    while(lato < 3 || lato > 20);

    for(i = 1; i <= lato; i++){
        if(i == 1 || i == lato){
            for(j = 1; j <= lato; j++){
                printf("*");
                if(j == lato) printf("\n");
            }
        } else {
            for(j = 1; j <= lato; j++){
                if(j == 1) printf("*");
                else if(j == lato) printf("*\n" );
                else printf(" ");
            }
        }
    }

    student* newStud = immetti_dati();

    printf("\n\n");
}