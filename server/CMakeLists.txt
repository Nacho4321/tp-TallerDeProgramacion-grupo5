target_sources(taller_server
    PRIVATE
    # .cpp files
    acceptor.cpp
    client_handler.cpp
    client_sender.cpp
    client_receiver.cpp
    outbox_monitor.cpp
    main.cpp
    server.cpp
    game_monitor.cpp
    message_handler.cpp
    PUBLIC
    # .h files
    acceptor.h
    client_handler.h
    client_sender.h
    client_receiver.h
    outbox_monitor.h
    server.h
    game_monitor.h
    message_handler.h
    )
