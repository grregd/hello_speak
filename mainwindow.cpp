/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/



#include "mainwindow.h"
#include <algorithm>
#include <QLoggingCategory>
#include <QString>
#include <QStandardPaths>
#include <QMessageBox>
#include <QIcon>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_speech(nullptr)
{
    ui.setupUi(this);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.speech.tts=true \n qt.speech.tts.*=true"));

    QString folder;
    QString iniFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/dyktandor.txt");
    QFile config(iniFile);
    if (config.open(QIODevice::ReadOnly))
    {
        folder = QString(config.readLine()).remove("\n").remove("\r");
    }

    if (folder.isEmpty() || !currentDirectory.cd(folder))
    {
        QString filesFolder(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        filesFolder += "/Dyktandor";
        if (!currentDirectory.cd(filesFolder))
        {
            QMessageBox messageBox(QMessageBox::Critical, "DyktandorApp",
                                   QString("Nie można znaleźć pliku\n'%1'\nani katalogu\n'%2'.\n\nNie da się pracować.")
                                   .arg(iniFile).arg(filesFolder), QMessageBox::Ok);
            messageBox.exec();
            exit(-1);
        }
    }

    qDebug() << currentDirectory.dirName() << currentDirectory.entryList(QDir::NoDot | QDir::NoDotDot | QDir::Files);

    ui.listWidget->addItems(currentDirectory.entryList(QDir::NoDot |
                                        QDir::NoDotDot |
                                        QDir::Files));

    ui.schemas->addItem("Polish");
    ui.schemas->addItem("Polish Polish");
    ui.schemas->addItem("Polish English");
    ui.schemas->addItem("English Polish");

    // Populate engine selection list
    ui.engine->addItem("Default", QString("default"));
    foreach (QString engine, QTextToSpeech::availableEngines())
        ui.engine->addItem(engine, engine);
    ui.engine->setCurrentIndex(0);
    engineSelected(0);

    connect(ui.speakButton, &QPushButton::clicked, this, &MainWindow::speak);
    connect(ui.pitch, &QSlider::valueChanged, this, &MainWindow::setPitch);
    connect(ui.rate, &QSlider::valueChanged, this, &MainWindow::setRate);
    connect(ui.volume, &QSlider::valueChanged, this, &MainWindow::setVolume);
    connect(ui.engine, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::engineSelected);

    connect(ui.nextButton, &QPushButton::clicked, this, &MainWindow::sayNext);
    connect(ui.repeatButton, &QPushButton::clicked, this, &MainWindow::sayRepeat);

    ui.nextButton->setEnabled(false);
    ui.repeatButton->setEnabled(false);
}

void MainWindow::speak()
{
    qDebug() << ">>>>>>>>>>>>>>";
    qDebug() << ui.listWidget->currentItem();
    qDebug() << ui.listWidget->currentItem()->text();
    qDebug() << currentDirectory.absoluteFilePath(ui.listWidget->currentItem()->text());
    QFile textFile(currentDirectory.absoluteFilePath(ui.listWidget->currentItem()->text()));
    textFile.open(QIODevice::ReadOnly);

    m_scheme = QString(textFile.readLine()).remove("\r").remove("\n").split(" ");
    qDebug() << "m_scheme: " << m_scheme;

    hiddentText.clear();
    while (!textFile.atEnd())
        hiddentText += QString(textFile.readLine()).remove("\r");
    qDebug() << "hiddentText: " << hiddentText;
    qDebug() << "ui.plainTextEdit->toPlainText(): " << ui.plainTextEdit->toPlainText();
    qDebug() << "<<<<<<<<<<<<<<";


    currentWord.clear();
//    hiddentText = ui.plainTextEdit->toPlainText();
    ui.plainTextEdit->clear();

    ui.speakButton->setEnabled(false);
    ui.nextButton->setEnabled(true);
    ui.repeatButton->setEnabled(true);

    auto lines = hiddentText.split("\n");
    lines.erase(std::remove_if(
                    lines.begin(), lines.end(),
                    [](auto const & line){ return line.isEmpty(); }),
                lines.end());
    qDebug() << "lines: " << lines;

//    m_scheme = ui.schemas->currentText().split(" ");

    m_schemeLocale.clear();
    QVector<QLocale> availableLocales = m_speech->availableLocales();
    for (auto const & scheme: m_scheme)
    {
        auto sl = m_schemeLocale.size();
        for (auto const & locale: availableLocales)
        {
            if (QLocale::languageToString(locale.language()).contains(scheme, Qt::CaseInsensitive))
            {
                m_schemeLocale.push_back(locale);
                break;
            }
        }
        if (m_schemeLocale.size() == sl)
        {
            qWarning() << "No locale for " << scheme << ", using " << availableLocales.front() << " instead.";
            m_schemeLocale.push_back(availableLocales.front());
        }
    }

    qDebug() << "m_schemeLocale: " << m_schemeLocale;

    auto c = m_scheme.size();
    lines.erase(lines.end() - lines.size() % c, lines.end()); // make size divisible by c
    m_groups.clear();
    m_groups.resize(lines.size() / c);

    int i = 0;
    for (auto const & line: lines)
    {
        m_groups[i++/c].push_back(line);
    }

    qDebug() << "m_groups: " << m_groups;

    std::random_shuffle(m_groups.begin(), m_groups.end());

    m_currentGroup = m_groups.begin();
    m_currentText = ui.reverseSchema->isChecked()
            ? m_scheme.size()-1
            : 0;

    sayNext();
}

void MainWindow::stop()
{
    m_speech->stop();
}

void MainWindow::sayNext()
{
    if (m_currentGroup == m_groups.end())
    {
        addWordToPlainText();

        ui.nextButton->setEnabled(false);
        ui.repeatButton->setEnabled(false);
        ui.speakButton->setEnabled(true);
    }
    else
    {
        addWordToPlainText();

        currentWord = (*m_currentGroup)[m_currentText];
        m_speech->setLocale(m_schemeLocale[m_currentText]);

        qDebug() << "currentWord: " << currentWord;
        qDebug() << "currentLocale: " << m_speech->locale();

        m_speech->say(currentWord);

        auto reverse = ui.reverseSchema->isChecked();
        m_currentText += reverse ? -1 : 1;

        if ((reverse && m_currentText < 0) ||
            (!reverse && m_currentText >= m_scheme.size()))
        {
            ++m_currentGroup;
            m_currentText = reverse ? m_scheme.size()-1 : 0;
        }
    }
}

void MainWindow::sayRepeat()
{
    m_speech->say(currentWord);
}

void MainWindow::showOriginal()
{
    ui.plainTextEdit->insertPlainText(hiddentText);
}

void MainWindow::addWordToPlainText()
{
    qDebug() << __PRETTY_FUNCTION__
             << "m_currentText: " << m_currentText
             << "m_scheme.size(): " << m_scheme.size();

    ui.plainTextEdit->insertPlainText(currentWord + "\n");
    ui.plainTextEdit->ensureCursorVisible();
}

void MainWindow::setRate(int rate)
{
    m_speech->setRate(rate / 10.0);
}

void MainWindow::setPitch(int pitch)
{
    m_speech->setPitch(pitch / 10.0);
}

void MainWindow::setVolume(int volume)
{
    m_speech->setVolume(volume / 100.0);
}

void MainWindow::stateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Speaking) {
        ui.statusbar->showMessage("Speech started...");
    } else if (state == QTextToSpeech::Ready)
        ui.statusbar->showMessage("Speech stopped...", 2000);
    else if (state == QTextToSpeech::Paused)
        ui.statusbar->showMessage("Speech paused...");
    else
        ui.statusbar->showMessage("Speech error!");

    ui.pauseButton->setEnabled(state == QTextToSpeech::Speaking);
    ui.resumeButton->setEnabled(state == QTextToSpeech::Paused);
    ui.stopButton->setEnabled(state == QTextToSpeech::Speaking || state == QTextToSpeech::Paused);
}

void MainWindow::engineSelected(int index)
{
    QString engineName = ui.engine->itemData(index).toString();
    delete m_speech;
    if (engineName == "default")
        m_speech = new QTextToSpeech(this);
    else
        m_speech = new QTextToSpeech(engineName, this);
    disconnect(ui.language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::languageSelected);
    ui.language->clear();
    // Populate the languages combobox before connecting its signal.
    QVector<QLocale> locales = m_speech->availableLocales();
    QLocale current = m_speech->locale();
    qDebug() << "current locale: " << current;
    foreach (const QLocale &locale, locales) {
        QString name(QString("%1 (%2)")
                     .arg(QLocale::languageToString(locale.language()))
                     .arg(QLocale::countryToString(locale.country())));
        QVariant localeVariant(locale);
        ui.language->addItem(name, localeVariant);
        if (locale.name() == current.name())
            current = locale;
    }
    setRate(ui.rate->value());
    setPitch(ui.pitch->value());
    setVolume(ui.volume->value());
    connect(ui.stopButton, &QPushButton::clicked, m_speech, &QTextToSpeech::stop);
    connect(ui.pauseButton, &QPushButton::clicked, m_speech, &QTextToSpeech::pause);
    connect(ui.resumeButton, &QPushButton::clicked, m_speech, &QTextToSpeech::resume);

    connect(m_speech, &QTextToSpeech::stateChanged, this, &MainWindow::stateChanged);
    connect(m_speech, &QTextToSpeech::localeChanged, this, &MainWindow::localeChanged);

    connect(ui.language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::languageSelected);
    localeChanged(current);
}

void MainWindow::languageSelected(int language)
{
    QLocale locale = ui.language->itemData(language).toLocale();
    m_speech->setLocale(locale);
    qDebug() << "languageSelected" << "setLocale: " << locale;
}

void MainWindow::voiceSelected(int index)
{
    m_speech->setVoice(m_voices.at(index));
}

void MainWindow::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui.language->setCurrentIndex(ui.language->findData(localeVariant));

    disconnect(ui.voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::voiceSelected);
    ui.voice->clear();

    m_voices = m_speech->availableVoices();
    QVoice currentVoice = m_speech->voice();
    foreach (const QVoice &voice, m_voices) {
        ui.voice->addItem(QString("%1 - %2 - %3").arg(voice.name())
                          .arg(QVoice::genderName(voice.gender()))
                          .arg(QVoice::ageName(voice.age())));
        if (voice.name() == currentVoice.name())
            ui.voice->setCurrentIndex(ui.voice->count() - 1);
    }
    connect(ui.voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::voiceSelected);
}
