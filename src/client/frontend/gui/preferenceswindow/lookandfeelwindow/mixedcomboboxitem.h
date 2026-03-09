#pragma once

#include "commongraphics/iconbutton.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencesconst.h"
#include "selectfileitem.h"
#include "sounds/soundmanager.h"

namespace PreferencesWindow {

class MixedComboBoxItem : public ComboBoxItem
{
    Q_OBJECT
public:
    explicit MixedComboBoxItem(ScalableGraphicsObject *parent, SoundManager *soundManager = nullptr);
    ~MixedComboBoxItem() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setCurrentItem(QVariant value) override;
    void setPath(const QString &path);
    void setSecondaryItems(const QList<QPair<QString, QVariant>> &list, QVariant curValue = QVariant::fromValue(nullptr));

    void setCustomValue(const QVariant &value) { customValue_ = value; updateSecondaryItemVisibility(); }
    void setBundledValue(const QVariant &value) { bundledValue_ = value; updateSecondaryItemVisibility(); }
    QVariant customValue() const { return customValue_; }
    QVariant bundledValue() const { return bundledValue_; }

    void setDialogText(const QString &title, const QString &filter);

    void setEnablePreview(bool enable);
    bool isPreviewVisible() const { return previewButton_ != nullptr; }

signals:
    void pathChanged(QVariant value);

private slots:
    void onCurrentItemChanged(QVariant value);
    void onPathChanged(const QString &path);
    void updatePositions();
    void onPreviewButtonClicked();
    void onPreviewPlaybackFinished();

private:
    static constexpr int kSecondaryItemMarginTop = 24;
    static constexpr int kPreviewButtonMarginLeft = 4;

    SelectFileItem *selectFileItem_;
    ComboBoxItem *secondaryComboBox_;
    IconButton *previewButton_;
    SoundManager *soundManager_;

    QVariant customValue_ = "Custom";
    QVariant bundledValue_ = "Bundled";

    bool isPreviewEnabled_ = false;
    bool isPlaying_ = false;

    void updateSecondaryItemVisibility(bool signal = true);
    void updatePreviewButtonVisibility();
    QString getCurrentSoundPath() const;
};

} // namespace PreferencesWindow
