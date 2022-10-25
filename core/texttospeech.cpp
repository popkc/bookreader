/*
Copyright (C) 2020 popkc(popkcer at gmail dot com)
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
#include "core/texttospeech.h"
#include "mainwindow.h"
using namespace popkc;

QTextToSpeech::QTextToSpeech(QObject *parent) : QObject(parent)
{
    speakString=nullptr;
    HRESULT hr;
    hr=cpVoice.CoCreateInstance(CLSID_SpVoice);
    if(SUCCEEDED(hr))
        hr=cpVoice->SetInterest(SPFEI(SPEI_END_INPUT_STREAM), SPFEI(SPEI_END_INPUT_STREAM));
    if(SUCCEEDED(hr))
        hr=cpVoice->SetNotifyCallbackFunction(&popkc::sapiEventHandler, (WPARAM)this, 0);

    if(SUCCEEDED(hr))
        state=Ready;
    else
        state=BackendError;
}

QVector<QVoice> QTextToSpeech::availableVoices()
{
    QVector<QVoice> vlist;
    if(state==BackendError)
        return vlist;
    CComPtr<IEnumSpObjectTokens> cpEnum;
    if(FAILED(SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum)))
        return vlist;
    ULONG count;
    if(FAILED(cpEnum->GetCount(&count)))
        return vlist;
    vlist.resize(count);
    for(ULONG i=0; i<count; i++) {
        if(FAILED(cpEnum->Next(1, &vlist[i].cpVoiceToken, NULL))) {
            vlist.clear();
            return vlist;
        }
    }
    return vlist;
}

void QTextToSpeech::setVoice(const QVoice &v)
{
    if(state!=BackendError)
        cpVoice->SetVoice(v.cpVoiceToken);
}

void QTextToSpeech::setPitch(double pitch)
{
    if(state==BackendError)
        return;
    int p=pitch*10;
    QString s="<pitch absmiddle=\""+ QString::number(p)+"\"/>";
    pitchXML.resize(s.length());
    s.toWCharArray(pitchXML.data());
}

void QTextToSpeech::setRate(double rate)
{
    if(state!=BackendError)
        cpVoice->SetRate(rate*10);
}

void QTextToSpeech::setVolume(double volume)
{
    if(state!=BackendError)
        cpVoice->SetVolume(volume*100);
}

void QTextToSpeech::say(const QString &s)
{
    if(s.isEmpty() || state==BackendError)
        return;
    WCHAR *os=speakString;
    speakString=new WCHAR[pitchXML.size()+ s.length()+1];
    memcpy(speakString, pitchXML.constData(), pitchXML.size()*sizeof(WCHAR));
    s.toWCharArray(speakString+pitchXML.size());
    speakString[pitchXML.size()+ s.length()]=0;
    cpVoice->Speak(speakString, SPF_ASYNC|SPF_PURGEBEFORESPEAK, NULL);
    if(os)
        delete[] os;
}

void QTextToSpeech::stop()
{
    cpVoice->Skip(L"SENTENCE", INT_MAX, NULL);
}

void QTextToSpeech::init()
{
    CoInitialize(NULL);
}

void __stdcall popkc::sapiEventHandler(WPARAM wparam, LPARAM )
{
    QTextToSpeech *tts=reinterpret_cast<QTextToSpeech*>(wparam);
    SPEVENT spe;
    tts->cpVoice->GetEvents(1, &spe, NULL);
    tts->state=QTextToSpeech::Ready;
    if(tts->speakString) {
        delete[] tts->speakString;
        tts->speakString=nullptr;
    }
    emit tts->stateChanged(QTextToSpeech::Ready);
}

QString QVoice::name() const
{
    QString s;
    WCHAR *pw;
    CComPtr<ISpDataKey> dk;
    if(FAILED(cpVoiceToken->OpenKey(L"Attributes", &dk)))
        return s;
    if(FAILED(dk->GetStringValue(L"Name", &pw)))
        return s;
    s=QString::fromWCharArray(pw);
    CoTaskMemFree(pw);
    return s;
}
