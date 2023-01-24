#include <stdlib.h>
#include <stdio.h>
#include "steque.h"

void steque_init(steque_t *this){
  this->front = NULL;
  this->back = NULL;
  this->N = 0;
}

void steque_enqueue(steque_t* this, steque_item item){
  steque_node_t* node;

  node = (steque_node_t*) malloc(sizeof(steque_node_t));
  node->item = item;
  node->next = NULL;
  
  if(this->back == NULL)
    this->front = node;
  else
    this->back->next = node;

  this->back = node;
  this->N++;
}

void steque_push(steque_t* this, steque_item item){
  steque_node_t* node;

  node = (steque_node_t*) malloc(sizeof(steque_node_t));
  node->item = item;
  node->next = this->front;

  if(this->back == NULL)
    this->back = node;
  
  this->front = node;
  this->N++;
}

int steque_size(steque_t* this){
  return this->N;
}

int steque_isempty(steque_t *this){
  return this->N == 0;
}

steque_item steque_pop(steque_t* this){
  steque_item ans;
  steque_node_t* node;
  
  if(this->front == NULL){
    fprintf(stderr, "Error: underflow in steque_pop.\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
  }

  node = this->front;
  ans = node->item;

  this->front = this->front->next;
  if (this->front == NULL) this->back = NULL;
  free(node);

  this->N--;

  return ans;
}

void steque_cycle(steque_t* this){
  if(this->back == NULL)
    return;
  
  this->back->next = this->front;
  this->back = this->front;
  this->front = this->front->next;
  this->back->next = NULL;
}

steque_item steque_front(steque_t* this){
  if(this->front == NULL){
    fprintf(stderr, "Error: underflow in steque_front.\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  
  return this->front->item;
}

void steque_destroy(steque_t* this){
  while(!steque_isempty(this))
    steque_pop(this);
}
