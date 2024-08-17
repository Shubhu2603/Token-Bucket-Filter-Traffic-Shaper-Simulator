/*
 * Author:      William Chia-Wei Cheng (bill.cheng@usc.edu)
 *
 * @(#)$Id: listtest.c,v 1.2 2020/05/18 05:09:12 william Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"
#include "my402list.h"



int  My402ListLength(My402List *List){
	return List->num_members;
}
int  My402ListEmpty(My402List *List){
	if(List->num_members!=0){
		return FALSE;
	}
	return TRUE;
}

int  My402ListAppend(My402List*List, void* obj){

	My402ListElem *newnode=(My402ListElem*)malloc(sizeof(My402ListElem));
	newnode->obj=obj;

	if(My402ListEmpty(List)){
		newnode->next=&(List->anchor);
		newnode->prev=&(List->anchor);
		(List->anchor).next=newnode;
		(List->anchor).prev=newnode;
	}
	else{
		My402ListElem *last=List->anchor.prev;
		newnode->prev=last;
		last->next=newnode;
		newnode->next=&(List->anchor);
		(List->anchor).prev=newnode;
	}
	List->num_members=List->num_members+1;
	return TRUE;
}
int  My402ListPrepend(My402List *List, void* obj){
	My402ListElem *newnode=(My402ListElem*)malloc(sizeof(My402ListElem));
	newnode->obj=obj;

	if(My402ListEmpty(List)){
		newnode->next=&(List->anchor);
		newnode->prev=&(List->anchor);
		(List->anchor).next=newnode;
		(List->anchor).prev=newnode;
	}
	else{
		My402ListElem *first=My402ListFirst(List);
		newnode->prev=&(List->anchor);
		first->prev=newnode;
		newnode->next=first;
		(List->anchor).next=newnode;
	}
	List->num_members=List->num_members+1;
	return TRUE;
}
void My402ListUnlink(My402List *List, My402ListElem *node){

	My402ListElem *prv=node->prev;
	My402ListElem *nxt=node->next;
	prv->next=nxt;
	nxt->prev=prv;
	free(node);
	List->num_members=List->num_members-1;
}
void My402ListUnlinkAll(My402List *List){
	My402ListElem *cur;
	My402ListElem *nxt;
	while(cur!=NULL){
		nxt=My402ListNext(List,cur);
		My402ListUnlink(List,cur);
		cur=nxt;
	}
	(List->anchor).next=NULL;
	(List->anchor).prev=NULL;

}
int  My402ListInsertAfter(My402List *List, void* obj, My402ListElem *node){
	My402ListElem *newnode=(My402ListElem*)malloc(sizeof(My402ListElem));

	if(node==NULL){
		return My402ListAppend(List,obj);
	}
	newnode->obj=obj;
	newnode->next=node->next;
	node->next->prev=newnode;
	newnode->prev=node;
	node->next=newnode;
	List->num_members=List->num_members+1;
	return TRUE;
}
int  My402ListInsertBefore(My402List *List, void* obj, My402ListElem *node){

	if(node==NULL){
		return My402ListPrepend(List,obj);
	}
	My402ListElem *newnode=(My402ListElem*)malloc(sizeof(My402ListElem));
	newnode->obj=obj;
	newnode->prev=node->prev;
	node->prev->next=newnode;
	newnode->next=node;
	node->prev=newnode;
	List->num_members=List->num_members+1;
	return TRUE;
}

My402ListElem *My402ListFirst(My402List *List){
	if(List->num_members==0){
		return NULL;
	}
	My402ListElem *node=List->anchor.next;
	return node;
}
My402ListElem *My402ListLast(My402List *List){
	if(List->num_members==0){
		return NULL;
	}
	My402ListElem *node=List->anchor.prev;
	return node;
}
My402ListElem *My402ListNext(My402List *List, My402ListElem *node){
	if(node==My402ListLast(List)){
		return NULL;
	}
	return node->next;
}
My402ListElem *My402ListPrev(My402List *List, My402ListElem *node){

	if(node==My402ListFirst(List)){
		return NULL;
	}
	return node->prev;
}

My402ListElem *My402ListFind(My402List *List, void* obj){

	My402ListElem *temp=NULL;
	for(temp=My402ListFirst(List);temp!=NULL;temp=My402ListNext(List,temp)){
		if(temp->obj==obj){
			return temp;
		}
	}
	return NULL;
}

int My402ListInit(My402List *List){

	List->num_members=0;
	List->anchor.next=&List->anchor;
	List->anchor.prev=&List->anchor;
	List->anchor.obj=NULL;
	return 0;
}