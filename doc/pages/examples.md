
# Examples {#examples}

# Events {#events}

@include event-reactor.cpp

# Stream Producers and Consumers {#streams}

Using a single producer and multiple consumers: the producer communicates notifications to all interested consumers.

@include stream-events.cpp

Using a single consumer and multiple producers: the producers communicate work requests to the consumer which does the work.

@include stream-work.cpp

# State and Observers {#state}

A state is updated by one thread while multiple other threads observe and react to the state changes.

@include states.cpp
