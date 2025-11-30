target_sources(taller_client_ui
    PRIVATE
    # .cpp files
    main.cpp
    ConnectionMenu.cpp
    MainWindow.cpp
    NewGameWindow.cpp
    JoinGameWindow.cpp
    GameLobbyWindow.cpp
    GameLauncher.cpp
    LobbyClient.cpp
    CarSelectionDialog.cpp

    # Designer
    ui/MainWindow.ui
    ui/ConnectionMenu.ui
    ui/NewGameWindow.ui
    ui/JoinGameWindow.ui
    ui/GameLobbyWindow.ui
    ui/CarSelectionDialog.ui

    # Recursos
    resources.qrc

    PUBLIC
    MainWindow.h
    ConnectionMenu.h
    NewGameWindow.h
    JoinGameWindow.h
    GameLobbyWindow.h
    GameLauncher.h
    LobbyClient.h
    CarSelectionDialog.h
    )

target_sources(taller_client_ui PRIVATE
    ../client/client.cpp
    ../client/game_client_handler.cpp
    ../client/game_client_receiver.cpp
    ../client/game_client_sender.cpp
    ../client/game_connection.cpp
    ../client/input_handler.cpp
    ../client/car.cpp
    ../client/camera.cpp
    ../client/checkpoint.cpp
    ../client/minimap.cpp
    ../client/game_renderer.cpp
    ../client/audio_manager.cpp
    ../client/results_screen.cpp
)

target_include_directories(taller_client_ui PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${libSDL2pp_SOURCE_DIR}
)

target_link_libraries(taller_client_ui PRIVATE
    SDL2pp::SDL2pp
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)
