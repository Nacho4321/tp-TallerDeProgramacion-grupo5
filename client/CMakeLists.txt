target_sources(taller_client
    PRIVATE
    # .cpp files
    main.cpp
    client.cpp
    game_client_receiver.cpp
    game_client_sender.cpp
    game_client_handler.cpp
    PUBLIC
    # .h files
    game_client_receiver.h
    game_client_sender.h
    game_client_handler.h)
    client.h
    )
