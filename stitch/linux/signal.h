#pragma once

#include "events.h"
#include "../connections.h"

#include <memory>

namespace Stitch {

namespace Detail {

struct SignalChannel
{
    SignalChannel();
    ~SignalChannel();
    void notify();
    void clear();
    int fd;
};

}

class Signal_Sender;
class Signal_Receiver;
void connect(Signal_Sender&, Signal_Receiver&);
void disconnect(Signal_Sender&, Signal_Receiver&);

/*!
  \brief Notifies other threads using an Event.

  This class lets one or more threads wait for
  an Event which is activated from another thread.

  Note that only one Event is provided, so
  it can be handled by multiple threads only
  if they are all waiting for it to be activated
  at the same time.

  \sa
  \ref Signal_Sender for notifying multiple threads
  independently (though less efficiently).
*/
class Signal
{

public:
    Signal();
    ~Signal();

    /*! \brief Activate the \ref event. */
    void notify();

    /*! \brief Wait for \ref event to be activated. */
    void wait() { Stitch::wait(event()); }

    /*! \brief Returns a momentary event activated when \ref notify is called. */
    Event event();

private:
    void clear();

    int d_fd;
};

class Signal_Receiver;

/*! \brief Notifies one or more \ref Signal_Receiver "Signal_Receivers" using individual Events.

  This class allows multiple threads to be notified
  independently of each other.
  Specifically, a \ref Signal_Receiver can \ref Stitch::connect(Signal_Sender&, Signal_Receiver&) "connect"
  to a Signal_Sender to receive notifications using its own
  \ref Event.

  \sa
  \ref Signal for notifying threads more efficiently using a single Event.
*/

class Signal_Sender : Client<Detail::SignalChannel>
{
public:
    friend void connect(Signal_Sender&, Signal_Receiver&);
    friend void disconnect(Signal_Sender&, Signal_Receiver&);

    void notify()
    {
        for(auto & receiver : *this)
        {
            receiver.notify();
        }
    }
};

/*! \brief Receives notifications from one ore more \ref Signal_Sender.

  \sa
  \ref Signal for notifying threads more efficiently using a single Event.
*/
class Signal_Receiver : Server<Detail::SignalChannel>
{
public:
    friend void connect(Signal_Sender&, Signal_Receiver&);
    friend void disconnect(Signal_Sender&, Signal_Receiver&);

    Event event();

    void wait() { Stitch::wait(event()); }
};

/*! \brief Make `sender` activate `receiver`'s Event. */
inline
void connect(Signal_Sender& sender, Signal_Receiver& receiver)
{
    Stitch::connect(static_cast<Client<Detail::SignalChannel>&>(sender),
                    static_cast<Server<Detail::SignalChannel>&>(receiver));
}

/*! \brief Disconnet `sender` and `receiver`. */
inline
void disconnect(Signal_Sender& sender, Signal_Receiver& receiver)
{
    Stitch::disconnect(static_cast<Client<Detail::SignalChannel>&> (sender),
                       static_cast<Server<Detail::SignalChannel>&> (receiver));
}

}

