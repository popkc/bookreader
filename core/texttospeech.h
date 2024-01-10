/*
Copyright (C) 2020-2024 popkc(popkc at 163 dot com)
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef TEXTTOSPEECH_H
#define TEXTTOSPEECH_H

#include "pch.h"

namespace popkc {
class QVoice
{
public:
    QString name() const;
    CComPtr<ISpObjectToken> cpVoiceToken;
};

class QTextToSpeech : public QObject
{
    Q_OBJECT
public:
    enum State {
        Ready,
        Speaking,
        Paused,
        BackendError
    } state;

    QTextToSpeech(QObject *parent = nullptr);
    QVector<QVoice> availableVoices();
    void setVoice(const QVoice &v);
    void setPitch(double pitch);
    void setRate(double rate);
    void setVolume(double volume);
    void say(const QString &s);
    void stop();
    static void init();

    CComPtr<ISpVoice> cpVoice;
    WCHAR *speakString;
signals:
    void stateChanged(State state);

private:
    QVector<WCHAR> pitchXML;
};

void __stdcall sapiEventHandler(WPARAM wparam, LPARAM lparam);
}
#endif // TEXTTOSPEECH_H
