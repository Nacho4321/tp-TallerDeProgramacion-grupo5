target_sources(taller_server
    PRIVATE
    # .cpp files
    acceptor.cpp
    client_handler.cpp
    outbox_monitor.cpp
    main.cpp
    server.cpp
    game_monitor.cpp
    message_admin.cpp
    PUBLIC
    # .h files
    acceptor.h
    client_handler.h
    outbox_monitor.h
    server.h
    game_monitor.h
    message_admin.h
    )
