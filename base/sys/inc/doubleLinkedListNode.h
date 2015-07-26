#ifndef __BASE_SYS_INC_DOUBLELINKEDLISTNODE_H__
#define __BASE_SYS_INC_DOUBLELINKEDLISTNODE_H__

namespace parrot
{
    template<typename T>
    class DoubleLinkedListNode
    {
      public:
        DoubleLinkedListNode():
            _prev(nullptr),
            _next(nullptr)
        {
        }
            
        virtual ~DoubleLinkedListNode()
        {
        }

      public:
        T* getNext()
        {
            return _next;
        }
        
        T* getPrev()
        {
            return _prev;
        }

        void setNext(T* n)
        {
            _next = n;
        }

        void setPrev(T* n)
        {
            _prev = n;
        }
        
      private:
        T * _prev;
        T * _next;
    };
}


#endif
