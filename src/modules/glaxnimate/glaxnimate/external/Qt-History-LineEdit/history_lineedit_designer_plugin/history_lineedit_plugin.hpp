/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2012-2020 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef COLOR_WHEEL_PLUGIN_HPP
#define COLOR_WHEEL_PLUGIN_HPP

#include <QObject>
#include <QDesignerCustomWidgetInterface>

class History_LineEdit_Plugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    History_LineEdit_Plugin(QObject *parent = 0);

    void initialize(QDesignerFormEditorInterface *core);
    bool isInitialized() const;

    QWidget *createWidget(QWidget *parent);

    QString name() const;
    QString group() const;
    QIcon icon() const;
    QString toolTip() const;
    QString whatsThis() const;
    bool isContainer() const;

    QString domXml() const;

    QString includeFile() const;

private:
    bool initialized;
};


#endif // COLOR_WHEEL_PLUGIN_HPP
