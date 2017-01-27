/**
 * @file stack.h
 * @brief  My version of a stack, with limited space.
 *
 */

#ifndef __STACK_H
#define __STACK_H

#include "exception.h"

template <class T,int N> class Stack {
    const char *name;
public:
    Stack(){
        name="unnamed";
        ct=0;
    }
    
    ~Stack(){
        clear();
    }
    
    void setName(const char *s){
        name = s;
    }
    
    /// get an item from the top of the stack, discarding n items first.
    T pop(int n=0) {
        if(ct<=n)
            throw _("stack underflow in stack '%s'\n",name);
        ct -= n+1;
        return stack[ct];
    }
    
    /// just throw away N items
    void drop(int n){
        if(ct<n)
            throw _("stack underflow in stack '%s'\n",name);
        ct-=n;
    }
        
    
    /// get the nth item from the top of the stack
    T peek(int n=0) {
        if(ct<=n)
            throw _("stack underflow in stack '%s'\n",name);
        return stack[ct-(n+1)];
    }
    
    /// get a pointer to the nth item from the top of the stack
    T *peekptr(int n=0) {
        if(ct<=n)
            throw _("stack underflow in stack '%s'\n",name);
        return stack+(ct-(n+1));
    }
    
    /// get a pointer to an item from the top of the stack, discarding n items first,
    /// return NULL if there are not enough items instead of throwing an exception.
    T *popptrnoex(int n=0) {
        if(ct<=n)
            return NULL;
        ct -= n+1;
        return stack+ct;
    }
    
    /// get a pointer to an item from the top of the stack, discarding n items first
    T *popptr(int n=0) {
        if(ct<=n)
            throw _("stack underflow in stack '%s'\n",name);
        ct -= n+1;
        return stack+ct;
    }
    
    /// push an item onto the stack, returning the new item slot
    /// to be written into.
    T* pushptr() {
        if(ct==N)
            throw _("stack overflow in stack '%s'\n",name);
        return stack+(ct++);
    }
    
    /// push an item onto the stack, passing the object in - consider
    /// using pushptr(), it might be quicker.
    void push(T o){
        if(ct==N)
            throw _("stack overflow in stack '%s'\n",name);
        stack[ct++]=o;
    }
    
    /// swap the top two items on the stack
    void swap(){
        if(ct<2)
            throw _("stack underflow in stack '%s'\n",name);
        T x;
        x=stack[ct-1];
        stack[ct-1]=stack[ct-2];
        stack[ct-2]=x;
    }
    
    /// is the stack empty?
    bool isempty(){
        return ct==0;
    }
    
    /// empty the stack
    void clear() {
        ct=0;
    }
    
    /// clear the stack and run in-place destructors
    void cleardestroy(){
        while(ct)
            popptr()->~T();
    }
    
    /// left public for debugging handiness and stuff
    T stack[N];
    int ct;
};


#endif /* __STACK_H */
