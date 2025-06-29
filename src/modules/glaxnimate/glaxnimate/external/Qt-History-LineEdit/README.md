History Line Edit
=================

Here is a QLineEdit that remembers and allows to navigate the history of lines 
entered in the widget
    
    
Demonstration
-------------

A simple demo of the widget is provided in ./history_lineedit_demo


Using it in a project
---------------------

Include history_lineedit.pri in the QMake project file. 
All the required files are in ./src


Installing as a Qt Designer/Creator Plugin
------------------------------------------

The sources for the designer plugin are in ./history_lineedit_designer_plugin

Compile the library and install in
(Qt SDK)/Tools/QtCreator/bin/designer/
(Qt SDK)/(Qt Version)/(Toolchain)/plugins/designer

cd history_lineedit_designer_plugin && qmake && make && make install


Latest Version
--------------

The latest version of the sources can be found at the following locations
https://gitlab.com/mattbas/Qt-History-LineEdit
git@gitlab.com:mattbas/Qt-History-LineEdit.git


License
-------

LGPLv3+, See COPYING
Copyright (C) 2012-2020 Mattia Basaglia <mattia.basaglia@gmail.com>
