#pragma once

#include <QLabel>
#include <QPushButton>
#include <QWidget>

class AlertWindowContents : public QWidget
{
    Q_OBJECT

public:
    AlertWindowContents(QWidget *parent);
    ~AlertWindowContents();

    void setIcon(const QString &path);
    void setTitle(const QString &title);
    void setTitleSize(int px);
    void setDescription(const QString &desc);
    void setDescriptionSize(int px);
    void setPrimaryButton(const QString &text);
    void setPrimaryButtonColor(const QColor &color);
    void setPrimaryButtonFontColor(const QColor &color);
    void setSecondaryButton(const QString &text);

signals:
    void sizeChanged();
    void primaryButtonClicked();
    void escapeClicked();

private slots:
    void onThemeChanged();

private:
    QLabel *icon_;
    QLabel *title_;
    QLabel *desc_;

    QPushButton *primaryButton_;
    QPushButton *secondaryButton_;
    QColor primaryButtonColor_;
    QColor primaryButtonFontColor_;

    int titleSize_;
    int descSize_;

    void updateDimensions();
};
