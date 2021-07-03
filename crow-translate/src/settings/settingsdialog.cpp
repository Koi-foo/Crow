/*
 *  Copyright © 2018-2021 Hennadii Chernyshchyk <genaloner@gmail.com>
 *
 *  This file is part of Crow Translate.
 *
 *  Crow Translate is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Crow Translate is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a get of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "languagebuttonswidget.h"
#include "mainwindow.h"
#include "qhotkey.h"
#include "screenwatcher.h"
#include "trayicon.h"
#include "ocr/ocr.h"
#include "shortcutsmodel/shortcutitem.h"
#include "shortcutsmodel/shortcutsmodel.h"
#ifdef Q_OS_WIN
#include "qgittag.h"
#include "updaterdialog.h"
#endif

#include <QDate>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QStandardItemModel>

SettingsDialog::SettingsDialog(MainWindow *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_translator(new QOnlineTranslator(this))
#ifdef WITH_PORTABLE_MODE
    , m_portableCheckbox(new QCheckBox(tr("Portable mode"), this))
#endif
{
    ui->setupUi(this);
    if (!ScreenWatcher::isWidthFitScreen(this))
        activateCompactMode();

    connect(ui->dialogButtonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &SettingsDialog::restoreDefaults);
    connect(ui->globalShortcutsCheckBox, &QCheckBox::toggled, ui->shortcutsTreeView->model(), &ShortcutsModel::setGlobalShortuctsEnabled);
    ui->logoLabel->setPixmap(QIcon::fromTheme(QStringLiteral("crow-translate")).pixmap(512, 512));
    ui->versionLabel->setText(QCoreApplication::applicationVersion());

#ifdef WITH_PORTABLE_MODE
    m_portableCheckbox->setToolTip(tr("Use %1 from the application folder to store settings").arg(AppSettings::portableConfigName()));
    qobject_cast<QFormLayout *>(ui->generalGroupBox->layout())->addRow(m_portableCheckbox);
#endif

    // Test voice
    ui->playerButtons->setMediaPlayer(new QMediaPlayer);
    connect(m_translator, &QOnlineTranslator::finished, this, &SettingsDialog::speakTestText);

    // Set item data in comboboxes
    ui->localeComboBox->addItem(tr("<System language>"), QLocale::AnyLanguage);
    const QMetaEnum languagesEnum = QMetaEnum::fromType<QLocale::Language>();
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/al.svg")), languagesEnum.valueToKey(QLocale::Albanian), QLocale::Albanian);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/cn.svg")), languagesEnum.valueToKey(QLocale::Chinese), QLocale::Chinese);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/nl.svg")), languagesEnum.valueToKey(QLocale::Dutch), QLocale::Dutch);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/gb.svg")), languagesEnum.valueToKey(QLocale::English), QLocale::English);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/fr.svg")), languagesEnum.valueToKey(QLocale::French), QLocale::French);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/id.svg")), languagesEnum.valueToKey(QLocale::Indonesian), QLocale::Indonesian);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/it.svg")), languagesEnum.valueToKey(QLocale::Italian), QLocale::Italian);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/my.svg")), languagesEnum.valueToKey(QLocale::Malay), QLocale::Malay);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/pl.svg")), languagesEnum.valueToKey(QLocale::Polish), QLocale::Polish);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/br.svg")), languagesEnum.valueToKey(QLocale::Portuguese), QLocale::Portuguese);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/ru.svg")), languagesEnum.valueToKey(QLocale::Russian), QLocale::Russian);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/es.svg")), languagesEnum.valueToKey(QLocale::Spanish), QLocale::Spanish);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/tr.svg")), languagesEnum.valueToKey(QLocale::Turkish), QLocale::Turkish);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/cn.svg")), languagesEnum.valueToKey(QLocale::Uighur), QLocale::Uighur);
    ui->localeComboBox->addItem(QIcon(QStringLiteral(":/icons/flags/ua.svg")), languagesEnum.valueToKey(QLocale::Ukrainian), QLocale::Ukrainian);

    ui->primaryLangComboBox->addItem(tr("<System language>"), QOnlineTranslator::Auto);
    ui->secondaryLangComboBox->addItem(tr("<System language>"), QOnlineTranslator::Auto);
    for (int i = 1; i <= QOnlineTranslator::Zulu; ++i) {
        const auto lang = static_cast<QOnlineTranslator::Language>(i);
        const QIcon langIcon = LanguageButtonsWidget::countryIcon(lang);

        ui->primaryLangComboBox->addItem(langIcon, QOnlineTranslator::languageName(lang), i);
        ui->secondaryLangComboBox->addItem(langIcon, QOnlineTranslator::languageName(lang), i);
    }

    ui->ocrLanguagesListWidget->addLanguages(parent->ocr()->availableLanguages());

    // Sort languages in comboboxes alphabetically
    ui->primaryLangComboBox->model()->sort(0);
    ui->secondaryLangComboBox->model()->sort(0);

    // Set maximum and minimum values for the size of the popup window
    ui->popupWidthSlider->setMaximum(QGuiApplication::primaryScreen()->availableGeometry().width());
    ui->popupWidthSpinBox->setMaximum(QGuiApplication::primaryScreen()->availableGeometry().width());
    ui->popupHeightSlider->setMaximum(QGuiApplication::primaryScreen()->availableGeometry().height());
    ui->popupHeightSpinBox->setMaximum(QGuiApplication::primaryScreen()->availableGeometry().height());
    ui->popupWidthSlider->setMinimum(200);
    ui->popupWidthSpinBox->setMinimum(200);
    ui->popupHeightSlider->setMinimum(200);
    ui->popupHeightSpinBox->setMinimum(200);

    // Disable (enable) opacity slider if "Window mode" ("Popup mode") selected
    connect(ui->windowModeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), ui->popupOpacityLabel, &QSlider::setDisabled);
    connect(ui->windowModeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), ui->popupOpacitySlider, &QSlider::setDisabled);

#ifdef Q_OS_WIN
    // Add information about icons
    auto *iconsTitleLabel = new QLabel(this);
    iconsTitleLabel->setText(tr("Icons:"));

    auto *iconsLabel = new QLabel(this);
    iconsLabel->setText("<a href=\"https://github.com/yeyushengfan258/We10X-icon-theme\">We10X</a>");
    iconsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    iconsLabel->setOpenExternalLinks(true);

    qobject_cast<QFormLayout *>(ui->aboutGroupBox->layout())->addRow(iconsTitleLabel, iconsLabel);

    // Add updater options
    auto *updatesGroupBox = new QGroupBox(tr("Updates"));
    qobject_cast<QVBoxLayout *>(ui->generalPage->layout())->insertWidget(1, updatesGroupBox);

    auto *updatesLayout = new QHBoxLayout;
    updatesGroupBox->setLayout(updatesLayout);

    auto *checkForUpdatesLabel = new QLabel;
    checkForUpdatesLabel->setText(tr("Check for updates:"));
    updatesLayout->addWidget(checkForUpdatesLabel);

    m_checkForUpdatesComboBox = new QComboBox;
    m_checkForUpdatesComboBox->addItem(tr("Every day"));
    m_checkForUpdatesComboBox->addItem(tr("Every week"));
    m_checkForUpdatesComboBox->addItem(tr("Every month"));
    m_checkForUpdatesComboBox->addItem(tr("Never"));
    updatesLayout->addWidget(m_checkForUpdatesComboBox);

    m_checkForUpdatesButton = new QPushButton;
    m_checkForUpdatesButton->setText(tr("Check now"));
    m_checkForUpdatesButton->setToolTip(tr("Check for updates now"));
    connect(m_checkForUpdatesButton, &QPushButton::clicked, this, &SettingsDialog::downloadUpdatesInfo);
    updatesLayout->addWidget(m_checkForUpdatesButton);

    m_checkForUpdatesStatusLabel = new QLabel;
    updatesLayout->addWidget(m_checkForUpdatesStatusLabel);

    updatesLayout->addStretch();
#endif

    // Check current date
    const QDate date = QDate::currentDate();
    if ((date.month() == 12 && date.day() == 31) || (date.month() == 1 && date.day() == 1))
        ui->testSpeechEdit->setText(tr("Happy New Year!"));

    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    if (!ui->tesseractParametersTableWidget->validateParameters()) {
        QMessageBox msgBox;
        msgBox.setText(tr("The OCR parameter fields can not be empty."));
        msgBox.setInformativeText(tr("Do you want to discard the invalid parameters?"));
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setIcon(QMessageBox::Warning);
        if (msgBox.exec() == QMessageBox::No) {
            // Navigate to OCR
            ui->pagesListWidget->setCurrentRow(ui->pagesStackedWidget->indexOf(ui->ocrPage));
            return;
        }
    }

    QDialog::accept();

    // Set settings location first
#ifdef WITH_PORTABLE_MODE
    AppSettings::setPortableModeEnabled(m_portableCheckbox->isChecked());
#endif

    // General settings
    AppSettings settings;
    settings.setLanguage(ui->localeComboBox->currentData().value<QLocale::Language>());
    settings.setMainWindowOrientation(static_cast<Qt::ScreenOrientation>(ui->mainWindowOrientationComboBox->currentIndex()));
    settings.setWindowMode(static_cast<AppSettings::WindowMode>(ui->windowModeComboBox->currentIndex()));
    settings.setTranslationNotificationTimeout(ui->translationNotificationTimeoutSpinBox->value());
    settings.setShowTrayIcon(ui->showTrayIconCheckBox->isChecked());
    settings.setStartMinimized(ui->startMinimizedCheckBox->isChecked());
    AppSettings::setAutostartEnabled(ui->autostartCheckBox->isChecked());
#ifdef Q_OS_WIN
    settings.setCheckForUpdatesInterval(static_cast<AppSettings::Interval>(m_checkForUpdatesComboBox->currentIndex()));
#endif

    // Interface settings
    QFont font = ui->fontNameComboBox->currentFont();
    font.setPointSize(ui->fontSizeSpinBox->value());
    settings.setFont(font);

    settings.setPopupOpacity(static_cast<double>(ui->popupOpacitySlider->value()) / 100);
    settings.setPopupWidth(ui->popupWidthSpinBox->value());
    settings.setPopupHeight(ui->popupHeightSpinBox->value());

    settings.setMainWindowLanguageFormat(static_cast<AppSettings::LanguageFormat>(ui->mainWindowLanguageFormatComboBox->currentIndex()));
    settings.setPopupLanguageFormat(static_cast<AppSettings::LanguageFormat>(ui->popupLanguageFormatComboBox->currentIndex()));

    settings.setTrayIconType(static_cast<AppSettings::IconType>(ui->trayIconComboBox->currentIndex()));
    settings.setCustomIconPath(ui->customTrayIconEdit->text());

    // Translation settings
    settings.setSourceTranslitEnabled(ui->sourceTranslitCheckBox->isChecked());
    settings.setTranslationTranslitEnabled(ui->translationTranslitCheckBox->isChecked());
    settings.setSourceTranscriptionEnabled(ui->sourceTranscriptionCheckBox->isChecked());
    settings.setTranslationOptionsEnabled(ui->translationOptionsCheckBox->isChecked());
    settings.setExamplesEnabled(ui->examplesCheckBox->isChecked());
    settings.setSimplifySource(ui->sourceSimplificationCheckBox->isChecked());
    settings.setPrimaryLanguage(ui->primaryLangComboBox->currentData().value<QOnlineTranslator::Language>());
    settings.setSecondaryLanguage(ui->secondaryLangComboBox->currentData().value<QOnlineTranslator::Language>());
    settings.setForceSourceAutodetect(ui->forceSourceAutodetectCheckBox->isChecked());
    settings.setForceTranslationAutodetect(ui->forceTranslationAutodetectCheckBox->isChecked());

    // OCR
    settings.setConvertLineBreaks(ui->convertLineBreaksCheckBox->isChecked());
    settings.setOcrLanguagesPath(ui->ocrLanguagesPathEdit->text().toLocal8Bit());
    settings.setOcrLanguagesString(ui->ocrLanguagesListWidget->checkedLanguagesString());
    settings.setRegionRememberType(static_cast<AppSettings::RegionRememberType>(ui->rememberRegionComboBox->currentIndex()));
    settings.setCaptureDelay(ui->captureDelaySpinBox->value());
    settings.setShowMagnifier(ui->showMagnifierCheckBox->isChecked());
    settings.setConfirmOnRelease(ui->confirmOnReleaseCheckBox->isChecked());
    settings.setApplyLightMask(ui->applyLightMaskCheckBox->isChecked());
    settings.setTesseractParameters(ui->tesseractParametersTableWidget->parameters());

    // Speech synthesis settings
    settings.setVoice(QOnlineTranslator::Yandex, ui->playerButtons->voice(QOnlineTranslator::Yandex));
    settings.setEmotion(QOnlineTranslator::Yandex, ui->playerButtons->emotion(QOnlineTranslator::Yandex));

    // Connection settings
    settings.setProxyType(static_cast<QNetworkProxy::ProxyType>(ui->proxyTypeComboBox->currentIndex()));
    settings.setProxyHost(ui->proxyHostEdit->text());
    settings.setProxyPort(static_cast<quint16>(ui->proxyPortSpinbox->value()));
    settings.setProxyAuthEnabled(ui->proxyAuthCheckBox->isChecked());
    settings.setProxyUsername(ui->proxyUsernameEdit->text());
    settings.setProxyPassword(ui->proxyPasswordEdit->text());

    // Shortcuts
    if (QHotkey::isPlatformSupported())
        settings.setGlobalShortcutsEnabled(ui->globalShortcutsCheckBox->isChecked());
    ui->shortcutsTreeView->model()->saveShortcuts(settings);
}

void SettingsDialog::setCurrentPage(int index)
{
    // Ignore size police for hidden pages to show scrollbar only for visible page
    // https://wiki.qt.io/Technical_FAQ#How_can_I_get_a_QStackedWidget_to_automatically_switch_size_depending_on_the_content_of_the_page.3F
    ui->pagesStackedWidget->currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->pagesStackedWidget->setCurrentIndex(index);
    ui->pagesStackedWidget->currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void SettingsDialog::onProxyTypeChanged(int type)
{
    if (type == QNetworkProxy::HttpProxy || type == QNetworkProxy::Socks5Proxy) {
        ui->proxyHostEdit->setEnabled(true);
        ui->proxyHostLabel->setEnabled(true);
        ui->proxyPortLabel->setEnabled(true);
        ui->proxyPortSpinbox->setEnabled(true);
        ui->proxyInfoLabel->setEnabled(true);
        ui->proxyAuthCheckBox->setEnabled(true);
    } else {
        ui->proxyHostEdit->setEnabled(false);
        ui->proxyHostLabel->setEnabled(false);
        ui->proxyPortLabel->setEnabled(false);
        ui->proxyPortSpinbox->setEnabled(false);
        ui->proxyInfoLabel->setEnabled(false);
        ui->proxyAuthCheckBox->setEnabled(false);
    }
}

// Update "Show tray Icon" checkbox state when “Notification" or "Popup" mode selected
void SettingsDialog::onWindowModeChanged(int mode)
{
    if (mode == AppSettings::Notification || mode == AppSettings::PopupWindow) {
        ui->showTrayIconCheckBox->setDisabled(true);
        ui->showTrayIconCheckBox->setChecked(true);
    } else {
        ui->showTrayIconCheckBox->setEnabled(true);
    }
}

// Disable (enable) "Custom icon path" option
void SettingsDialog::onTrayIconTypeChanged(int type)
{
    if (type == AppSettings::CustomIcon) {
        ui->customTrayIconLabel->setEnabled(true);
        ui->customTrayIconEdit->setEnabled(true);
        ui->customTrayIconButton->setEnabled(true);
    } else {
        ui->customTrayIconLabel->setEnabled(false);
        ui->customTrayIconEdit->setEnabled(false);
        ui->customTrayIconButton->setEnabled(false);
    }
}

void SettingsDialog::selectCustomTrayIcon()
{
    const QString path = ui->customTrayIconEdit->text().left(ui->customTrayIconEdit->text().lastIndexOf(QDir::separator()));
    const QString file = QFileDialog::getOpenFileName(this, tr("Select icon"), path, tr("Images (*.png *.ico *.svg *.jpg);;All files()"));
    if (!file.isEmpty())
        ui->customTrayIconEdit->setText(file);
}

void SettingsDialog::setCustomTrayIconPreview(const QString &iconPath)
{
    ui->customTrayIconButton->setIcon(TrayIcon::customTrayIcon(iconPath));
}

void SettingsDialog::selectOcrLanguagesPath()
{
    const QString path = ui->ocrLanguagesPathEdit->text().left(ui->ocrLanguagesPathEdit->text().lastIndexOf(QDir::separator()));
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Select OCR languages path"), path);
    if (!directory.isEmpty())
        ui->ocrLanguagesPathEdit->setText(directory);
}

void SettingsDialog::onOcrLanguagesPathChanged(const QString &path)
{
    ui->ocrLanguagesListWidget->clear();
    ui->ocrLanguagesListWidget->addLanguages(Ocr::availableLanguages(path));
}

void SettingsDialog::onTesseractParametersCurrentItemChanged()
{
    if (ui->tesseractParametersTableWidget->currentRow() == -1)
        ui->tesseractParametersRemoveButton->setEnabled(false);
    else
        ui->tesseractParametersRemoveButton->setEnabled(true);
}

// Disable unsupported voice settings for engines.
void SettingsDialog::showAvailableTtsOptions(int engine)
{
    // Avoid index changed signal
    ui->voiceComboBox->blockSignals(true);
    ui->emotionComboBox->blockSignals(true);

    switch (engine) {
    case QOnlineTranslator::Bing:
        setSpeechTestEnabled(false);
        setVoiceOptions({});
        setEmotionOptions({});
        break;
    case QOnlineTranslator::Google:
        setSpeechTestEnabled(true);
        setVoiceOptions({});
        setEmotionOptions({});
        break;
    case QOnlineTranslator::Yandex:
        setSpeechTestEnabled(true);
        setVoiceOptions({{tr("Zahar"), QOnlineTts::Zahar},
                         {tr("Ermil"), QOnlineTts::Ermil},
                         {tr("Jane"), QOnlineTts::Jane},
                         {tr("Oksana"), QOnlineTts::Oksana},
                         {tr("Alyss"), QOnlineTts::Alyss},
                         {tr("Omazh"), QOnlineTts::Omazh}});
        setEmotionOptions({{tr("Neutral"), QOnlineTts::Neutral},
                           {tr("Good"), QOnlineTts::Good},
                           {tr("Evil"), QOnlineTts::Evil}});

        const QOnlineTts::Voice voice = ui->playerButtons->voice(QOnlineTranslator::Yandex);
        const QOnlineTts::Emotion emotion = ui->playerButtons->emotion(QOnlineTranslator::Yandex);

        ui->voiceComboBox->setCurrentIndex(ui->voiceComboBox->findData(voice));
        ui->emotionComboBox->setCurrentIndex(ui->emotionComboBox->findData(emotion));
        break;
    }

    ui->voiceComboBox->blockSignals(false);
    ui->emotionComboBox->blockSignals(false);
}

// Save current engine voice settings
void SettingsDialog::saveEngineVoice(int voice)
{
    ui->playerButtons->setVoice(static_cast<QOnlineTranslator::Engine>(ui->engineComboBox->currentIndex()), ui->voiceComboBox->itemData(voice).value<QOnlineTts::Voice>());
}

// Save current engine emotion settings
void SettingsDialog::saveEngineEmotion(int emotion)
{
    ui->playerButtons->setEmotion(static_cast<QOnlineTranslator::Engine>(ui->engineComboBox->currentIndex()), ui->emotionComboBox->itemData(emotion).value<QOnlineTts::Emotion>());
}

// To play test text
void SettingsDialog::detectTextLanguage()
{
    if (ui->testSpeechEdit->text().isEmpty()) {
        QMessageBox::information(this, tr("Nothing to play"), tr("Playback text is empty"));
        return;
    }

    const auto engine = static_cast<QOnlineTranslator::Engine>(ui->engineComboBox->currentIndex());
    m_translator->detectLanguage(ui->testSpeechEdit->text(), engine);
}

void SettingsDialog::speakTestText()
{
    if (m_translator->error() != QOnlineTranslator::NoError) {
        QMessageBox::critical(this, tr("Unable to detect language"), m_translator->errorString());
        return;
    }

    const auto engine = static_cast<QOnlineTranslator::Engine>(ui->engineComboBox->currentIndex());
    ui->playerButtons->speak(ui->testSpeechEdit->text(), m_translator->sourceLanguage(), engine);
}

void SettingsDialog::loadShortcut(ShortcutItem *item)
{
    if (item->childCount() == 0) {
        ui->shortcutGroupBox->setEnabled(true);
        ui->shortcutSequenceEdit->setKeySequence(item->shortcut());
    } else {
        ui->shortcutGroupBox->setEnabled(false);
        ui->shortcutSequenceEdit->clear();
    }
}

void SettingsDialog::updateAcceptButton()
{
    if (ui->shortcutsTreeView->currentItem()->shortcut() != ui->shortcutSequenceEdit->keySequence())
        ui->acceptShortcutButton->setEnabled(true);
    else
        ui->acceptShortcutButton->setEnabled(false);
}

void SettingsDialog::acceptCurrentShortcut()
{
    ui->shortcutsTreeView->currentItem()->setShortcut(ui->shortcutSequenceEdit->keySequence());
    ui->acceptShortcutButton->setEnabled(false);
}

void SettingsDialog::clearCurrentShortcut()
{
    ui->shortcutSequenceEdit->clear();
    ui->acceptShortcutButton->setEnabled(true);
}

void SettingsDialog::resetCurrentShortcut()
{
    ui->shortcutsTreeView->currentItem()->resetShortcut();
    ui->shortcutSequenceEdit->setKeySequence(ui->shortcutsTreeView->currentItem()->shortcut());
    ui->acceptShortcutButton->setEnabled(false);
}

void SettingsDialog::resetAllShortcuts()
{
    ui->shortcutsTreeView->model()->resetAllShortcuts();
}

#ifdef Q_OS_WIN
void SettingsDialog::downloadUpdatesInfo()
{
    m_checkForUpdatesButton->setEnabled(false);
    m_checkForUpdatesStatusLabel->setText(tr("Checking for updates..."));

    // Get update information
    auto *release = new QGitTag(this);
    connect(release, &QGitTag::finished, this, &SettingsDialog::checkForUpdates);
    release->get("crow-translate", "crow-translate");
}

void SettingsDialog::checkForUpdates()
{
    auto *release = qobject_cast<QGitTag *>(sender());
    release->deleteLater();
    m_checkForUpdatesButton->setEnabled(true);

    if (release->error()) {
        m_checkForUpdatesStatusLabel->setStyleSheet("color: red");
        m_checkForUpdatesStatusLabel->setText(release->errorString());
        return;
    }

    if (const int installer = release->assetId(".exe"); QCoreApplication::applicationVersion() < release->tagName() && installer != -1) {
        m_checkForUpdatesStatusLabel->setStyleSheet("color: green");
        m_checkForUpdatesStatusLabel->setText(tr("Update available!"));
        auto *updaterDialog = new UpdaterDialog(release, installer, this);
        updaterDialog->setAttribute(Qt::WA_DeleteOnClose);
        updaterDialog->open();
    } else {
        m_checkForUpdatesStatusLabel->setStyleSheet("");
        m_checkForUpdatesStatusLabel->setText(tr("No updates available."));
    }

    AppSettings settings;
    settings.setLastUpdateCheckDate(QDate::currentDate());
}
#endif

void SettingsDialog::restoreDefaults()
{
    // General settings
    ui->localeComboBox->setCurrentIndex(ui->localeComboBox->findData(AppSettings::defaultLanguage()));
    ui->mainWindowOrientationComboBox->setCurrentIndex(AppSettings::defaultMainWindowOrientation());
    ui->windowModeComboBox->setCurrentIndex(AppSettings::defaultWindowMode());
    ui->translationNotificationTimeoutSpinBox->setValue(AppSettings::defaultTranslationNotificationTimeout());
    ui->showTrayIconCheckBox->setChecked(AppSettings::defaultShowTrayIcon());
    ui->startMinimizedCheckBox->setChecked(AppSettings::defaultStartMinimized());
    ui->autostartCheckBox->setChecked(AppSettings::defaultAutostartEnabled());
#ifdef Q_OS_WIN
    m_checkForUpdatesComboBox->setCurrentIndex(AppSettings::defaultCheckForUpdatesInterval());
#endif

    // Interface settings
    const QFont defaultFont = QApplication::font();
    ui->fontNameComboBox->setCurrentFont(defaultFont);
    ui->fontSizeSpinBox->setValue(defaultFont.pointSize());

    ui->popupOpacitySlider->setValue(static_cast<int>(AppSettings::defaultPopupOpacity() * 100));
    ui->popupWidthSpinBox->setValue(AppSettings::defaultPopupWidth());
    ui->popupHeightSpinBox->setValue(AppSettings::defaultPopupHeight());

    ui->mainWindowLanguageFormatComboBox->setCurrentIndex(AppSettings::defaultMainWindowLanguageFormat());
    ui->popupLanguageFormatComboBox->setCurrentIndex(AppSettings::defaultPopupLanguageFormat());

    ui->trayIconComboBox->setCurrentIndex(AppSettings::defaultTrayIconType());
    ui->customTrayIconEdit->setText(AppSettings::defaultCustomIconPath());

    // Translation settings
    ui->sourceTranslitCheckBox->setChecked(AppSettings::defaultSourceTranslitEnabled());
    ui->translationTranslitCheckBox->setChecked(AppSettings::defaultTranslationTranslitEnabled());
    ui->sourceTranscriptionCheckBox->setChecked(AppSettings::defaultSourceTranscriptionEnabled());
    ui->translationOptionsCheckBox->setChecked(AppSettings::defaultTranslationOptionsEnabled());
    ui->examplesCheckBox->setChecked(AppSettings::defaultExamplesEnabled());
    ui->sourceSimplificationCheckBox->setChecked(AppSettings::defaultSimplifySource());
    ui->primaryLangComboBox->setCurrentIndex(ui->primaryLangComboBox->findData(AppSettings::defaultPrimaryLanguage()));
    ui->secondaryLangComboBox->setCurrentIndex(ui->secondaryLangComboBox->findData(AppSettings::defaultSecondaryLanguage()));
    ui->forceSourceAutodetectCheckBox->setChecked(AppSettings::defaultForceSourceAutodetect());
    ui->forceTranslationAutodetectCheckBox->setChecked(AppSettings::defaultForceTranslationAutodetect());

    // OCR
    ui->convertLineBreaksCheckBox->setChecked(AppSettings::defaultConvertLineBreaks());
    ui->ocrLanguagesPathEdit->setText(AppSettings::defaultOcrLanguagesPath());
    ui->ocrLanguagesListWidget->setCheckedLanguages(AppSettings::defaultOcrLanguagesString());
    ui->rememberRegionComboBox->setCurrentIndex(AppSettings::defaultRegionRememberType());
    ui->captureDelaySpinBox->setValue(AppSettings::defaultCaptureDelay());
    ui->showMagnifierCheckBox->setChecked(AppSettings::defaultShowMagnifier());
    ui->confirmOnReleaseCheckBox->setChecked(AppSettings::defaultConfirmOnRelease());
    ui->applyLightMaskCheckBox->setChecked(AppSettings::defaultApplyLightMask());
    ui->tesseractParametersTableWidget->setParameters(AppSettings::defaultTesseractParameters());

    // Speech synthesis settings
    ui->playerButtons->setVoice(QOnlineTranslator::Yandex, AppSettings::defaultVoice(QOnlineTranslator::Yandex));
    ui->playerButtons->setEmotion(QOnlineTranslator::Yandex, AppSettings::defaultEmotion(QOnlineTranslator::Yandex));

    // Connection settings
    ui->proxyTypeComboBox->setCurrentIndex(AppSettings::defaultProxyType());
    ui->proxyHostEdit->setText(AppSettings::defaultProxyHost());
    ui->proxyPortSpinbox->setValue(AppSettings::defaultProxyPort());
    ui->proxyAuthCheckBox->setChecked(AppSettings::defaultProxyAuthEnabled());
    ui->proxyUsernameEdit->setText(AppSettings::defaultProxyUsername());
    ui->proxyPasswordEdit->setText(AppSettings::defaultProxyPassword());

    // Shortcuts
    if (QHotkey::isPlatformSupported())
        ui->globalShortcutsCheckBox->setEnabled(AppSettings::defaultGlobalShortcutsEnabled());
    resetAllShortcuts();
}

void SettingsDialog::activateCompactMode()
{
    setWindowState(windowState() | Qt::WindowMaximized);

    ui->pagesListWidget->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->scrollArea->hide();

    auto *backButton = new QPushButton(QIcon::fromTheme("arrow-left"), tr("Back"));
    backButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->settingsDialogLayout->insertWidget(0, backButton);
    backButton->hide();

    connect(backButton, &QPushButton::clicked, backButton, &QPushButton::hide);
    connect(backButton, &QPushButton::clicked, ui->scrollArea, &QScrollArea::hide);
    connect(backButton, &QPushButton::clicked, ui->pagesListWidget, &QListWidget::show);
    connect(ui->pagesListWidget, &QListWidget::itemActivated, ui->pagesListWidget, &QListWidget::hide);
    connect(ui->pagesListWidget, &QListWidget::itemActivated, backButton, &QPushButton::show);
    connect(ui->pagesListWidget, &QListWidget::itemActivated, ui->scrollArea, &QScrollArea::show);
    connect(ui->pagesListWidget, &QListWidget::itemClicked, ui->pagesListWidget, &QListWidget::hide);
    connect(ui->pagesListWidget, &QListWidget::itemClicked, backButton, &QPushButton::show);
    connect(ui->pagesListWidget, &QListWidget::itemClicked, ui->scrollArea, &QScrollArea::show);
}

void SettingsDialog::loadSettings()
{
    // General settings
    const AppSettings settings;
    ui->localeComboBox->setCurrentIndex(ui->localeComboBox->findData(settings.language()));
    ui->mainWindowOrientationComboBox->setCurrentIndex(settings.mainWindowOrientation());
    ui->translationNotificationTimeoutSpinBox->setValue(settings.translationNotificationTimeout());
    ui->windowModeComboBox->setCurrentIndex(settings.windowMode());
    ui->showTrayIconCheckBox->setChecked(settings.isShowTrayIcon());
    ui->startMinimizedCheckBox->setChecked(settings.isStartMinimized());
    ui->autostartCheckBox->setChecked(AppSettings::isAutostartEnabled());
#ifdef WITH_PORTABLE_MODE
    m_portableCheckbox->setChecked(settings.isPortableModeEnabled());
#endif
#ifdef Q_OS_WIN
    m_checkForUpdatesComboBox->setCurrentIndex(settings.checkForUpdatesInterval());
#endif

    // Interface settings
    const QFont font = settings.font();
    ui->fontNameComboBox->setCurrentFont(font);
    ui->fontSizeSpinBox->setValue(font.pointSize());

    ui->popupOpacitySlider->setValue(static_cast<int>(settings.popupOpacity() * 100));
    ui->popupWidthSpinBox->setValue(settings.popupWidth());
    ui->popupHeightSpinBox->setValue(settings.popupHeight());

    ui->mainWindowLanguageFormatComboBox->setCurrentIndex(settings.mainWindowLanguageFormat());
    ui->popupLanguageFormatComboBox->setCurrentIndex(settings.popupLanguageFormat());

    ui->trayIconComboBox->setCurrentIndex(settings.trayIconType());
    ui->customTrayIconEdit->setText(settings.customIconPath());

    // Translation settings
    ui->sourceTranslitCheckBox->setChecked(settings.isSourceTranslitEnabled());
    ui->translationTranslitCheckBox->setChecked(settings.isTranslationTranslitEnabled());
    ui->sourceTranscriptionCheckBox->setChecked(settings.isSourceTranscriptionEnabled());
    ui->translationOptionsCheckBox->setChecked(settings.isTranslationOptionsEnabled());
    ui->examplesCheckBox->setChecked(settings.isExamplesEnabled());
    ui->sourceSimplificationCheckBox->setChecked(settings.isSimplifySource());
    ui->primaryLangComboBox->setCurrentIndex(ui->primaryLangComboBox->findData(settings.primaryLanguage()));
    ui->secondaryLangComboBox->setCurrentIndex(ui->secondaryLangComboBox->findData(settings.secondaryLanguage()));
    ui->forceSourceAutodetectCheckBox->setChecked(settings.isForceSourceAutodetect());
    ui->forceTranslationAutodetectCheckBox->setChecked(settings.isForceTranslationAutodetect());

    // OCR
    ui->convertLineBreaksCheckBox->setChecked(settings.isConvertLineBreaks());
    ui->ocrLanguagesPathEdit->setText(settings.ocrLanguagesPath());
    ui->ocrLanguagesListWidget->setCheckedLanguages(settings.ocrLanguagesString());
    ui->rememberRegionComboBox->setCurrentIndex(settings.regionRememberType());
    ui->captureDelaySpinBox->setValue(settings.captureDelay());
    ui->showMagnifierCheckBox->setChecked(settings.isShowMagnifier());
    ui->confirmOnReleaseCheckBox->setChecked(settings.isConfirmOnRelease());
    ui->applyLightMaskCheckBox->setChecked(settings.isApplyLightMask());
    ui->tesseractParametersTableWidget->setParameters(settings.tesseractParameters());

    // Speech synthesis settings
    ui->playerButtons->setVoice(QOnlineTranslator::Yandex, settings.voice(QOnlineTranslator::Yandex));
    ui->playerButtons->setEmotion(QOnlineTranslator::Yandex, settings.emotion(QOnlineTranslator::Yandex));

    // Connection settings
    ui->proxyTypeComboBox->setCurrentIndex(settings.proxyType());
    ui->proxyHostEdit->setText(settings.proxyHost());
    ui->proxyPortSpinbox->setValue(settings.proxyPort());
    ui->proxyAuthCheckBox->setChecked(settings.isProxyAuthEnabled());
    ui->proxyUsernameEdit->setText(settings.proxyUsername());
    ui->proxyPasswordEdit->setText(settings.proxyPassword());

    // Shortcuts
    if (QHotkey::isPlatformSupported()) {
        ui->globalShortcutsCheckBox->setChecked(settings.isGlobalShortuctsEnabled());
    } else {
        ui->globalShortcutsCheckBox->setChecked(false);
        ui->globalShortcutsCheckBox->setEnabled(false);
    }
    ui->shortcutsTreeView->model()->loadShortcuts(settings);
}

void SettingsDialog::setVoiceOptions(const QMap<QString, QOnlineTts::Voice> &voices)
{
    ui->voiceComboBox->clear();

    if (voices.isEmpty()) {
        // Disable voice settings
        ui->voiceLabel->setEnabled(false);
        ui->voiceComboBox->setEnabled(false);
        ui->voiceComboBox->addItem(tr("Default"));
        return;
    }

    ui->voiceLabel->setEnabled(true);
    ui->voiceComboBox->setEnabled(true);
    for (auto it = voices.cbegin(); it != voices.cend(); ++it)
        ui->voiceComboBox->addItem(it.key(), it.value());
}

void SettingsDialog::setEmotionOptions(const QMap<QString, QOnlineTts::Emotion> &emotions)
{
    ui->emotionComboBox->clear();

    if (emotions.isEmpty()) {
        // Disable emotion settings
        ui->emotionLabel->setEnabled(false);
        ui->emotionComboBox->setEnabled(false);
        ui->emotionComboBox->addItem(tr("Default"));
        return;
    }

    ui->emotionLabel->setEnabled(true);
    ui->emotionComboBox->setEnabled(true);
    for (auto it = emotions.cbegin(); it != emotions.cend(); ++it)
        ui->emotionComboBox->addItem(it.key(), it.value());
}

void SettingsDialog::setSpeechTestEnabled(bool enabled)
{
    ui->testSpeechEdit->setEnabled(enabled);
    ui->playerButtons->setEnabled(enabled);
    ui->testSpeechLabel->setEnabled(enabled);
}
