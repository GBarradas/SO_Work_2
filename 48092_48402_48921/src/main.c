#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "queue.c"

#define MEM_SIZE 200
#define IO_TIME 4
#define MAX_TREADS 4
#define QUANTUM_TIME 3
// Intruções
#define ZERO 0
#define COPY 1
#define DECR 2
#define NWTH 3
#define JFRW 4
#define JBCK 5
#define DISK 6
#define JIFZ 7
#define PRNT 8
#define JOIN 9
#define ADDX 10
#define MULX 11
#define RETN 12
#define HALT 20
#define LOAD 19
#define THRD 18
#define ENDP 17
// Erros
#define SegmentationFault  0
#define InvalidVariable    1
#define InputOutputCall    2
#define PrintVariable      3
#define WaitByThread       4
#define OK                 5
#define InvalidInstruction 6

typedef struct runner Runner;
typedef struct Process Process;
typedef struct Program Program;
// Diversos estados
enum States {
    EXIT,
    PRE_EXIT,
    READY,
    RUN,
    BLOCKED,
    NONCREATE,
    NEW,
    FINISH,
    WBT, //waiting by thread
};

struct runner{
    int output;             //valor a imprimir no caso do processo pedir para imprimir
    int variableId;         // variavel em analise
    int id_running;         // id do processo que se encpntra no estado RUN caso não esteja nenhum toma o valor de (-1)
    int quantum;            //quantum da instrunção atual
    int inputOutput;        //instantes restante para o processo no inicio da fila block passar para ready
    int instant;            // instante atual
    //auxilares para ajudar a imprimir a tabela dos resultados
    int newl;               //tamanho da coluna new
    int readyl;             //tamanho da coluna ready
    int blockl;             //tamanho da coluna block
    int exitl;              //tamanho da coluna exit
};

struct Process{
    Boolean isThread;                   //indica se é uma thread
    Boolean isWaiting[MAX_TREADS];      //indica se um processo esta ha espera de uma thread
    int size;                           //tamanho de um processo
    int pai;                            //id do processo pai
    int idThread;                       //id da thread (diferente do id geral dos processos)
    char *tag;                          // "P" ou "TH"
    int id;                             
    int pc;                             //programCounter
    int startIntant;                    //inicio do blodo de codigo na memoria
    int pcVars;                         //incio do bloco das variaveis na memoria
    int numVars;                        // numero de variaveis de um processo
    enum States state;                  // estado de um processo
    int numOfthreads;                   // numero de threads que um processo tem
    int threads[MAX_TREADS];            //guarda o id das threads do processo
};

struct Program{
    int id;                             //id do programa
    int tStart;                         //instante em que o pragama se inicia                        
    int instruction[200];               //intruçoes ja em codigo e suas variaveis
    int numInstructions;                // numero de instruçoes do programa
    int numVars;                        // numero de variaveis do programa
    int total;                          // espaço total necessario para o programa
    int tNumInstructions;               // numero de instruçoes da thread
    int tIndex;                         // index de onde começa o bloco de codigo da thread
    int tNumVars;                       // numero de variaveis da thread
    int tTotal;                         //espaço total necessario para a thread
    
};

Program programs[MEM_SIZE/2];
Process process[MEM_SIZE/2];
Runner R;
    
Queue ready;
Queue block;
int numOfProcess;
int numOfPrograms;
// memoria
int mem[MEM_SIZE];
int bit[MEM_SIZE];
int freeSpace;
int lastIndex;


int getMax(int maxVal, int instruction, int var){ //devolve o valor maximo entre o valor maximo atual e da instrução em analise
    switch(instruction){
        case ZERO:
        case COPY:
        case DECR:
        case PRNT:
        case ADDX:
        case MULX:
            if(var > maxVal)
                return var;
        default:
            return maxVal;
    }
}

void printMemory(void){             //imprime o array da memoria e o array de bits e o espaço livre  
    for(int i =0 ; i < MEM_SIZE; i=i+1)
        printf("%2d ",mem[i]);
    for(int i =0 ; i < MEM_SIZE; ++i)
        printf("%2d ",bit[i]);
    printf("\nEspaço Livre: %3d\n",freeSpace);
    printf("\n\n");//
}

void removeProcess(int id){     //remove um processo da memoria
    process[id].state = FINISH;
    Process p = process[id];
    for(int t = 0; t < p.numOfthreads; t++ ){     //remover todas as threads do process
        if(process[p.threads[t]].state != FINISH){
            removeProcess(p.threads[t]);
        }
    }
    for(int i = p.startIntant; i < p.startIntant + p.size ; ++i){
        mem[i] = 0;
        bit[i] = -1;
    }
    freeSpace += p.size;
    //printMemory();
}

void allocateThread(int idProgram, int idThread){  //alocar uma Thread
    Program p = programs[idProgram];
    int segmento=-1;
    int pcIntructions;
    int pcVars;
    int indexFinal;
    boolean allocate = false;
    if(p.tTotal > freeSpace){   //caso não haja espaço disponivel 
        process[numOfProcess].state = FINISH;
        printf("> Erro ao alocar o TH%d do P%d espaço de memoria insuficiente\n",idThread+1,idProgram+1);
        return;
    }
    for(int i = lastIndex ; i < MEM_SIZE; i++){     //procura um segmento partindo do ultimo index utilizado
        if(bit[i] == -1){
            segmento ++;
            if(segmento == 0){
                pcIntructions = i;
                segmento++;
            }
            if(segmento == p.tTotal){       //se encontrar o segmento aloca a thread
                lastIndex = pcIntructions + p.tTotal;
                pcVars = pcIntructions + p.tNumInstructions * 2;
                indexFinal = pcIntructions + p.tTotal;

                process[numOfProcess].startIntant = pcIntructions;
                process[numOfProcess].pc = pcIntructions;
                process[numOfProcess].pcVars = pcVars;
                process[numOfProcess].numVars = p.tNumVars;
                process[numOfProcess].state = READY;
                process[numOfProcess].tag = "TH";
                process[numOfProcess].idThread = idThread;
                process[numOfProcess].pai = idProgram;
                process[numOfProcess].id = numOfProcess;
                process[numOfProcess].isThread = true;
                process[numOfProcess].size = p.tTotal;
                allocate = true;
                freeSpace -= p.tTotal;
                for (int j = pcIntructions, k=p.tIndex; j<indexFinal; j++,k++){
                    bit[j] = numOfProcess;
                    mem[j] = p.instruction[k];
                }
                for(int j =pcVars, k = process[idProgram].pcVars; j < indexFinal ; ++j ){
                    bit[j] = numOfProcess;
                    mem[j] = mem[k];
                    k++;
                }
            }

        }
        else{
            segmento = -1;
            pcIntructions = -1;
        }
    }
    if(!allocate){  //caso não aloque a thread começa a procurar do incio da memoria
        for(int i = 0; i < MEM_SIZE; i++){
        if(bit[i] == -1){
            segmento ++;
            if(segmento == 0){
                pcIntructions = i;
                segmento++;
            }
            if(segmento == p.tTotal){
                lastIndex = pcIntructions + p.tTotal;
                pcVars = pcIntructions + p.tNumInstructions * 2;
                indexFinal = pcIntructions + p.tTotal;

                process[numOfProcess].startIntant = pcIntructions;
                process[numOfProcess].pc = pcIntructions;
                process[numOfProcess].pcVars = pcVars;
                process[numOfProcess].numVars = p.tNumVars;
                process[numOfProcess].state = READY;
                process[numOfProcess].tag = "TH";
                process[numOfProcess].idThread = idThread;
                process[numOfProcess].pai = idProgram;
                process[numOfProcess].id = numOfProcess;
                process[numOfProcess].isThread = true;
                process[numOfProcess].size = p.tTotal;
                allocate = true;
                freeSpace -= p.tTotal;
                for (int j = pcIntructions, k=0; j<indexFinal; j++,k++){
                    bit[j] = numOfProcess;
                    mem[j] = p.instruction[k];
                    //printf("%2d %2d\n",p.instruction[k],k);
                }
                for(int j =pcVars, k = process[idProgram].pcVars; j < indexFinal ; ++j ){
                    bit[j] = numOfProcess;
                    mem[j] = mem[k];
                    k++;
                }
            }

            }
            else{
                segmento = -1;
                pcIntructions = -1;
            }
        }
    }
    if(allocate){
        mem[process[numOfProcess].pcVars] = mem[process[idProgram].pcVars+idThread]; 
        mem[process[numOfProcess].pcVars+1] = idThread;
        enqueue(numOfProcess,ready);
        numOfProcess++;
    }
    if(!allocate){
         process[numOfProcess].state = FINISH;
        printf("> Erro ao alocar o TH%d do P%d espaço de memoria insuficiente\n",idThread+1,idProgram+1);
        return;
    }

}

void allocate(int id){      //aloca um processo
    Program p = programs[id];
    int k;
    int segmento = -1;
    int pcIntructions = -1;
    int pcVars, indexFinal;
    int p_size = p.total;
    boolean allocate;
    if (p_size > freeSpace){  //caso não haja espaço suficiente disponivel
        process[id].state = FINISH;
        printf("> Erro ao alocar o P%d espaço de memoria insuficiente\n",id+1);
        return;
    }

    for(int i=lastIndex; i < MEM_SIZE;i++){ //procura um segmento a partir do ultimo endereço utilizado
        if(bit[i] == -1){
            segmento ++;
            if(segmento == 0){
                pcIntructions = i;
                segmento++;
            }
            if(segmento == p_size){
                lastIndex = pcIntructions + p_size;
                pcVars = pcIntructions + p.numInstructions*2;
                indexFinal = pcIntructions + p.total;
                process[id].startIntant = pcIntructions;
                process[id].pcVars = pcVars;
                process[id].numVars = p.numVars;
                process[id].state = NEW;
                process[id].tag = "P";
                process[id].id = id;
                process[id].pc = pcIntructions;
                process[id].isThread = false;
                process[id].size = p_size;
                freeSpace -= p_size;
                //printf("%d %d %d %d \n",pcIntructions,pcVars,indexFinal,p_size);
                for (int j = pcIntructions, k=0; j<indexFinal; j++,k++){
                    bit[j] = id;
                    mem[j] = p.instruction[k];
                    //printf("%2d %2d\n",p.instruction[k],k);
                }
                for(int j =pcVars; j < indexFinal ; ++j ){
                    bit[j] = id;
                    mem[j] = 0;
                }
                allocate = true;

                if(pcIntructions >= MEM_SIZE)
                    pcIntructions = 0;
            }
        }
        else{
            segmento = -1;
            pcIntructions = -1;
        }
    }
    if(!allocate){  //caso não aloque procura um segmento a partir do incio da memoria
        segmento = -1;
        pcIntructions = -1;

        for(int i = 0; i <MEM_SIZE; ++i){
            if(bit[i] == -1){
                segmento++;
                if(segmento == 0){
                    pcIntructions = i;
                    segmento ++;
                }
                if(segmento == p_size){
                    lastIndex = pcIntructions + p_size;
                    pcVars = pcIntructions + p.numInstructions*2;
                    indexFinal = pcIntructions + p.total;
                    process[id].startIntant = pcIntructions;
                    process[id].pcVars = pcVars;
                    process[id].numVars = p.numVars;
                    process[id].state = NEW;
                    process[id].tag = "P";
                    process[id].id = id;
                    process[id].pc = pcIntructions;
                    process[id].isThread = false;
                    process[id].size = p_size;
                    freeSpace -= p_size;

                    for (int j = pcIntructions, k = 0; j < pcVars; j++,k++){
                        bit[j] = id;
                        mem[j] = p.instruction[k];
                    }
                    
                    for (int j = pcVars; j < indexFinal;j++){
                        bit[j] = id;
                        mem[j] = 0;
                    }
                    
                    allocate = true;
                    if(pcIntructions >= MEM_SIZE){
                        pcIntructions = 0;
                    }
                    return;
                }
            }
            else{
                segmento = -1;
                pcIntructions = -1;
            }
        }
    }
    if(!allocate){
        process[id].state = FINISH;
        printf("> Erro ao alocar o P%d espaço de memoria insuficiente\n",id+1);
        return;
    }

}

int getInstructionID(char *instruction){    //determina o codigo de um intrução 
    if (instruction == NULL) return -1;
    if (strcmp(instruction, "ZERO") == 0)
        return ZERO;
    else if (strcmp(instruction, "COPY") == 0)
        return COPY;
    else if (strcmp(instruction, "DECR") == 0)
        return DECR;
    else if (strcmp(instruction, "NWTH") == 0)
        return NWTH;
    else if (strcmp(instruction, "JFRW") == 0)
        return JFRW;
    else if (strcmp(instruction, "JBCK") == 0)
        return JBCK;
    else if (strcmp(instruction, "DISK") == 0)
        return DISK;
    else if (strcmp(instruction, "JIFZ") == 0)
        return JIFZ;
    else if (strcmp(instruction, "PRNT") == 0)
        return PRNT;
    else if (strcmp(instruction, "JOIN") == 0)
        return JOIN;
    else if (strcmp(instruction, "ADDX") == 0)
        return ADDX;
    else if (strcmp(instruction, "MULX") == 0)
        return MULX;
    else if (strcmp(instruction, "RETN") == 0)
        return RETN;
    else if (strcmp(instruction, "HALT") == 0)
        return HALT;
    else if (strcmp(instruction, "LOAD") == 0)
        return LOAD;
    else if (strcmp(instruction, "THRD") == 0)
        return THRD;
    else if (strcmp(instruction, "ENDP") == 0)
        return ENDP;
    else
        return -1;
}

void readFile( FILE  *file){    //le o ficheiro e guarda as informações que le no array programs
    if (file == NULL) return ;
    int maxVal=0 , i = 0, j = -1 , nOfInstru=0;
    char *line;
    size_t size = 0;
    while(getline(&line,&size, file) != -1){
        if(strcmp(line, "\n")!= 0){
            char *instruction = strtok(line, " ");
            if(strcmp(instruction, "LOAD") == 0){ //incio de um programa e do seu bloco de codigo
                programs[i].tStart = atoi(strtok(NULL, " "));
            }
            else if(strcmp(instruction, "THRD") == 0){  //inicio de umm bloco de  codigo de Threads
                nOfInstru++;
                programs[i].numInstructions = nOfInstru;
                programs[i].numVars = maxVal+1;
                programs[i].total = programs[i].numVars + (programs[i].numInstructions) * 2;
                programs[i].instruction[(programs[i].numInstructions*2) -2] = THRD;
                maxVal = 0;
                nOfInstru = 0;
                programs[i].tIndex = j+1;
                programs[i].id = i;
                process[i].state = NONCREATE;
            }
            else if(strcmp(instruction, "ENDP") == 0){  //fim do programa
                programs[i].tNumInstructions = nOfInstru;
                if (maxVal < 10)
                    programs[i].tNumVars = maxVal;
                else   
                    programs[i].tNumVars = 9;
                programs[i].tTotal = programs[i].tNumVars + (programs[i].tNumInstructions ) * 2;
                maxVal = 0;
                nOfInstru = 0;
                j = -1;
                i++;
            }
            else{
                nOfInstru ++;
                int intruction = getInstructionID(instruction);
                int var = atoi(strtok(NULL, " "));
                maxVal = getMax(maxVal, intruction,var);
                ++j;
                programs[i].instruction[j] = intruction;
                ++j;
                programs[i].instruction[j]= var;
            }
        }

    }
    programs[i].numInstructions = nOfInstru;
    programs[i].numVars = maxVal+1;
}

int getNumOfPrograms(FILE *file){ //le o ficheiro e determina o numero de programas
    char *line;
    size_t size;
    int nOProg = 0;
    if(file == NULL){
        return -1;
    }
    while(getline(&line, &size, file)!= -1){
        char *instruction = strtok(line, " ");
        if (strcmp(instruction, "LOAD")==0)
            nOProg++;
    }
    rewind(file);
    return nOProg;
}

int executeThread(int id){  //executa a instrução de um thread
    //printf("TH%d",id);
    Process p = process[id];
    Process pai = process[p.pai];
    int index = p.pc;
    int instruction = mem[index];
    int value = mem[index+1];
    int dest;
            //printf("@%d %d, index : %d, %d",instruction,value,index,p.pcVars);
    if(index >= p.pcVars || index < p.startIntant){
        removeProcess(id);
        return SegmentationFault;
    }
    switch(instruction){
        case ZERO:
                mem[p.pcVars] = value;
                process[id].pc += 2;
            break;
        case COPY:
            if( value > 0 ||value > p.numVars){
                if(value > 9){                  //variavel global
                    mem[pai.pcVars + value] = mem[p.pcVars];
                    process[id].pc +=2;
                }
                else{                           //variavel local
                    mem[p.pcVars + value] = mem[p.pcVars];
                    process[id].pc +=2;
                }

            }
            else{
                removeProcess(id);
                return InvalidVariable;
            }
            break;
        case DECR:
            if( value > 0 || value > p.numVars){
                if(value > 9){      // variavel global
                    mem[pai.pcVars + value]--;
                    process[id].pc +=2;
                }
                else{              // variavel local
                    mem[p.pcVars + value]--;
                    process[id].pc +=2;
                }

            }
            else{
                removeProcess(id);
                return InvalidVariable;
            }
            break;
        case JFRW:
            dest = index + value*2;
            if(dest >= p.pcVars || dest < p.startIntant){
                removeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].pc = dest;
            }
            break;
        case JBCK:
        dest = index - value*2;
            if(dest >= p.pcVars || dest < p.startIntant){
                removeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].pc = dest;
            }
            break;
        case DISK:
            process[id].pc +=2;
            return InputOutputCall;
            break;
        case JIFZ:
            dest = index + 6;
            int memindex = value > 9 ? pai.pcVars+value : p.pcVars+value;
            if(mem[memindex] == 0){
                if(dest >= p.pcVars || dest < p.startIntant){
                    removeProcess(id);
                    return SegmentationFault;
                }
                else{
                    process[id].pc = dest;
                }
            }
            else{
                process[id].pc += 2;
            }
            break;
        case PRNT:
            if(value >= 0){
                process[id].pc += 2;
                return PrintVariable;
            }
            else{
                removeProcess(id);
                return InvalidVariable;
            }
            break;
        case ADDX:
            if(value < 0){
                return SegmentationFault;
            }
            else{
                process[id].pc += 2;
                if(value <= 9){  // variavel local
                    mem[p.pcVars] = mem[p.pcVars] + mem[p.pcVars + value];
                }
                else{           // variavel global
                    mem[p.pcVars] = mem[p.pcVars] + mem[pai.pcVars + value];
                }
            }
            break;
        case MULX:
            if(value < 0){
                    return SegmentationFault;
                }
                else{
                    process[id].pc += 2;
                    if(value <= 9){  // variavel local
                        mem[p.pcVars] = mem[p.pcVars] * mem[p.pcVars + value];
                    }
                    else{           // variavel global
                        mem[p.pcVars] = mem[p.pcVars] * mem[pai.pcVars + value];
                    }
                }
            break;
        case RETN:
            process[id].state = FINISH;
            removeProcess(id);
            break;
        default:
            removeProcess(id);
            return InvalidInstruction;
            break;

    }
    return OK;
}

int executeProgram(int id){ //executa a intrução de um programa
    Process p = process[id];
    if (p.isThread || strcmp(p.tag,"TH") == 0){
        int result = executeThread(id);
        return result;
    }
    int index = p.pc;
    int instruction = mem[index];
    int value = mem[index +1];
    int dest;
    if(index >= p.pcVars || index < p.startIntant){
        removeProcess(id);
        return SegmentationFault;
    }
    switch(instruction){
        case ZERO:
            mem[p.pcVars] = value;
            process[id].pc += 2;
            break;
        case COPY:
            if(value >0 || value > p.pcVars){
                mem[p.pcVars + value] = mem[p.pcVars];
                process[id].pc +=2;
            }
            else{
                removeProcess(id);
                return InvalidVariable;
            }
            break;
        case DECR:
            if(value >0 || value > p.numVars){
                mem[p.pcVars + value]--;
                process[id].pc +=2;
            }
            else{
                removeProcess(id);
                return InvalidVariable;
            }
            break;
        case NWTH:
            if(p.numOfthreads < MAX_TREADS){
                allocateThread(id,p.numOfthreads);
                process[id].threads[p.numOfthreads] = numOfProcess;
                process[id].numOfthreads++;
                process[id].pc +=2;
            }
            break;
        case JFRW:
            dest = index + value *2;
            if(dest >= p.pcVars|| dest < p.startIntant){
                removeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].pc = dest;
            }
            break;
        case JBCK:
            dest = index - value *2;
            if(dest >= p.pcVars|| dest < p.startIntant){
                removeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].pc = dest;
            }
            break;
        case DISK:
            process[id].pc +=2;
            return InputOutputCall;
            break;
        case JIFZ:
            dest = index + 4;
            if(mem[p.pcVars+value] == 0){
                if(dest >= p.pcVars || dest < p.startIntant){
                    removeProcess(id);
                    return SegmentationFault;
                }
                else{
                    process[id].pc = dest;
                }
            }
            else{
                process[id].pc += 2;
            }
            break;
        case PRNT:
            if(value >= 0){
                process[id].pc += 2;
                return PrintVariable;
            }
            else{
                removeProcess(id);
                return InvalidVariable;
            }
            break;
        case JOIN:
            process[id].pc += 2;
            return WaitByThread;
            break;
        case ADDX:
            if(value < 0)
                return SegmentationFault;
            else{
                mem[p.pcVars] = mem[p.pcVars] + mem[p.pcVars+ value];
                process[id].pc += 2;
            }
            break;
        case MULX:
            if(value < 0)
                return SegmentationFault;
            else{
                mem[p.pcVars] = mem[p.pcVars] * mem[p.pcVars+ value];
                process[id].pc +=  2;
            }
            break;

        case HALT:
            process[id].state = PRE_EXIT;
            break;
        default:
            return InvalidInstruction;
            break;
    }
    return OK;

}

Boolean canProced(int id){  // verifica se um processo que esta é espera de uma thread pode continuar
    Process p = process[id];
    for(int c = 0; c < MAX_TREADS; c++){
        if(p.isWaiting[c]){
            if(process[p.threads[c]].state != FINISH)
                return false;
        }
    }
    return true;
}

void blocked2Ready(){  // verifica se um processo pode passar de BLOCK para READY
    if(!isEmpty(block)){
        if(R.inputOutput == 0){
            process[peek(block)].state = READY;
            enqueue(dequeue(block), ready);
            R.inputOutput = IO_TIME;
        }
        R.inputOutput--;
    }
}

void newProcess(){      // verifica se podemo incializar um novo processo
   for( int i = 0; i < numOfPrograms; i++){
        Process p = process[i];
        if(programs[i].tStart == R.instant){
            allocate(i);
        }
    }
}

void new2Ready(){   // passamos todos os processos em NEW para READY
    for( int i = 0; i < numOfPrograms; i++){
        Process p = process[i];
        if(p.state == NEW && programs[i].tStart != R.instant ){
            enqueue(i,ready);
            process[i].state = READY;
        }
    }
}

void run2exit_blocked_run(){    //executa o processo e verifica o quantum
    if(R.id_running !=-1){
                R.quantum--;
                if(R.quantum == 0 && isEmpty(ready)){
                    R.quantum++;
                }
                if(R.quantum == 0){
                    enqueue(R.id_running,ready);
                    process[R.id_running].state = READY;
                    R.id_running = -1;
                }
                else{
                    R.variableId = executeProgram(R.id_running);
                    if( R.variableId == PrintVariable){
                        int var = mem[process[R.id_running].pc-1];
                        if(process[R.id_running].isThread){
                            if(var >9){         //variavel global no caso de um thread
                                int pos = process[process[R.id_running].pai].pcVars + var;
                                R.output = mem[pos];
                            }
                            else{              //variavel local no caso de um thread
                                R.output = mem[process[R.id_running].pcVars+var];
                            }
                        }
                        else{
                            R.output = mem[process[R.id_running].pcVars+var];
                        }
                    }
                }
            }
            
}

void ready2run(){   //caso não esteja nenhum process podemos por la um process
     if(R.id_running == -1 && !isEmpty(ready)){
                R.id_running = dequeue(ready);
                R.quantum = QUANTUM_TIME;
                process[R.id_running].state = RUN;
                R.variableId = executeProgram(R.id_running);
                if( R.variableId == PrintVariable){
                    int var = mem[process[R.id_running].pc-1];
                    if(process[R.id_running].isThread){
                        if(var >9){         //variavel global no caso de um thread
                            int pos = process[process[R.id_running].pai].pcVars + var;
                            R.output = mem[pos];
                        }
                        else{             //variavel local no caso de um thread
                            R.output = mem[process[R.id_running].pcVars+var];
                        }
                    }
                    else{
                        R.output = mem[process[R.id_running].pcVars+var];
                    }
                }
            }
            
}

void exit2finish(){ // passa todos os processos de EXIT para FINISH
    for(int i = 0; i< numOfPrograms; i++){
                if(process[i].state == EXIT){
                    removeProcess(i);
                    process[i].state == FINISH;
                }
                if(process[i].state == PRE_EXIT){
                    process[i].state = EXIT;
                    R.id_running = -1;
                }
            }
}

void runner(){  //responsavel pela execução de todos os programas

    R.variableId = -1;
    R.id_running=-1;
    R.quantum = QUANTUM_TIME;
    R.inputOutput = IO_TIME;
    R.instant=0;
    R.newl = 6;
    R.readyl = 13;
    R.blockl = 15;
    R.exitl = 8;
        printf("|   T   |NEW   |READY        | RUN |BLOCK          |EXIT    |\n");
        int l = 0;
        newProcess();
        printf("| %3d   |",R.instant);
        for(int i = 0; i < numOfPrograms;++i){
            if(process[i].state == NEW){
                printf("%s%d ",process[i].tag,i+1);
                    if(process[i].id <= 9)
                        l += 3;
                    else
                        l += 4;
            }
           
        }
        for(l;l<R.newl;l++){
                printf(" ");
        }
        printf("|             |     |               |        |\n");

        while(true){
            
            R.variableId = 0;
            R.instant ++;

            blocked2Ready();
            exit2finish();
            run2exit_blocked_run();
            new2Ready();
            ready2run();
            newProcess();

            l = 0;
            printf("| %3d   |",R.instant);
            for( int i = 0; i < numOfPrograms; i++){
                Process p = process[i];
                if(process[i].state == NEW){
                    printf("%s%d ",process[i].tag,i+1);
                    if(p.id <= 9)
                        l += 3;
                    else
                        l += 4;
                }
            }
            for(l;l<R.newl;l++){
                printf(" ");
            }
            printf("|");
            l= 0;
            Node aux = ready->front;
            while(aux != NULL){         //imprimir a fila ready
                Process p = process[aux->element];
                if(p.isThread)
                    printf("%s%d ",p.tag,p.idThread+1);
                else
                    printf("%s%d ",p.tag,p.id+1);
                if(strcmp(p.tag,"TH") == 0){
                    if(p.id <= 9)
                        l += 4;
                    else
                        l += 5;
                }
                else{
                    if(p.id <= 9)
                        l += 3;
                    else
                        l += 4;
                    
                }
                aux = aux->next;
            }
            for(l;l<R.readyl;l++){  
            printf(" ");
            }
            printf("|");    
            if(R.id_running == -1){     //caso nenhum programa esta em RUN
                printf("     |");
            }
            else{
                Process p = process[R.id_running];
                if(strcmp(p.tag,"TH") == 0){
                    if(p.id <= 9)
                        printf(" %s%d |",p.tag,p.idThread+1);
                    else
                        printf("%s%d |",p.tag,p.idThread+1);
                }
                else{
                    if(p.id <= 9)
                        printf(" %s%d  |",p.tag,p.id+1);
                    else
                        printf(" %s%d |",p.tag,p.id+1);
                }
            }
            l = 0;
            aux = block->front;
            while(aux != NULL){     //imprimir a fila block
                Process p = process[aux->element];
                if(p.isThread)
                    printf("%s%d ",p.tag,p.idThread+1);
                else
                    printf("%s%d ",p.tag,p.id+1);
                if(strcmp(p.tag,"TH") == 0){
                    if(p.id <= 9)
                        l += 4;
                    else
                        l += 5;
                }
                else{
                    if(p.id <= 9)
                        l += 3;
                    else
                        l += 4;
                    
                }
                aux = aux->next;
            }
            int wb = 0;
            while(wb < numOfPrograms){  // imprimir junto com a fila block os processos que se encontre á espera de uma thread
                Process p = process[wb];   
                if(p.state == WBT){
                    printf("%s%d ",p.tag,p.id+1);
                    if(p.id <= 9)
                        l += 3;
                    else
                        l += 4;
                }
                wb++;    
                
            }
            for(l; l<R.blockl;++l){
                printf(" ");
            }
            printf("|");
            l = 0;
            for( int i = 0; i < numOfPrograms;i++){     //imprime programa no estado exit
                if(process[i].state==EXIT ){
                    printf("P%d ",i+1);
                    if(i < 9)
                        l += 3;
                    else
                        l += 4;
                }
            }
            for(l ;l < R.exitl; l++){
                printf(" ");
            }

            printf("|\n");
            if(process[R.id_running].state == PRE_EXIT){    // caso o programa deixe de correr por que vai terminar
                R.id_running = -1;
            }
            if(R.variableId == PrintVariable){          // imprimir a variavel no caso de instrução executada ser PRNT
                printf(">Print %d \n",R.output);
            }
            if(R.variableId == InputOutputCall){        //caso a intrução executada seja DISK
                process[R.id_running].state = BLOCKED;  //o processo passa para o estado BLOCKED 
                enqueue(R.id_running,block);
                R.id_running = -1;
            }
            if (R.variableId == WaitByThread){          // caso a instrução executada seja JOIN
                int thread = mem[process[R.id_running].pc-1];       // o processo espera pelo final da thread em questão
                process[R.id_running].isWaiting[thread-1] = true;
                process[R.id_running].state = WBT;
                R.id_running = -1;
                
            }
            if( R.variableId == SegmentationFault){         // caso ocorra uma falha de segmentação é imprimida uma mensagem
                if(R.id_running != -1){
                    if(process[R.id_running].isThread){
                        Process p = process[R.id_running];
                        printf("> Falha de Segmentação da TH%d do P%d\n",p.idThread, p.pai+1);
                        process[p.pai].isWaiting[p.idThread] = false;
                    }
                    else{
                        printf("> Falha de Segmentação no P%d\n",R.id_running+1);
                    }
                    R.id_running = -1;
                }

            }
            if( R.variableId == InvalidVariable){   // caso a instrução tente aceder uma variavel que não existe
               if(R.id_running != -1){
                   if(process[R.id_running].isThread){
                       Process p = process[R.id_running];
                       printf("> Variavel invalida da TH%d do P%d\n",p.idThread,p.pai+1);
                   }
                   else{
                        printf("> Variavel invalida no P%d\n",R.id_running);
                   }
                R.id_running = -1;
                }
            }
            if(process[R.id_running].isThread){
                Process p = process[R.id_running];     //caso a thread termine de correr 
                if(p.state == FINISH){
                    process[p.pai].isWaiting[p.idThread] = false;   //o processo pai ja não espera mais por esta thread
                    R.id_running = -1;
                }
            }
            for(int c = 0; c < numOfPrograms; c++){     //verificar se processos em á espera de thread podem prosseguir
                if(process[c].state == WBT && canProced(c)){
                    process[c].state = READY;
                    enqueue(c,ready);
                }
            }
            
            int nOfProgramsRunning = 0;
            for(int i = 0; i < numOfPrograms; i++){ 
                if(process[i].state == FINISH){
                    nOfProgramsRunning++;           //determinar o numeros de processos executando
                }
            }
            if(nOfProgramsRunning == numOfPrograms) break;
            //if(R.instant == 100) break;
        }

}

void main(){
    char *path = "inputs/input.txt";       //ficheiro de input
    FILE *file = fopen(path, "r");
    if(file ==NULL){
        printf("Não foi possivel abrir o fichero!\n");
        return;
    }
    else{
        printf("Ficheiro aberto com sucesso!\n");
    }
    numOfPrograms = getNumOfPrograms(file); //numero de programas
    numOfProcess = numOfPrograms;           // numero de processos é igual ao numero de programas pois ainda não ha threads
    readFile(file);                         // le o ficheiro
    freeSpace = MEM_SIZE;
    lastIndex = -1;
    for(int i =0 ; i < 200; ++i){
        mem[i] = 0;
        bit[i] = -1;
    }
    ready = inicializeQueue();
    block = inicializeQueue();
    runner();
    
    
}
