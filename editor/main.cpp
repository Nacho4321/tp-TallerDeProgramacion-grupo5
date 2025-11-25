#include <QApplication>
#include "checkpoint_editor_window.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    
    CheckpointEditorWindow editor;
    editor.show();
    
    return app.exec();
}
