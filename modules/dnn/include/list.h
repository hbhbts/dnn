/**
 * @file   list.h
 * @author Liangfu Chen <liangfu.chen@nlpr.ia.ac.cn>
 * @date   Mon Sep 22 15:39:18 2014
 * 
 * @brief  
 * 
 * 
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#ifndef __func__
#define __func__ __FUNCTION__
#endif
#endif

#define LOGE(fmt,...)                                   \
  do {                                                  \
  fprintf(stderr,"%s: %d: %s: error: " fmt "\n",__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__);   \
  }while(0)
#define LOGW(fmt,...)                                   \
  do {                                                  \
  fprintf(stderr,"%s: %d: %s: warning: " fmt "\n",__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__); \
  }while(0)
#define LOGI(fmt,...)                                   \
  do {                                                  \
  fprintf(stderr,"%s: %d: %s: info:" fmt "\n",__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__);    \
  }while(0)

template <typename T>
class Node
{
public:
  T data;
  Node * next;
  Node(T _data,Node * _next)//:data(_data),next(_next){}
  {
  data=_data;
  next=_next;
  }
};

template <typename T>
class List
{
  Node<T> * root;

public:
  List();
  ~List();
  void push_back(const T &value);
  void clear();
  int size();
  T operator [] (int idx);
  void erase(int idx);
};

template <typename T>
List<T>::List():root(0){};

template <typename T>
List<T>::~List(){};

template<typename T>
void List<T>::clear()
{
  Node<T> * p = root;
  while (p){
	p = root->next;
	if (root){delete root;}
	root = p;
  }
}

template<typename T>
void List<T>::push_back(const T &val)
{
  if (!root){
	root = new Node<T>(val,0);
  }else{
	Node<T> * ptr = root;
	while (ptr->next){
	  ptr = ptr->next;
	}
	ptr->next = new Node<T>(val,0);
  }
}

template<typename T>
int List<T>::size()
{
  int count=0;
  if (!root){return 0;}
  else{
	Node<T> * ptr = root;
	count++;
	while (ptr->next){
	  ptr = ptr->next;
	  count++;
	}
	return count;
  }
  return count;
}

template<typename T>
void List<T>::erase(int idx)
{
  int i=0;
  if (idx>=0){
    Node<T> * curr=root;
    Node<T> * prev=0;
    if (!curr){LOGE("insufficient elements");return;}
    for (i=0;i<idx;i++,prev=curr,curr=curr->next){
      if (!curr->next){LOGE("insufficient elements");return;}
    }
    if (!prev){
      if (!curr->next){
        delete root;root=0;
      }else{
        prev=root;
        root=curr->next;
        delete prev;prev=0;
      }
    }else if (!curr){
      delete prev;prev=0;
    }else{
      prev->next=curr->next;
      delete curr;curr=0;
    }
  }else{LOGE("invalid argument");}
}

template<typename T>
T List<T>::operator [] (int idx)
{
  if (idx>=0){
    Node<T> * ptr=root;
    if (!ptr){LOGE("insufficient elements");}
    int i=0;
    for (i=0;i<idx;i++,ptr=ptr->next){
      if (!ptr){LOGE("insufficient elements");}
    }
    return ptr->data;
  }else{LOGE("invalid argument");}
}

#endif // __LIST_H__
