// vim: filetype=cpp ts=4 sw=4 et tw=0 wm=0 cindent
/*
 * Dunnart - Constraint-based Diagram Editor
 *
 * Copyright (C) 2011  Monash University
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * 
 * Author: Michael Wybrow <mjwybrow@users.sourceforge.net>
*/

#include <QApplication>
#include <QFileOpenEvent>

#include "application.h"
#include "mainwindow.h"
#include "canvas.h"
#include "oldcanvas.h"

namespace dunnart {

Application::Application( int & argc, char ** argv)
    : CanvasApplication(argc, argv)
{
}


MainWindow *Application::mainWindow(void) const
{
    return (MainWindow *) CanvasApplication::mainWindow();
}


bool Application::openDiagram(QFileInfo *file)
{
    MainWindow *main_window = mainWindow();
    if (main_window)
    {
        return main_window->loadDiagram(file->absoluteFilePath());
    }
    return false;
}


bool Application::event(QEvent *ev)
{
    MainWindow *main_window = mainWindow();
    if (ev->type() == QEvent::FileOpen)
    {
        QString file = static_cast<QFileOpenEvent *>(ev)->file();
        if (main_window)
        {
            main_window->loadDiagram(file);
        }
        return true;
    }
    else if (ev->type() == QEvent::Close)
    {
        //QCloseEvent *closeEvent = static_cast<QCloseEvent *>(ev);
        return true;
    }
    return CanvasApplication::event(ev);
}


}
