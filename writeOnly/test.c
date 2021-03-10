#include <stdio.h>
#include <stdlib.h>

int main(){                 
    char inputInd1[10];     //assistente alla lettura del primo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    char inputInd2[10];     //assistente alla lettura del secondo indirizzo (dim == 20 in quanto un numero intero senza segno rappresentato a 64bit contiene al massimo 20 cifre)
    unsigned int ind1;
    unsigned int ind2;      
    int i = 0;              //assiste nella lettura della stringa "input"
    int j = 0;              //assiste nella scrittura delle stringhe "inputInd1" e "inputInd2"
    char input[1024];       //multi-purpose string che registra comandi da decodificare e salva temporaneamente i dati da inserire nella struttura dati
    char command;           //contiene il comando da eseguire in seguito alla lettura di una riga di istruzione
    int foundComma = 0;     //aiuta a distinguere i comandi U,R,Q da C,D,P
    int quitted = 0;        //consente di uscire dal ciclo while principale del programma

    while(quitted != 4){
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
            ind1 = atoi(inputInd1);         //atoi(char*) permette la conversione da stringa a intero di un numero contenuto nella stringa "inputInd1"

            if(input[i] == ','){            //nel caso di C,D,P ci si aspetta il formato (ind1,ind2command): trovare una virgola dopo il primo numero implica
                i++;                        //trattare uno dei casi evidenziati sopra, altrimenti si registra subito il comando U,R,Q e si procede senza ind2
                j = 0;                      //(non necessario nei comandi appena citati).
                foundComma = 1;
                while(input[i] >= '0' && input[i] <= '9'){
                    inputInd2[j] = input[i];
                    j++;
                    i++;
                }
                ind2 = atoi(inputInd2); 
            }

            command = input[i];
        }

        //debug
        if(command == 'q') printf("%c", command);
        else if(foundComma == 1) printf("%d,%d %c\n\n", ind1, ind2, command);
        else printf("%d %c\n\n", ind1, command);

        //operazioni
        
        quitted++;                  //debug - diventerà if(command == 'q') quitted = 1 (oppure = true) per far uscire dal programma
        foundComma = 0;             //il prossimo ciclo esegue una nuova riga di istruzione, si potrebbe dover rilevare un nuovo comando di tipo C,D,P
        for(i = 0; i < 10; i++){    //reset degli assistenti per il prossimo comando
            inputInd1[i] = '\0';
            inputInd2[i] = '\0';
        }                           //error-prone: potrebbe essere necessario flushare input in caso di rimasugli di precedenti valori non sovrascritti
    }

    return 0;
}

