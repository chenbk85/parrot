#ifndef __BASE_SYS_INC_DOUBLELINKEDLISTNODE_H__
#define __BASE_SYS_INC_DOUBLELINKEDLISTNODE_H__

namespace parrot {
// If this class is not a template, the client code needs to cast the
// DoubleLinkedListNode pointer to the child type after it gets the
// pointer.
template <typename T> class DoubleLinkedListNode {
  public:
    DoubleLinkedListNode() : _prevPointer(nullptr), _nextPointer(nullptr) {
    }

    virtual ~DoubleLinkedListNode() {
    }

  public:
    T *getNext() {
        return _nextPointer;
    }

    T *getPrev() {
        return _prevPointer;
    }

    void setNext(T *n) {
        _nextPointer = n;
    }

    void setPrev(T *n) {
        _prevPointer = n;
    }

  private:
    T *_prevPointer;
    T *_nextPointer;
};
}

#endif
