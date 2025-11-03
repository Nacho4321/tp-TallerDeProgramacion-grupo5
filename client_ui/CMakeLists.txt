target_sources(taller_client_ui
    PRIVATE
    # .cpp files
    main.cpp
    ConnectionMenu.cpp
    MainWindow.cpp

    # Designer
    ui/MainWindow.ui
    ui/ConnectionMenu.ui

    # Recursos
    resources.qrc

    PUBLIC
    MainWindow.h
    ConnectionMenu.h
    )

target_sources(taller_client_ui PRIVATE
    ../client/client.cpp
    ../client/game_client_handler.cpp
    ../client/game_client_receiver.cpp
    ../client/game_client_sender.cpp
    ../client/input_handler.cpp
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
