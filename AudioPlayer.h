#pragma once

#include <stdio.h>
#include <iostream>
#include <QThread>
#include <QMutex>
#include <QByteArray>
#include <QQueue>

extern "C"
{
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_types.h>
#include <SDL_name.h>
#include <SDL_main.h>
#include <SDL_config.h>
}

using namespace std;

#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

class AudioPlayer : public QThread
{
	Q_OBJECT

public:
	AudioPlayer(QObject *parent);
	~AudioPlayer();

public:
	//sdlͨ���ص�������ȡ����
	static void audioCallback(void *userData, Uint8 *stream, int len);
	static void audioCallbaclEx(void *userData, Uint8 *stream, int len);

public slots:
    //��ʼ��������
    void onInitSDLAudio(int sampleRate, int channels, int samples);
	//������Ƶ����
	void onGetOneAudioBuffer(uint8_t *pAudioOutBuffer, int bufferSize, const QString strID);

protected:
	void run();

public:
	static Uint8 m_pAudioBuf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];  //��Ƶ��������
	static unsigned int m_pAudioBufSize; //������δ��ȡ�߻�����Ƶ���ݴ�С
	static Uint8 *m_pAudioBufIndex;  //sdlȡ����λ��
	static QMutex m_pMutexOpBuffer;
	static QMutex m_pMutexOpQue;
	static QQueue<Uint8 *> m_pDecodecBufferQue;  //�������

	QMutex m_pMutexThreadRun;
	bool m_pThreadRun;

	QMutex m_pMutexThreadSafelyOut;
	bool m_pThreadSafelyOut;
};
