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

#include "addlanguagedialog.h"
#include "ui_addlanguagedialog.h"

#include "languagebuttonswidget.h"

#include <QPushButton>
#include <QShortcut>

AddLanguageDialog::AddLanguageDialog(const QVector<QOnlineTranslator::Language> &currentLang, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddLanguageDialog)
    , m_searchShortcut(new QShortcut(QStringLiteral("Ctrl+F"), this))
{
    ui->setupUi(this);
    ui->searchEdit->setPlaceholderText(tr("Filter (%1)").arg(m_searchShortcut->key().toString()));
    connect(m_searchShortcut, &QShortcut::activated, ui->searchEdit, qOverload<>(&QLineEdit::setFocus));

    // Load languages
    for (int i = 1; i <= QOnlineTranslator::Zulu; ++i) {
        const auto lang = static_cast<QOnlineTranslator::Language>(i);
        if (!currentLang.contains(lang))
            addLanguage(ui->availableLanguagesListWidget, lang);
    }
    ui->availableLanguagesListWidget->setCurrentRow(0);

    for (QOnlineTranslator::Language lang : currentLang)
        addLanguage(ui->currentLanguagesListWidget, lang);

    if (ui->currentLanguagesListWidget->count() != 0) {
        ui->currentLanguagesListWidget->setCurrentRow(0);
        ui->moveLeftButton->setEnabled(true);
    }
}

AddLanguageDialog::~AddLanguageDialog()
{
    delete ui;
}

QVector<QOnlineTranslator::Language> AddLanguageDialog::languages() const
{
    return m_languages;
}

void AddLanguageDialog::accept()
{
    QDialog::accept();

    m_languages.reserve(ui->currentLanguagesListWidget->count());
    for (int i = 0; i < ui->currentLanguagesListWidget->count(); ++i) {
        QListWidgetItem *item = ui->currentLanguagesListWidget->item(i);
        m_languages.append(item->data(Qt::UserRole).value<QOnlineTranslator::Language>());
    }
}

void AddLanguageDialog::filterLanguages(const QString &text)
{
    bool isItemSelected = false;
    for (int i = 0; i < ui->availableLanguagesListWidget->count(); ++i) {
        QListWidgetItem *item = ui->availableLanguagesListWidget->item(i);
        if (item->text().contains(text, Qt::CaseInsensitive)) {
            item->setHidden(false);
            if (!isItemSelected) {
                ui->availableLanguagesListWidget->setCurrentItem(item); // Select first unhidden item
                isItemSelected = true;
            }
        } else {
            item->setHidden(true);
        }
    }

    // Disable Ok button if no item selected
    ui->dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(isItemSelected);
}

void AddLanguageDialog::moveLanguageRight()
{
    moveLanguageHorizontally(ui->availableLanguagesListWidget, ui->currentLanguagesListWidget, ui->moveRightButton, ui->moveLeftButton);
}

void AddLanguageDialog::moveLanguageLeft()
{
    // Block signals to emit index change after item deletion
    ui->currentLanguagesListWidget->blockSignals(true);
    moveLanguageHorizontally(ui->currentLanguagesListWidget, ui->availableLanguagesListWidget, ui->moveLeftButton, ui->moveRightButton);
    ui->currentLanguagesListWidget->blockSignals(false);

    emit ui->currentLanguagesListWidget->currentRowChanged(ui->currentLanguagesListWidget->currentRow());
}

void AddLanguageDialog::moveLanguageUp()
{
    moveLanguageVertically(ui->currentLanguagesListWidget, - 1);
}

void AddLanguageDialog::moveLanguageDown()
{
    moveLanguageVertically(ui->currentLanguagesListWidget, + 1);
}

void AddLanguageDialog::checkVerticalMovement(int row)
{
    if (row == -1) {
        ui->moveUpButton->setEnabled(false);
        ui->moveDownButton->setEnabled(false);
        return;
    }

    // Disable "Up" button for first element and "Down" for last
    ui->moveUpButton->setEnabled(row != 0);
    ui->moveDownButton->setEnabled(row != ui->currentLanguagesListWidget->count() - 1);

}

void AddLanguageDialog::addLanguage(QListWidget *widget, QOnlineTranslator::Language lang)
{
    auto *item = new QListWidgetItem;
    item->setText(QOnlineTranslator::languageName(lang));
    item->setIcon(LanguageButtonsWidget::countryIcon(lang));
    item->setData(Qt::UserRole, lang);
    widget->addItem(item);
}

void AddLanguageDialog::moveLanguageVertically(QListWidget *widget, int offset)
{
    const int currentRow = widget->currentRow();
    widget->insertItem(currentRow + offset, widget->takeItem(currentRow));
    widget->setCurrentRow(currentRow + offset);
}

void AddLanguageDialog::moveLanguageHorizontally(QListWidget *from, QListWidget *to, QAbstractButton *addButton, QAbstractButton *removeButton)
{
    QListWidgetItem *item = from->takeItem(from->currentRow());
    to->addItem(item);
    to->setCurrentItem(item);

    removeButton->setEnabled(true);
    if (from->count() == 0)
        addButton->setEnabled(false);
}
