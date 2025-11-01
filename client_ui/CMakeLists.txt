target_sources(taller_client_ui
    PRIVATE
    # .cpp files
    main.cpp
    ConnectDialog.cpp
    MainWindow.cpp

    # Designer
    ui/MainWindow.ui
    ui/ConnectDialog.ui

    # Recursos
    resources.qrc

    PUBLIC
    MainWindow.h
    ConnectDialog.h
    )
