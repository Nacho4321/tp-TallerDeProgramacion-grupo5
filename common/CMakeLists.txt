target_sources(taller_common
    PRIVATE
    # .cpp files
    protocol.cpp
    liberror.cpp
    protocol_encode.cpp
    protocol_receivers.cpp
    protocol_utils.cpp
    resolver.cpp
    socket.cpp
    PUBLIC
    # .h files
    constants.h
    liberror.h
    protocol.h
    queue.h
    resolver.h
    socket.h
    thread.h
    messages.h
    )
