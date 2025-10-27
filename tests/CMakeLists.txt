target_sources(taller_tests
    PRIVATE
    # .cpp files
    test_protocol.cpp
    test_acceptor_clienthandler.cpp
    test_client_communication.cpp
    foo.cpp
    ${CMAKE_SOURCE_DIR}/server/acceptor.cpp
    ${CMAKE_SOURCE_DIR}/server/client_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/outbox_monitor.cpp
    ${CMAKE_SOURCE_DIR}/common/game_client_receiver.cpp
    ${CMAKE_SOURCE_DIR}/common/game_client_sender.cpp
    PUBLIC
    # .h files
    )
