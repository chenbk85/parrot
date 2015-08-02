#ifndef __BASE_SYS_INC_DOUBLELINKEDLIST_H__
#define __BASE_SYS_INC_DOUBLELINKEDLIST_H__

#include "doubleLinkedListNode.h"
#include "macroFuncs.h"

namespace parrot
{
    template<typename T>
    class DoubleLinkedList final
    {
        using Node = DoubleLinkedListNode<T>;
        
      public:
        DoubleLinkedList():
            _count(0),
            _head(new Node()),
            _tail(_head)
        {
        }

        ~DoubleLinkedList()
        {
            delete _head;
            _head = _tail = nullptr;
        }
        
        DoubleLinkedList(const DoubleLinkedList&) = delete;
        DoubleLinkedList& operator=(const DoubleLinkedList&) = delete;

      public:
        void add(Node *n) noexcept
        {
            PARROT_ASSERT(n != nullptr && n->getPrev() == nullptr &&
                          n->getNext() == nullptr);
            n->setPrev(_tail);
            _tail->setNext(n);
            _tail = n;
            ++_count;
        }

        Node* front() noexcept
        {
            return _head->getNext();
        }

        uint32_t count() const noexcept
        {
            return _count;
        }
        
        void remove(Node *n) noexcept
        {
            PARROT_ASSERT(n != nullptr && n->getPrev() != nullptr);

            n->getPrev()->setNext(n->getNext());

            if (n->getNext() != nullptr)
            {
                n->getNext()->setPrev(n->getPrev());
            }
            else
            {
                // Here, n is the tail.
                _tail = n->getPrev();
            }

            --count;
        }

      private:
        uint32_t   _count;
        Node * _head;
        Node * _tail;
    };
}


#endif
