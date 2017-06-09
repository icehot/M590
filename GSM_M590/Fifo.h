// Fifo.h

#ifndef _FIFO_h
#define _FIFO_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#define MAX_ITEM 10 


template <class T>
struct Node
{
    T val;
    Node<T> *next;
};

template <class T> class Fifo
{
private:
    Node<T> *head = NULL;
    Node<T> *tail = NULL;
    unsigned int itemCount = 0;

public:
    Fifo()
    {
        head = NULL;
        tail = NULL;
        itemCount = 0;
    }

    ~Fifo()
    {
        clear();
    }

    bool isEmpty()
    {
        if (head == NULL && tail == NULL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool isFull()
    {
        if (itemCount < MAX_ITEM)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    int size()
    {
        if (head == NULL && tail == NULL) { return 0; }

        Node<T>* temp = head;
        int nodeCounter = 0;

        while (temp != NULL)
        {
            nodeCounter = nodeCounter + 1;
            temp = temp->next;
        }
        return nodeCounter;
    }

    void clear()
    {
        Node<T>* temp = head;
        while (temp != NULL)
        {
            temp = temp->next;
            delete(head);
            head = temp;
        }

        head = NULL;
        tail = NULL;
        itemCount = 0;
    }

    bool add(T element)
    {
        if (itemCount < MAX_ITEM)
        {
            Node<T>* newNode = new Node<T>();
            newNode->val = element;
            newNode->next = NULL;

            if (!isEmpty())
            {
                tail->next = newNode;
                tail = tail->next;
            }
            else
            {
                head = newNode;
                tail = newNode;
            }
            itemCount++;

            return true;
        }
        else
        {
            return false;
        }
    }


    bool remove(T* element)
    {
        /* Check if Fifo is empty */
        if (isEmpty())
        {
            return false;
        }
        else
        {
            /*Get first element */
            *element = head->val;

            /* Remove first element */
            Node<T>* temp;
            temp = head;
            if (head->next != NULL)
            { //Set the new head
                head = head->next;
            }
            else
            { // The last element is returned, the list will be empty 
                head = NULL;
                tail = NULL;
            }
            delete(temp);

            itemCount--;

            return true;
        }
    }

    bool peek(T* element)
    {
        if (isEmpty())
        {
            return false;
        }
        else
        {
            *element = head->val;
            return true;
        }
    }

    unsigned int getItemCount()
    {
        return itemCount;
    }
};

#endif

