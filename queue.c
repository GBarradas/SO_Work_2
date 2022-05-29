// Queue
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
typedef bool boolean,Boolean;
typedef struct dummyNode *Queue;
typedef struct node *Node;

struct dummyNode{
    Node front;
    Node rear;
    int size;
};

struct node{
    int element;
    Node next;
    Node prev;
};

Queue inicializeQueue(){
    Queue Q = malloc(sizeof(Queue));
    Q->size=0;
    Q->front=NULL;
    Q->rear =NULL;
}

Boolean isEmpty(Queue Q){
    if(Q != NULL){
        return Q->size ==0;
    }
}

void enqueue(int element , Queue Q){
    Node n = malloc(sizeof(Node));
    n->element = element;
    n->next = NULL;
    n->prev = NULL;
    if(isEmpty(Q)){
        Q->front = n;
        Q->rear = n;
        ++Q->size;
    }
    else{
        Node aux = Q->rear;
        aux->next=n;
        n->prev = aux;
        Q->rear = n;
        ++Q->size;
    }
}

int dequeue(Queue Q){
    if(!isEmpty(Q)){
        int element = Q->front->element;
        Node n = Q->front;
        if(Q->size == 1){
            Q->front = NULL;
            Q->rear = NULL;
        }
        else{
            Node aux = n->next;
            aux->prev = NULL;
            Q->front = aux;
        }
        free(n);
        --Q->size;
        return element;
    }
    return -1;
}

int peek(Queue Q){
    if (!isEmpty(Q)){
        return Q->front->element;
    }
    return -1;
}

void printQueue(Queue Q){
    Node aux = Q->front;
    while(aux != NULL){
        printf("%d ",aux->element);
        aux = aux->next;
    }
}
