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

#ifndef OCR_H
#define OCR_H

#include <QObject>
#include <QFuture>

#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>

class QDir;

class Ocr : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.crow_translate.CrowTranslate.Ocr")
    Q_DISABLE_COPY(Ocr)

public:
    explicit Ocr(QObject *parent = nullptr);

    void setConvertLineBreaks(bool convert);

    QStringList availableLanguages() const;
    QByteArray languagesString() const;
    bool init(const QByteArray &languages, const QByteArray &languagesPath, const QMap<QString, QVariant> &parameters);

    void recognize(const QPixmap &pixmap, int dpi);
    void cancel();

    static QStringList availableLanguages(const QString &languagesPath);

public slots:
    Q_SCRIPTABLE void applyParameters(const QMap<QString, QVariant> &parameters, bool saveSettings = false);

signals:
    void recognized(const QString &text);
    void canceled();

private:
    static QStringList parseLanguageFiles(const QDir &directory);

    QMap<QString, QVariant> m_parameters;
    QFuture<void> m_future;

    tesseract::TessBaseAPI m_tesseract;
#if TESSERACT_MAJOR_VERSION < 5
    ETEXT_DESC m_monitor;
#else
    tesseract::ETEXT_DESC m_monitor;
#endif

    bool m_convertLineBreaks = false;
};

#endif // OCR_H
