#ifndef __BASE_SYS_INC_DOUBLELINKEDLIST_H__
#define __BASE_SYS_INC_DOUBLELINKEDLIST_H__

#include "macroFuncs.h"

namespace parrot
{
template <typename T> class DoubleLinkedList
{
  public:
    DoubleLinkedList() : _count(0), _head(nullptr), _tail(nullptr)
    {
    }

    virtual ~DoubleLinkedList()
    {
        _head = _tail = nullptr;
    }

    DoubleLinkedList(const DoubleLinkedList&) = delete;
    DoubleLinkedList& operator=(const DoubleLinkedList&) = delete;

  public:
    void add(T* n) noexcept
    {
        PARROT_ASSERT(n != nullptr);

        if (!_head)
        {
            _head = _tail = n;
            ++_count;
            return;
        }

        n->setPrev(_tail);
        n->setNext(_tail->getNext());
        _tail->setNext(n);
        _tail = n;
        ++_count;
    }

    T* front() noexcept
    {
        return _head;
    }

    uint32_t count() const noexcept
    {
        return _count;
    }

    void remove(T* n) noexcept
    {
        PARROT_ASSERT(n != nullptr && n->getPrev() != nullptr);

        if (_count == 1)
        {
            _head = _tail = nullptr;
            --_count;
            n->setPrev(nullptr);
            n->setNext(nullptr);
            return;
        }

        if (n == _head)
        {
            _head = n->next();
            _head->setPrev(nullptr);
        }
        else if (n == _tail)
        {
            _tail = n->prev();
            _tail->setNext(nullptr);
        }
        else
        {
            n->getPrev()->setNext(n->getNext());
            n->getNext()->setPrev(n->getPrev());            
        }

        n->setPrev(nullptr);
        n->setNext(nullptr);
        --_count;
    }

  private:
    uint32_t _count;
    T* _head;
    T* _tail;
};
}

#endif
