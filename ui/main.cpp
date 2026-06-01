//
// Created by Candy on 4/14/2026.
//

#include "mainwindow.h"
#include<QApplication>

int main(int argc , char *argv[])
{
    // Qt app bootstrap.
    // Build the window, show it, then let Qt take over the event loop from here.
    QApplication app(argc , argv);
    mainwindow window;

    window.show();
    return app.exec();
}
