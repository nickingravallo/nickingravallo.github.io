#include <QApplication>
#include "gui/main_window.hpp"
#include "ompeval/hand_evaluator.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Initialize the hand evaluator (generates lookup tables)
    ompeval::HandEvaluator::instance();
    
    gui::MainWindow window;
    window.show();
    
    return app.exec();
}
