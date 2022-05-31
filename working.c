#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "queue.c"

#define MEM_SIZE 200
#define IO_TIME 5
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

typedef struct Process Process;
typedef struct Program Program;

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

struct Process{
    Boolean isThread;
    Boolean isWaiting[MAX_TREADS];
    int size;
    int pai;
    int idThread;
    char *tag;
    int id;
    int index;
    int intr_start;
    int pc_vars;
    int n_vars;
    enum States state;
    int n_threads;
    int threads[MAX_TREADS];
};

struct Program{
    int id;
    int initial;
    int index;
    int ins_start;
    int var_start;
    int instruction[200];
    int n_instructions;
    int n_vars;
    int total;
    int thread_n_instructions;
    int thread_index;
    int thread_n_var;
    int thread_total;
    
};

Program programs[MEM_SIZE/2];
Process process[MEM_SIZE/2];
    
Queue ready;
Queue block;
int numOfProcess;
int numOfPrograms;

int mem[MEM_SIZE];
int bit[MEM_SIZE];
int freeSpace;
int lastIndex;

int io;

int getMax(int maxVal, int instruction, int var){
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

void printMemory(){
    for(int i =0 ; i < MEM_SIZE; i=i+1)
        printf("%2d ",mem[i]);
    printf("\n|%3d|\n",freeSpace);
    for(int i =0 ; i < MEM_SIZE; ++i)
        printf("%2d ",bit[i]);
    printf("\n\n");//
}

void freeProcess(int id){
    process[id].state = FINISH;
    Process p = process[id];
    for(int i = p.intr_start; i < p.intr_start + p.size ; ++i){
        mem[i] = 0;
        bit[i] = -1;
    }
    freeSpace += p.size;
    //printMemory();
}

void allocateThread(int idProgram, int idThread){
    Program p = programs[idProgram];
    int segmento=-1;
    int index_inicio;
    int index_vars;
    int index_final;
    boolean allocate = false;
    if(p.thread_total > freeSpace){
        printf("> Erro ao alocar o TH%d do P%d espaço de memoria insuficiente\n",idThread+1,idProgram+1);
        return;
    }
    for(int i = lastIndex ; i < MEM_SIZE; i++){
        if(bit[i] == -1){
            segmento ++;
            if(segmento == 0){
                index_inicio = i;
                segmento++;
            }
            if(segmento == p.thread_total){
                //printf("@%d, %d, --%d\n",lastIndex,index_inicio,p.thread_total);
                lastIndex = index_inicio + p.thread_total;
                index_vars = index_inicio + p.thread_n_instructions * 2;
                index_final = index_inicio + p.thread_total;

                process[numOfProcess].intr_start = index_inicio;
                process[numOfProcess].index = index_inicio;
                process[numOfProcess].pc_vars = index_vars;
                process[numOfProcess].n_vars = p.thread_n_var;
                process[numOfProcess].state = READY;
                process[numOfProcess].tag = "TH";
                process[numOfProcess].idThread = idThread;
                process[numOfProcess].pai = idProgram;
                process[numOfProcess].id = numOfProcess;
                process[numOfProcess].isThread = true;
                process[numOfProcess].size = p.thread_total;
                allocate = true;
                freeSpace -= p.thread_total;
                for (int j = index_inicio, k=p.thread_index; j<index_final; j++,k++){
                    bit[j] = numOfProcess;
                    mem[j] = p.instruction[k];
                    //printf("%2d %2d\n",p.instruction[k],k);
                }
                for(int j =index_vars, k = process[idProgram].pc_vars; j < index_final ; ++j ){
                    bit[j] = numOfProcess;
                    mem[j] = mem[k];
                    k++;
                }
            }

        }
        else{
            segmento = -1;
            index_inicio = -1;
        }
    }
    if(!allocate){
        for(int i = lastIndex ; i < MEM_SIZE; i++){
        if(bit[i] == -1){
            segmento ++;
            if(segmento == 0){
                index_inicio = i;
                segmento++;
            }
            if(segmento == p.thread_total){
                lastIndex = index_inicio + p.thread_total;
                index_vars = index_inicio + p.thread_n_instructions * 2;
                index_final = index_inicio + p.thread_total;

                process[numOfProcess].intr_start = index_inicio;
                process[numOfProcess].index = index_inicio;
                process[numOfProcess].pc_vars = index_vars;
                process[numOfProcess].n_vars = p.thread_n_var;
                process[numOfProcess].state = READY;
                process[numOfProcess].tag = "TH";
                process[numOfProcess].idThread = idThread;
                process[numOfProcess].pai = idProgram;
                process[numOfProcess].id = numOfProcess;
                process[numOfProcess].isThread = true;
                process[numOfProcess].size = p.thread_total;
                allocate = true;
                freeSpace -= p.thread_total;
                for (int j = index_inicio, k=0; j<index_final; j++,k++){
                    bit[j] = numOfProcess;
                    mem[j] = p.instruction[k];
                    //printf("%2d %2d\n",p.instruction[k],k);
                }
                for(int j =index_vars, k = process[idProgram].pc_vars; j < index_final ; ++j ){
                    bit[j] = numOfProcess;
                    mem[j] = mem[k];
                    k++;
                }
            }

            }
            else{
                segmento = -1;
                index_inicio = -1;
            }
        }
    }
    if(allocate){
        mem[process[numOfProcess].pc_vars] = mem[process[idProgram].pc_vars+idThread]; 
        mem[process[numOfProcess].pc_vars+1] = idThread;
        enqueue(numOfProcess,ready);
        numOfProcess++;
    }

}

void allocate(int id){
    Program p = programs[id];
    int k;
    int segmento = -1;
    int index_inicio = -1;
    int index_vars, index_final;
    int p_size = p.total;
    boolean allocate;
    //printf("%d",p_size);
    if (p_size > freeSpace){
        process[id].state = FINISH;
        printf("> Erro ao alocar o P%d espaço de memoria insuficiente\n",id+1);
        return;
    }

    for(int i=lastIndex; i < MEM_SIZE;i++){
        if(bit[i] == -1){
            segmento ++;
            if(segmento == 0){
                index_inicio = i;
                segmento++;
            }
            if(segmento == p_size){
                lastIndex = index_inicio + p_size;
                index_vars = index_inicio + p.n_instructions*2;
                index_final = index_inicio + p.total;
                process[id].intr_start = index_inicio;
                process[id].pc_vars = index_vars;
                process[id].n_vars = p.n_vars;
                process[id].state = NEW;
                process[id].tag = "P";
                process[id].id = id;
                process[id].index = index_inicio;
                process[id].isThread = false;
                process[id].size = p_size;
                freeSpace -= p_size;
                //printf("%d %d %d %d \n",index_inicio,index_vars,index_final,p_size);
                for (int j = index_inicio, k=0; j<index_final; j++,k++){
                    bit[j] = id;
                    mem[j] = p.instruction[k];
                    //printf("%2d %2d\n",p.instruction[k],k);
                }
                for(int j =index_vars; j < index_final ; ++j ){
                    bit[j] = id;
                    mem[j] = 0;
                }
                allocate = true;

                if(index_inicio >= MEM_SIZE)
                    index_inicio = 0;
            }
        }
        else{
            segmento = -1;
            index_inicio = -1;
        }
    }
    if(!allocate){
        segmento = -1;
        index_inicio = -1;

        for(int i = 0; i <MEM_SIZE; ++i){
            if(bit[i] == -1){
                segmento++;
                if(segmento == 0){
                    index_inicio = i;
                    segmento ++;
                }
                if(segmento == p_size){
                    lastIndex = index_inicio + p_size;
                    index_vars = index_inicio + p.n_instructions*2;
                    index_final = index_inicio + p.total;
                    process[id].intr_start = index_inicio;
                    process[id].pc_vars = index_vars;
                    process[id].n_vars = p.n_vars;
                    process[id].state = NEW;
                    process[id].tag = "P";
                    process[id].id = id;
                    process[id].index = index_inicio;
                    process[id].isThread = false;
                    process[id].size = p_size;
                    freeSpace -= p_size;

                    for (int j = index_inicio, k = 0; j < index_vars; j++,k++){
                        bit[j] = id;
                        mem[j] = p.instruction[k];
                    }
                    
                    for (int j = index_vars; j < index_final;j++){
                        bit[j] = id;
                        mem[j] = 0;
                    }
                    
                    allocate = true;
                    if(index_inicio >= MEM_SIZE){
                        index_inicio = 0;
                    }
                    return;
                }
            }
            else{
                segmento = -1;
                index_inicio = -1;
            }
        }
    }

}

int getInstructionID(char *instruction){
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

void readFile( FILE  *file){
    if (file == NULL) return ;
    int maxVal=0 , i = 0, j = -1 , nOfInstru=0;
    char *line;
    size_t size = 0;
    while(getline(&line,&size, file) != -1){
        if(strcmp(line, "\n")!= 0){
            char *instruction = strtok(line, " ");
            if(strcmp(instruction, "LOAD") == 0){
                programs[i].initial = atoi(strtok(NULL, " "));
            }
            else if(strcmp(instruction, "THRD") == 0){
                nOfInstru++;
                programs[i].n_instructions = nOfInstru;
                programs[i].n_vars = maxVal+1;
                printf("%d %d %d\n",i,programs[i].n_vars,programs[i].n_instructions * 2 );
                programs[i].total = programs[i].n_vars + (programs[i].n_instructions) * 2;
                programs[i].instruction[(programs[i].n_instructions*2) -2] = THRD;
                maxVal = 0;
                nOfInstru = 0;
                programs[i].thread_index = j+1;
                programs[i].id = i;
                programs[i].index = 0;
            }
            else if(strcmp(instruction, "ENDP") == 0){
                programs[i].thread_n_instructions = nOfInstru;
                if (maxVal < 10)
                    programs[i].thread_n_var = maxVal;
                else   
                    programs[i].thread_n_var = 9;
                programs[i].thread_total = programs[i].thread_n_var + (programs[i].thread_n_instructions ) * 2;
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
    programs[i].n_instructions = nOfInstru;
    programs[i].n_vars = maxVal+1;
}

int getNumOfPrograms(FILE *file){
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

int executeThread(int id){
    
    Process p = process[id];
    Process pai = process[p.pai];
    if(pai.state == FINISH || pai.state == EXIT){
        freeProcess(id);
    }
    int index = p.index;
    int instruction = mem[index];
    int variableId = mem[index+1];
    int dest;
            //printf("@%d %d, index : %d, %d",instruction,variableId,index,p.pc_vars);
    if(index >= p.pc_vars || index < p.intr_start){
        freeProcess(id);
        return SegmentationFault;
    }
    switch(instruction){
        case ZERO:
                mem[p.pc_vars] = variableId;
                process[id].index += 2;
            break;
        case COPY:
            if( variableId > 0 ||variableId > p.n_vars){
                if(variableId > 9){
                    mem[pai.pc_vars + variableId] = mem[p.pc_vars];
                    process[id].index +=2;
                }
                else{
                    mem[p.pc_vars + variableId] = mem[p.pc_vars];
                    process[id].index +=2;
                }

            }
            else{
                freeProcess(id);
                return InvalidVariable;
            }
            break;
        case DECR:
            if( variableId > 0 || variableId > p.n_vars){
                if(variableId > 9){
                    mem[pai.pc_vars + variableId]--;
                    process[id].index +=2;
                }
                else{
                    mem[p.pc_vars + variableId]--;
                    process[id].index +=2;
                }

            }
            else{
                freeProcess(id);
                return InvalidVariable;
            }
            break;
        case JFRW:
            dest = index + variableId*2;
            if(dest >= p.pc_vars || dest < p.intr_start){
                freeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].index = dest;
            }
            break;
        case JBCK:
        dest = index - variableId*2;
            if(dest >= p.pc_vars || dest < p.intr_start){
                freeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].index = dest;
            }
            break;
        case DISK:
            process[id].index +=2;
            return InputOutputCall;
            break;
        case JIFZ:
            dest = index + 6;
            int memindex = variableId > 9 ? pai.pc_vars+variableId : p.pc_vars+variableId;
            if(mem[memindex] == 0){
                if(dest >= p.pc_vars || dest < p.intr_start){
                    freeProcess(id);
                    return SegmentationFault;
                }
                else{
                    process[id].index = dest;
                }
            }
            else{
                process[id].index += 2;
            }
            break;
        case PRNT:
            if(variableId >= 0){
                process[id].index += 2;
                return PrintVariable;
            }
            else{
                freeProcess(id);
                return InvalidVariable;
            }
            break;
        case ADDX:
            if(variableId < 0){
                return SegmentationFault;
            }
            else{
                process[id].index += 2;
                if(variableId < 9){
                    mem[p.pc_vars] = mem[p.pc_vars] + mem[p.pc_vars + variableId];
                }
                else{
                    mem[p.pc_vars] = mem[p.pc_vars] + mem[pai.pc_vars + variableId];
                }
            }
            break;
        case MULX:
            if(variableId < 0){
                    return SegmentationFault;
                }
                else{
                    process[id].index += 2;
                    if(variableId < 9){
                        mem[p.pc_vars] = mem[p.pc_vars] * mem[p.pc_vars + variableId];
                    }
                    else{
                        mem[p.pc_vars] = mem[p.pc_vars] * mem[pai.pc_vars + variableId];
                    }
                }
            break;
        case RETN:
            process[id].state = FINISH;
            freeProcess(id);
            break;
        default:
            freeProcess(id);
            return InvalidInstruction;
            break;

    }
    return OK;
}

int executeProgram(int id){
    Process p = process[id];
    if (p.isThread || strcmp(p.tag,"TH") == 0){
        int result = executeThread(id);
        return result;
    }
    int index = p.index;
    int instruction = mem[index];
    int variableId = mem[index +1];
    int dest;
    if(index >= p.pc_vars || index < p.intr_start){
        freeProcess(id);
        return SegmentationFault;
    }
    switch(instruction){
        case ZERO:
            mem[p.pc_vars] = variableId;
            process[id].index += 2;
            break;
        case COPY:
            if(variableId >0 || variableId > p.pc_vars){
                mem[p.pc_vars + variableId] = mem[p.pc_vars];
                process[id].index +=2;
            }
            else{
                freeProcess(id);
                return InvalidVariable;
            }
            break;
        case DECR:
            if(variableId >0 || variableId > p.n_vars){
                mem[p.pc_vars + variableId]--;
                process[id].index +=2;
            }
            else{
                freeProcess(id);
                return InvalidVariable;
            }
            break;
        case NWTH:
            if(p.n_threads < MAX_TREADS){
                allocateThread(id,p.n_threads);
                process[id].threads[p.n_threads] = numOfProcess;
                process[id].n_threads++;
                process[id].index +=2;
            }
            break;
        case JFRW:
            dest = index + variableId *2;
            if(dest >= p.pc_vars|| dest < p.intr_start){
                freeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].index = dest;
            }
            break;
        case JBCK:
            dest = index - variableId *2;
            if(dest >= p.pc_vars|| dest < p.intr_start){
                freeProcess(id);
                return SegmentationFault;
            }
            else{
                process[id].index = dest;
            }
            break;
        case DISK:
            process[id].index +=2;
            return InputOutputCall;
            break;
        case JIFZ:
            dest = index + 4;
            if(mem[p.pc_vars+variableId] == 0){
                if(dest >= p.pc_vars || dest < p.intr_start){
                    freeProcess(id);
                    return SegmentationFault;
                }
                else{
                    process[id].index = dest;
                }
            }
            else{
                process[id].index += 2;
            }
            break;
        case PRNT:
            if(variableId >= 0){
                process[id].index += 2;
                return PrintVariable;
            }
            else{
                freeProcess(id);
                return InvalidVariable;
            }
            break;
        case JOIN:
            process[id].index += 2;
            return WaitByThread;
            break;
        case ADDX:
            if(variableId < 0)
                return SegmentationFault;
            else{
                mem[p.pc_vars] = mem[p.pc_vars] + mem[p.pc_vars+ variableId];
                process[id].index += 2;
            }
            break;
        case MULX:
            if(variableId < 0)
                return SegmentationFault;
            else{
                mem[p.pc_vars] = mem[p.pc_vars] * mem[p.pc_vars+ variableId];
                process[id].index +=  2;
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

Boolean canProced(int id){
    Process p = process[id];
    for(int c = 0; c < MAX_TREADS; c++){
        if(p.isWaiting[c])
            return false;
    }
    return true;
}

void runner(){
    int output ,
        variableId = -1,
        id_running=-1,
        quantum = QUANTUM_TIME,
        inputOutput = IO_TIME,
        length,
        instant=0;
        int newl = 6;
        int readyl = 13;
        int blockl = 15;
        int exitl = 8;
        printf("|   T   |NEW   |READY        | RUN |BLOCK          |EXIT    |\n");
        printf("| %3d   |",instant);
        int l = 0;
        for(int i = 0; i < numOfPrograms;++i){
            if(programs[i].initial == 0){
                allocate(i);
                printf("%s%d ",process[i].tag,i+1);
                    if(process[i].id <= 9)
                        l += 3;
                    else
                        l += 4;
           }
        }
        for(l;l<newl;l++){
                printf(" ");
        }
        printf("|             |     |               |        |\n");

        while(true){
            
            variableId = 0;
            instant ++;
            printf("| %3d   |",instant);

            if(!isEmpty(block)){
                if(inputOutput == 0){
                    process[peek(block)].state = READY;
                    enqueue(dequeue(block), ready);
                    inputOutput = IO_TIME;
                }
                inputOutput--;
            }
            for(int i = 0; i< numOfPrograms; i++){
                if(process[i].state == EXIT){
                    freeProcess(i);
                    process[i].state == FINISH;
                }
                if(process[i].state == PRE_EXIT){
                    process[i].state = EXIT;
                    id_running = -1;
                }
            }
            if(id_running !=-1){
                quantum--;
                if(quantum == 0 && isEmpty(ready)){
                    quantum++;
                }
                if(quantum == 0){
                    enqueue(id_running,ready);
                    process[id_running].state = READY;
                    id_running = -1;
                }
                else{
                    variableId = executeProgram(id_running);
                    if( variableId == PrintVariable){
                        int var = mem[process[id_running].index-1];
                        if(process[id_running].isThread){
                            if(var >9){
                                int pos = process[process[id_running].pai].pc_vars + var;
                                output = mem[pos];
                            }
                            else{
                                output = mem[process[id_running].pc_vars+var];
                            }
                        }
                        else{
                            output = mem[process[id_running].pc_vars+var];
                        }
                    }
                }
            }
            l = 0;
            for( int i = 0; i < numOfPrograms; i++){
                Process p = process[i];
                if(p.state == NEW && programs[i].initial != instant ){
                    enqueue(i,ready);
                    process[i].state = READY;
                }
                else if(programs[i].initial == instant){
                    allocate(i);
                }
                if(process[i].state == NEW){
                    printf("%s%d ",process[i].tag,i+1);
                    if(p.id <= 9)
                        l += 3;
                    else
                        l += 4;
                }
            }
            for(l;l<newl;l++){
                printf(" ");
            }
            printf("|");
            if(id_running == -1 && !isEmpty(ready)){
                id_running = dequeue(ready);
                quantum = QUANTUM_TIME;
                process[id_running].state = RUN;
                variableId = executeProgram(id_running);
                if( variableId == PrintVariable){
                    int var = mem[process[id_running].index-1];
                    if(process[id_running].isThread){
                        if(var >9){
                            int pos = process[process[id_running].pai].pc_vars + var;
                            output = mem[pos];
                        }
                        else{
                            output = mem[process[id_running].pc_vars+var];
                        }
                    }
                    else{
                        output = mem[process[id_running].pc_vars+var];
                    }
                }
            }
            l= 0;
            Node aux = ready->front;
            while(aux != NULL){
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
            for(l;l<readyl;l++){
            printf(" ");
            }
            printf("|");
            if(id_running == -1){
                printf("     |");
            }
            else{
                Process p = process[id_running];
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
            while(aux != NULL){
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
            while(wb < numOfPrograms){
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
            for(l; l<blockl;++l){
                printf(" ");
            }
            printf("|");
            l = 0;
            for( int i = 0; i < numOfPrograms;i++){
                if(process[i].state==EXIT ){
                    printf("P%d ",i+1);
                    if(i < 9)
                        l += 3;
                    else
                        l += 4;
                }
            }
            for(l ;l < exitl; l++){
                printf(" ");
            }

            printf("|\n");
            if(process[id_running].state == PRE_EXIT){
                id_running = -1;
            }
            if(variableId == PrintVariable){
                printf(">Print %d \n",output);
            }
            if(variableId == InputOutputCall){
                process[id_running].state = BLOCKED;
                enqueue(id_running,block);
                id_running = -1;
            }
            if (variableId == WaitByThread){
                int thread = mem[process[id_running].index-1];
                process[id_running].isWaiting[thread-1] = true;
                process[id_running].state = WBT;
                id_running = -1;
                
            }
            if( variableId == SegmentationFault){
                if(id_running != -1){
                    printf("> Segmentation Error in P%d\n",id_running);
                    if(process[id_running].isThread){
                        Process p = process[id_running];
                        process[p.pai].isWaiting[p.idThread] = false;
                    }
                    id_running = -1;
                }

            }
            if( variableId == InvalidVariable){
               if(id_running != -1){
                printf("> Invalid Variable in P%d\n",id_running);
                id_running = -1;
                }
            }
            if(process[id_running].isThread){
                Process p = process[id_running];
                if(p.state == FINISH){
                    process[p.pai].isWaiting[p.idThread] = false;
                    id_running = -1;
                }
            }
            for(int c = 0; c < numOfPrograms; c++){
                if(process[c].state == WBT && canProced(c)){
                    process[c].state = READY;
                    enqueue(c,ready);
                }
            }
            
            int nOfProgramsRunning = 0;
            for(int i = 0; i < numOfPrograms; i++){
                if(process[i].state != FINISH){
                    nOfProgramsRunning++;
                }
            }
    
            if(nOfProgramsRunning == 0) break;
            if(instant == 100) break;
        }

}

void main(){
    char *path ="input1.txt";
    FILE *file = fopen(path, "r");
    if(file ==NULL){
        printf("Não foi possivel abrir o fichero!\n");
    }
    else{
        printf("Ficheiro aberto com sucesso!\n");
    }
    numOfPrograms = getNumOfPrograms(file);
    numOfProcess = numOfPrograms;
    readFile(file);
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
