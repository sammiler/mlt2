/*
 * SPDX-FileCopyrightText: 2013-2020 Mattia Basaglia
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef COLOR_WIDGETS_COLOR_PALETTE_WIDGET_PLUGIN_HPP
#define COLOR_WIDGETS_COLOR_PALETTE_WIDGET_PLUGIN_HPP

#include <QObject>
#include <QtUiPlugin/QDesignerCustomWidgetInterface>

class ColorPaletteWidget_Plugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    ColorPaletteWidget_Plugin(QObject *parent = 0);

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


#endif // COLOR_WIDGETS_COLOR_PALETTE_WIDGET_PLUGIN_HPP
