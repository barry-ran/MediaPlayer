#include "AudioPlayer.h"

Uint8 AudioPlayer::m_pAudioBuf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2] = { 0 };
unsigned int AudioPlayer::m_pAudioBufSize = 0;
Uint8* AudioPlayer::m_pAudioBufIndex = m_pAudioBuf;
QMutex AudioPlayer::m_pMutexOpBuffer;
QMutex AudioPlayer::m_pMutexOpQue;
QQueue<Uint8*> AudioPlayer::m_pDecodecBufferQue;

AudioPlayer::AudioPlayer(QObject *parent)
	: QThread(parent)
{
	m_pThreadRun = true;
	m_pThreadSafelyOut = true;
}

AudioPlayer::~AudioPlayer()
{
	SDL_Quit();

	QMutexLocker locker(&m_pMutexThreadRun);
	m_pThreadRun = false;
	while (true)
	{
		QMutexLocker locker(&m_pMutexThreadSafelyOut);
		if (m_pThreadSafelyOut)
		{
			break;
		}
		msleep(100);
	}

	for (; m_pDecodecBufferQue.count() > 0; )
	{
		unsigned char *c = m_pDecodecBufferQue.dequeue();
		delete[] c;
	}
}

void AudioPlayer::audioCallback(void * userData, Uint8 * stream, int len)
{
	static const int Max_audio_data_size = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
	static uint8_t audio_buf[Max_audio_data_size];//缓存区
	static unsigned int audio_buf_size = 0;//缓存区大小
	static unsigned int audio_buf_index = 0;//当前应当取出缓存所在首地址
	int len1;//用于记录每次取出缓存大小
	int audio_data_size;//缓存最大长度

	SDL_memset(stream, 0, len);
	while (len > 0)
	{
		//当索引位置超过最大时，表示缓存应该更新了
		if (audio_buf_index >= audio_buf_size)
		{
			audio_buf_size = 0;
			QMutexLocker locker(&m_pMutexOpQue);
			if (m_pDecodecBufferQue.count() > 0)
			{
				while (m_pDecodecBufferQue.count() > 0)
				{
					unsigned char* c = m_pDecodecBufferQue.dequeue();
					int nBuffSize = *((int*)c);
					if (audio_buf_size + nBuffSize >= Max_audio_data_size)
					{
						break;//跳出填充缓存循环
					}
					memcpy(audio_buf + audio_buf_size, c + 4, nBuffSize);
					audio_buf_size += nBuffSize;
					delete[] c;
				}

				/*QMutexLocker locker(&m_pMutexOpQue);
				unsigned char* c = m_pDecodecBufferQue.dequeue();
				int nBuffSize = *((int*)c);
				memcpy(audio_buf, c + 4, nBuffSize);
				audio_buf_size = nBuffSize;
				delete[] c;*/
			}
			else
			{
				audio_buf_size = 1024;
				memset(audio_buf, 0, audio_buf_size);
			}
			audio_buf_index = 0;
		}
		/*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
		len1 = audio_buf_size - audio_buf_index;//缓存剩余可取数据大小
		if (len1 > len) {
			len1 = len;
		}
		SDL_MixAudio(stream, (uint8_t *)audio_buf + audio_buf_index, len1, SDL_MIX_MAXVOLUME);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
	/*     if (m_pDecodecBufferQue.count()>0)
	{
	QMutexLocker locker(&m_pMutexOpQue);
	unsigned char* c = m_pDecodecBufferQue.dequeue();
	int nBuffSize = *((int*)c);
	int copyLen = len >= nBuffSize ? nBuffSize : len;
	memcpy(stream, c+4, nBuffSize);
	delete[] c;
	}*/
}

void AudioPlayer::audioCallbaclEx(void * userData, Uint8 * stream, int len)
{
	//SDL2中必须首先使用SDL_memset()将stream中的数据设置为0  

	SDL_memset(stream, 0, len);
	if (m_pAudioBufSize == 0)
	{
		return;
	}
	len = (len > m_pAudioBufSize ? m_pAudioBufSize : len);

	//SDL_MixAudio(stream, m_pAudioBufIndex, len, SDL_MIX_MAXVOLUME);
	memcpy(stream, m_pAudioBufIndex, len);
	m_pAudioBufIndex += len;
	m_pAudioBufSize -= len;
	//SDL_Delay(1000);
}

void AudioPlayer::onInitSDLAudio(int sampleRate, int channels, int samples)
{
	//  打开SDL播放设备 - 开始
	SDL_LockAudio();
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return;
	}

	SDL_AudioSpec spec; 
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = sampleRate;  // audioCodecCtx->sample_rate; 44100 ------------ 采样率
	wanted_spec.format = AUDIO_S16SYS; //格式：这告诉SDL我们将给它什么格式。 “S16SYS”中的“S”代表“有符号”，16表示每个样本长16位，“SYS”表示顺序取决于您所在的系统。这是avcodec_decode_audio2给我们音频的格式。
	wanted_spec.channels = channels; // audioCodecCtx->channels; 2----频道：音频频道的数量。
	wanted_spec.silence = 0;
	wanted_spec.samples = samples;
	wanted_spec.callback = audioCallback; //非多线程版本
	//wanted_spec.callback = audioCallbackEx;//多线程版本
	//wanted_spec.userData =  这里是回调函数自定义参数，依据情况定义
	if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
		//if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
	{
		//fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		return;
	}

	SDL_UnlockAudio();   //开始播放
	SDL_PauseAudio(0);

	//start();
}

void AudioPlayer::onGetOneAudioBuffer(uint8_t * pAudioOutBuffer, int bufferSize, const QString strID)
{
	//前面四个字节表示长度，后面的表示数据缓存，主要是考虑到有些采样数是不一样的
	unsigned char *c = new unsigned char[4 + bufferSize];
	memcpy(c, &bufferSize, 4);

	//第5个字节开始代表换成的音频数据
	memcpy(c + 4, pAudioOutBuffer, bufferSize);
	//	printf("%s\n", c + 4);
	QMutexLocker locker(&m_pMutexOpQue);
	m_pDecodecBufferQue.enqueue(c);
}

void AudioPlayer::run()
{
	QMutexLocker locker(&m_pMutexThreadSafelyOut);
	m_pThreadSafelyOut = false;
	while (true)
	{
		QMutexLocker locker(&m_pMutexThreadRun);
		if (!m_pThreadRun)
		{
			break;
		}

		while (m_pAudioBufSize > 0 && m_pThreadRun)//Wait until finish
		{
			SDL_Delay(1);
			//continue;
		}

		/* if (m_pDecodecBufferQue.count() >0 )
		{
		QMutexLocker locker(&m_pMutexOpQue);
		unsigned char* c = m_pDecodecBufferQue.dequeue();
		int nBuffSize = *((int*)c);
		memcpy(m_sAudio_buf, c + 4, nBuffSize);
		m_pAudioBufSize = nBuffSize;
		m_pAudioBufIndex = m_pAudioBuf;
		delete[] c;
		}*/
		//用以下做测试
		if (m_pDecodecBufferQue.count() > 50)
		{
			for (int i = 0; i < 50; ++i)
			{
				QMutexLocker locker(&m_pMutexOpQue);
				unsigned char* c = m_pDecodecBufferQue.dequeue();
				int nBuffSize = *((int*)c);
				memcpy(m_pAudioBuf + m_pAudioBufSize, c + 4, nBuffSize);
				m_pAudioBufSize += nBuffSize;
				delete[] c;
			}
			m_pAudioBufIndex = m_pAudioBuf;
		}
	}

	QMutexLocker lock(&m_pMutexThreadSafelyOut);
	m_pThreadSafelyOut = true;
}
