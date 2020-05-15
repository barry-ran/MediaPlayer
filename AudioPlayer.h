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
	//sdl通过回调函数获取数据
	static void audioCallback(void *userData, Uint8 *stream, int len);
	static void audioCallbaclEx(void *userData, Uint8 *stream, int len);

public slots:
    //初始化播放器
    void onInitSDLAudio(int sampleRate, int channels, int samples);
	//保存音频数据
	void onGetOneAudioBuffer(uint8_t *pAudioOutBuffer, int bufferSize, const QString strID);

protected:
	void run();

public:
	static Uint8 m_pAudioBuf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];  //音频缓冲容器
	static unsigned int m_pAudioBufSize; //容器中未被取走缓存音频数据大小
	static Uint8 *m_pAudioBufIndex;  //sdl取数据位置
	static QMutex m_pMutexOpBuffer;
	static QMutex m_pMutexOpQue;
	static QQueue<Uint8 *> m_pDecodecBufferQue;  //解码队列

	QMutex m_pMutexThreadRun;
	bool m_pThreadRun;

	QMutex m_pMutexThreadSafelyOut;
	bool m_pThreadSafelyOut;
};
