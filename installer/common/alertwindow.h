#pragma once

#include <QLabel>
#include <QWidget>

#include "alertwindowcontents.h"

class AlertWindow : public QWidget
{
    Q_OBJECT

public:
    enum Style { kDefault, kQuit };

    AlertWindow(QWidget *parent);
    ~AlertWindow();

    void setStyle(Style style);
    void setIcon(const QString &path);
    void setTitle(const QString &title);
    void setTitleSize(int px);
    void setDescription(const QString &desc);
    void setPrimaryButton(const QString &text);
    void setPrimaryButtonColor(const QColor &color);
    void setPrimaryButtonFontColor(const QColor &color);
    void setSecondaryButton(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onContentsSizeChanged();

signals:
    void primaryButtonClicked();
    void escapeClicked();

private:
    AlertWindowContents *contents_;
};