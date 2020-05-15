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
	void UpdateFormatContext(const char *mediaSource); //更新音视频源
	void ChangeMediaSourec(const char *mediaSource);
	void ChangeOutputPixelFormat(const AVPixelFormat pixelFormat);
	void ChangeOutChLayout(const uint64_t outChLayout);
	void ChangeAVSampleFormat(const AVSampleFormat outSampleFmt);

	//获取采样率
	int GetAudioSampleRate();

signals:
	void onSigDecodecOnePicture(QImage srcImage, const QString strID);
	void onSigDecodecOneAudio(uint8_t *pAudioOutBuffer, int audioOutBufferSize, const QString strID);
	void onSigOutAudioParams(int sampleRate, int channels, int samples);

private:
	void InitDecodec();  //初始化解码器
	int InitDecodecForVideo();
	int InitDecodecForAudio();
	void ReleaseDecodec();  //释放解码器

private:
	QString m_pMediaSourcePath;
	QString m_pErrInfo;

	AVFormatContext *m_pFormatCtx;   //流媒体数据
	AVPacket        *m_pPacket;      //解码前的数据包


	/*流队列中，音视频流所在的位置*/
	int m_nVideoStream;
	int m_nAudioStream;

	/*音视频解码器上下文环境*/
	AVCodecContext *m_pVideoCodecCtx;
	AVCodecContext *m_pAudioCodecCtx; 

	/*音视频解码器*/
	AVCodec *m_pVideoCodec;
	AVCodec *m_pAudioCodec;

	/*音视频输出缓存*/
	uint8_t *m_pVideoOutBuffer;
	uint8_t *m_pAudioOutBuffer;

	/*音视频输出缓存大小*/
	int m_nVideoOutBufferSize;
	int m_nAudioOutBufferSize;

	AVFrame *m_pOriginalVideoFrame;   //存储解码出的原始图像帧
	AVFrame *m_pOutputVideoFrame;
	AVFrame *m_pOriginalAudioFrame;
	AVFrame *m_pOutputAudioFrame;

	struct SwsContext *m_pImgConvertCtx;   //图像转码器
	struct SwrContext *m_pAudioConvertCtx;  //音频转码器

private:
	/*帧率FPS*/
	int m_nVideoFPS;

	/*输出的像素格式*/
	enum AVPixelFormat m_pOutputPixelFormat;

	/*音频参数*/
	int m_nChannels;  //音频声道数量
	enum AVSampleFormat m_pOutSampleFmt;  //输出的采样格式 16bit PCM
	int m_nOutSampleRate;   //输出采样率
	uint64_t m_pOutChLayout;  //输出的声道布局：立体声
	int m_nOutSamples;        //音频输出通道采样数

	int m_nInitRet;           //初始化情况
	QMutex m_pMutexStopCircleRunByInitRet;

	//用于标记音视频转码准备结果
	int m_nAudioInitRet;
	int m_nVideoInitRet;

	bool m_pLastDecodecWorkFinished;  //退出时，解码线程必须先退出
	QMutex m_pMutexForDecodecFinished;

private:
	virtual void run();
};
