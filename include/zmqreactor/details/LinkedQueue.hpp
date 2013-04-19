#ifndef ZMQREACTOR_LINKEDQUEUE_HPP_
#define ZMQREACTOR_LINKEDQUEUE_HPP_

#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>

namespace ZmqReactor
{
  namespace Private
  {
    template <typename Object>
    class LinkedQueue;

    class LinkedBase
    {
    private:
      LinkedBase* prev_;
      LinkedBase* next_;

      template <typename Object>
      friend class LinkedQueue;

    protected:
      LinkedBase() :
        prev_(0), next_(0)
      {}
    };

    /**
     * To organize dynamically allocated objects in linked queue
     */
    template <typename Object>
    class LinkedQueue
    {
    private:
      BOOST_STATIC_ASSERT((boost::is_base_of<LinkedBase, Object>::value));

      Object* head_;

      inline
      static
      Object*
      to_obj(LinkedBase* base)
      {
        return static_cast<Object*>(static_cast<void*>(base));
      }

    public:
      LinkedQueue() : head_(0)
      {}

      ~LinkedQueue();

      void
      enqueue(Object* obj) throw();

      void
      dequeue(Object* obj) throw();

      Object*
      pop_head() throw();

      inline
      Object*
      head() const throw()
      {
        return head_;
      }

      inline
      Object*
      next(Object* obj) const throw()
      {
        return to_obj(obj->next_);
      }
    };

    /////////////////////// implementation //////////////////////////

    template <typename Object>
    LinkedQueue<Object>::~LinkedQueue()
    {
      while (head_)
      {
        Object* next = to_obj(head_->next_);
        delete head_;
        head_ = next;
      }
    }

    template <typename Object>
    void
    LinkedQueue<Object>::enqueue(Object* obj) throw()
    {
      obj->next_ = head_;
      obj->prev_ = 0;
      if (head_)
      {
        head_->prev_ = obj;
      }
      head_ = obj;
    }

    template <typename Object>
    void
    LinkedQueue<Object>::dequeue(Object* obj) throw()
    {
      if (obj->prev_)
      {
        obj->prev_->next_ = obj->next_;
      }
      if (obj->next_)
      {
        obj->next_->prev_ = obj->prev_;
      }
      if (head_ == obj)
      {
        head_ = to_obj(obj->prev_ ? obj->prev_ : obj->next_);
      }
    }

    template <typename Object>
    Object*
    LinkedQueue<Object>::pop_head() throw()
    {
      Object* obj = head_;
      if (head_)
      {
        if (head_->next_)
        {
          head_->next_->prev_ = 0;
        }
        head_ = head_->next_;
      }
      return obj;
    }
  }
}

#endif // ZMQREACTOR_LINKEDQUEUE_HPP_

