target_sources(taller_common
    PRIVATE
    # .cpp files
    foo.cpp
    eventloop.cpp
    eventDispatcher.cpp
    PUBLIC
    # .h files
    foo.h
    eventloop.h
    queue.h
    thread.h
    Event.h
    eventDispatcher.h
    )
