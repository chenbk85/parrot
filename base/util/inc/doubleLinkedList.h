#ifndef __BASE_SYS_INC_DOUBLELINKEDLIST_H__
#define __BASE_SYS_INC_DOUBLELINKEDLIST_H__

#include "macroFuncs.h"

namespace parrot
{
template <typename T> class DoubleLinkedList
{
  public:
    DoubleLinkedList() : _count(0), _head(new T()), _tail(_head)
    {
    }

    virtual ~DoubleLinkedList()
    {
        delete _head;
        _head = _tail = nullptr;
    }

    DoubleLinkedList(const DoubleLinkedList&) = delete;
    DoubleLinkedList& operator=(const DoubleLinkedList&) = delete;

  public:
    void add(T* n) noexcept
    {
        PARROT_ASSERT(n != nullptr);
        n->setPrev(_tail);
        n->setNext(_tail->getNext());
        _tail->setNext(n);
        _tail = n;
        ++_count;
    }

    T* front() noexcept
    {
        return _head->getNext();
    }

    uint32_t count() const noexcept
    {
        return _count;
    }

    void remove(T* n) noexcept
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

        --_count;
    }

  private:
    uint32_t _count;
    T* _head;
    T* _tail;
};
}

#endif
