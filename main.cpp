#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("LALR(1) 语法分析生成器");

    MainWindow w;
    w.setWindowTitle("LALR(1) 语法分析生成器");
    w.resize(1000, 720);
    w.show();

    return app.exec();
}
