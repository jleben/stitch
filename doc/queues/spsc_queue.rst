SPSC Queue
==========

.. cpp:class:: template<typename T> SPSC_Queue

Single-producer-single-consumer queue.

All methods have wait-free progress guarantee.

.. cpp:function:: template<typename T> SPSC_Queue<T>::SPSC_Queue(int capacity)

Constructs the queue with the given maximum capacity.

.. cpp:function:: template<typename T> int SPSC_Queue<T>::capacity() const

Returns the queue's capacity.

.. cpp:function:: template<typename T> bool SPSC_Queue<T>::empty() const

There is no elements in the queue.

.. cpp:function:: template<typename T> bool SPSC_Queue<T>::full() const

The number of elements in the queue is equal to its capacity.

.. cpp:function:: template<typename T> bool SPSC_Queue<T>::push(const T & value) const

Adds `value` to the back of the queue.
Returns true if successful.
This can fail if the queue is full,
in which case the queue is not changed and this method return false.

.. cpp:function:: template<typename T> bool SPSC_Queue<T>::pop(T & value) const

Removes an element from the front of the queue and stores it in `value`.
Returns true if successful.
This can fail if the queue is empty,
in which case the queue is not changed and this method return false.

.. cpp:function:: template<typename T, typename I> bool SPSC_Queue<T>::push(int count, I start) const

Adds `count` elements to the queue from successive positions starting at `start`.
`start` must satisfy the `InputIterator` concept.
Returns true if successful.
This can fail if the queue does not have space for `count` elements.
In that case the queue is not changed and this method return false.

.. cpp:function:: template<typename T, typename O> bool SPSC_Queue<T>::pop(int count, O start) const

Removes `count` elements from the queue and stores them at successive positions starting
at `start`. `start` must satisfy the `OutputIterator` concept.
Returns true if successful.
This can fail if the queue contains fewer than `count` elements.
In that case the queue is not changed and this method return false.

