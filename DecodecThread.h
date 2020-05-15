#pragma once

#include <QThread>
#include <QMutex>
#include <QImage>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}

class DecodecThread : public QThread
{
	Q_OBJECT

public:
	DecodecThread(QObject *parent);
	~DecodecThread();

public:
	void UpdateFormatContext(const char *mediaSource); //��������ƵԴ
	void ChangeMediaSourec(const char *mediaSource);
	void ChangeOutputPixelFormat(const AVPixelFormat pixelFormat);
	void ChangeOutChLayout(const uint64_t outChLayout);
	void ChangeAVSampleFormat(const AVSampleFormat outSampleFmt);

	//��ȡ������
	int GetAudioSampleRate();

signals:
	void onSigDecodecOnePicture(QImage srcImage, const QString strID);
	void onSigDecodecOneAudio(uint8_t *pAudioOutBuffer, int audioOutBufferSize, const QString strID);
	void onSigOutAudioParams(int sampleRate, int channels, int samples);

private:
	void InitDecodec();  //��ʼ��������
	int InitDecodecForVideo();
	int InitDecodecForAudio();
	void ReleaseDecodec();  //�ͷŽ�����

private:
	QString m_pMediaSourcePath;
	QString m_pErrInfo;

	AVFormatContext *m_pFormatCtx;   //��ý������
	AVPacket        *m_pPacket;      //����ǰ�����ݰ�


	/*�������У�����Ƶ�����ڵ�λ��*/
	int m_nVideoStream;
	int m_nAudioStream;

	/*����Ƶ�����������Ļ���*/
	AVCodecContext *m_pVideoCodecCtx;
	AVCodecContext *m_pAudioCodecCtx; 

	/*����Ƶ������*/
	AVCodec *m_pVideoCodec;
	AVCodec *m_pAudioCodec;

	/*����Ƶ�������*/
	uint8_t *m_pVideoOutBuffer;
	uint8_t *m_pAudioOutBuffer;

	/*����Ƶ��������С*/
	int m_nVideoOutBufferSize;
	int m_nAudioOutBufferSize;

	AVFrame *m_pOriginalVideoFrame;   //�洢�������ԭʼͼ��֡
	AVFrame *m_pOutputVideoFrame;
	AVFrame *m_pOriginalAudioFrame;
	AVFrame *m_pOutputAudioFrame;

	struct SwsContext *m_pImgConvertCtx;   //ͼ��ת����
	struct SwrContext *m_pAudioConvertCtx;  //��Ƶת����

private:
	/*֡��FPS*/
	int m_nVideoFPS;

	/*��������ظ�ʽ*/
	enum AVPixelFormat m_pOutputPixelFormat;

	/*��Ƶ����*/
	int m_nChannels;  //��Ƶ��������
	enum AVSampleFormat m_pOutSampleFmt;  //����Ĳ�����ʽ 16bit PCM
	int m_nOutSampleRate;   //���������
	uint64_t m_pOutChLayout;  //������������֣�������
	int m_nOutSamples;        //��Ƶ���ͨ��������

	int m_nInitRet;           //��ʼ�����
	QMutex m_pMutexStopCircleRunByInitRet;

	//���ڱ������Ƶת��׼�����
	int m_nAudioInitRet;
	int m_nVideoInitRet;

	bool m_pLastDecodecWorkFinished;  //�˳�ʱ�������̱߳������˳�
	QMutex m_pMutexForDecodecFinished;

private:
	virtual void run();
};
