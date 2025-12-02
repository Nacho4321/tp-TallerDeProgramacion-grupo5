target_sources(taller_tests
    PRIVATE
    # .cpp files
    test_protocol.cpp
    test_acceptor_clienthandler.cpp
    test_client_communication.cpp
    test_full_integration.cpp
    test_lobby_protocol.cpp
    ${CMAKE_SOURCE_DIR}/server/acceptor.cpp
    ${CMAKE_SOURCE_DIR}/server/client_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/client_sender.cpp
    ${CMAKE_SOURCE_DIR}/server/client_receiver.cpp
    ${CMAKE_SOURCE_DIR}/server/lobby_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/game_monitor.cpp
    ${CMAKE_SOURCE_DIR}/client/game_client_receiver.cpp
    ${CMAKE_SOURCE_DIR}/client/game_client_sender.cpp
    ${CMAKE_SOURCE_DIR}/client/game_client_handler.cpp
    ${CMAKE_SOURCE_DIR}/client/game_connection.cpp
    ${CMAKE_SOURCE_DIR}/server/gameloop.cpp          
    ${CMAKE_SOURCE_DIR}/server/event.cpp   
    ${CMAKE_SOURCE_DIR}/server/map_layout.cpp
    ${CMAKE_SOURCE_DIR}/server/npc_config.cpp
    ${CMAKE_SOURCE_DIR}/server/eventloop.cpp
    ${CMAKE_SOURCE_DIR}/server/car_physics_config.cpp
    ${CMAKE_SOURCE_DIR}/server/game_event_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/gameloop/npc/npc_manager.cpp
    ${CMAKE_SOURCE_DIR}/server/gameloop/bridge/bridge_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/gameloop/checkpoint/checkpoint_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/gameloop/physics/physics_handler.cpp

    PUBLIC
    # .h files
    ${CMAKE_SOURCE_DIR}/server/client_sender.h
    ${CMAKE_SOURCE_DIR}/server/client_receiver.h
    )
