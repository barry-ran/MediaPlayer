#include "DecodecThread.h"
#include <QDebug>

#define MAX_AUDIO_FRAME_SIZE 192000

DecodecThread::DecodecThread(QObject *parent)
	: QThread(parent)
{
	m_pErrInfo = "";
	m_pMediaSourcePath = "";
	m_pFormatCtx = Q_NULLPTR;

	m_nVideoStream = -1;
	m_nAudioStream = -1;

	m_pVideoCodecCtx = Q_NULLPTR;
	m_pAudioCodecCtx = Q_NULLPTR;

	m_pVideoCodec = Q_NULLPTR;
	m_pAudioCodec = Q_NULLPTR;

	m_pOriginalVideoFrame = Q_NULLPTR;
	m_pOriginalAudioFrame = Q_NULLPTR;
	m_pOutputVideoFrame = Q_NULLPTR;
	m_pOutputAudioFrame = Q_NULLPTR;

	m_pVideoOutBuffer = Q_NULLPTR;
	m_pAudioOutBuffer = Q_NULLPTR;

	m_pImgConvertCtx = Q_NULLPTR;
	m_pAudioConvertCtx = Q_NULLPTR;

	m_pOutputPixelFormat = AV_PIX_FMT_RGB24;
	m_pPacket = Q_NULLPTR;
	m_pOutSampleFmt = AV_SAMPLE_FMT_S16;
	m_pOutChLayout = AV_CH_LAYOUT_STEREO;

	m_nInitRet = -1;  //0表示成功
	m_nAudioInitRet = -1;
	m_nVideoInitRet = -1;

	m_pLastDecodecWorkFinished = true;
	InitDecodec();
}

DecodecThread::~DecodecThread()
{
	//通知结束线程
	QMutexLocker locker(&m_pMutexStopCircleRunByInitRet);
	m_nInitRet = -1;

	//等待线程结束
	while (true)
	{
		QMutexLocker locker(&m_pMutexForDecodecFinished);
		if (m_pLastDecodecWorkFinished)
		{
			break;
		}
		msleep(100);
	}

	//释放资源
	ReleaseDecodec();
}

//更新视频源
void DecodecThread::UpdateFormatContext(const char * mediaSource)
{
	//通知解码线程停止
	QMutexLocker locker(&m_pMutexStopCircleRunByInitRet);
	m_nInitRet = -1;

	QString tempStrSource = mediaSource;
	if (tempStrSource.isEmpty())
	{
		m_nInitRet = 1;
		m_pErrInfo = "media source is empty";
		return;
	}
	m_pMediaSourcePath = mediaSource;

	//等待解码线程停止
	while (true)
	{
		QMutexLocker locker(&m_pMutexForDecodecFinished);
		if (m_pLastDecodecWorkFinished)
		{
			break;
		}
		msleep(100);
	}

	//释放当前资源
	ReleaseDecodec();

	//重新开始
	start();
}

void DecodecThread::ChangeMediaSourec(const char * mediaSource)
{
	m_pMediaSourcePath = mediaSource;
}

void DecodecThread::ChangeOutputPixelFormat(const AVPixelFormat pixelFormat)
{
	m_pOutputPixelFormat = pixelFormat;
}

void DecodecThread::ChangeOutChLayout(const uint64_t outChLayout)
{
	m_pOutChLayout = outChLayout;
}

void DecodecThread::ChangeAVSampleFormat(const AVSampleFormat outSampleFmt)
{
	m_pOutSampleFmt = outSampleFmt;
}

int DecodecThread::GetAudioSampleRate()
{
	return m_nOutSampleRate;
}

void DecodecThread::InitDecodec()
{
	//初始化FFMPEG
	av_register_all();
	//支持网络流
	avformat_network_init();
	//分配一个AVFormatContext，FFMPEG[所有的操作]都要通过这个AVFormatContext来进行
	m_pFormatCtx = avformat_alloc_context();

}

int DecodecThread::InitDecodecForVideo()
{
	if (m_nVideoStream == -1)
	{
		m_pErrInfo.append("unknown videoStream failed");
		return -1;
	}

	m_pVideoCodecCtx = m_pFormatCtx->streams[m_nVideoStream]->codec;  //解码器上下文环境
	m_pVideoCodecCtx->time_base.num = 1;
	m_pVideoCodecCtx->time_base.den = 30;
	m_pVideoCodecCtx->bit_rate = 3000;
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
	if (m_pVideoCodec == Q_NULLPTR)
	{
		m_pErrInfo.append(", m_pVideoCodec is null");
		return -1;
	}

	//打开视频解码器
	if (avcodec_open2(m_pVideoCodecCtx, m_pVideoCodec, NULL) < 0)
	{
		m_pErrInfo.append(", open video avcodec failed");
		return -1;
	}
	m_pOriginalVideoFrame = av_frame_alloc();
	m_pOutputVideoFrame = av_frame_alloc();

	//===========================video转码准备============================//
	//设置转码器
	m_pImgConvertCtx = sws_getContext(m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, m_pVideoCodecCtx->pix_fmt, 
		m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, m_pOutputPixelFormat, SWS_BICUBIC, NULL, NULL, NULL);
	
	//计算视频输出缓存
	int numBytes = avpicture_get_size(m_pOutputPixelFormat, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);
	//输出缓存
	m_pVideoOutBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	//avpicture_fill函数将ptr指向的数据填充到picture内，但并没有拷贝，只是将picture结构内的data指针指向了ptr的数据。
	avpicture_fill((AVPicture *)m_pOutputVideoFrame, m_pVideoOutBuffer, m_pOutputPixelFormat, m_pVideoCodecCtx->width, 
		m_pVideoCodecCtx->height);
	av_dump_format(m_pFormatCtx, 0, m_pMediaSourcePath.toLatin1().data(), 0);  //输出视频信息

	qDebug() << QStringLiteral("视频转码准备 OK");
	return 0;
}

int DecodecThread::InitDecodecForAudio()
{
	if (m_nAudioStream == -1)
	{
		m_pErrInfo.append("unknow audioStream failed");
		return -1;
	}

	//根据音频类型查找解码器
	m_pAudioCodecCtx = m_pFormatCtx->streams[m_nAudioStream]->codec;
	m_pAudioCodec = avcodec_find_decoder(m_pAudioCodecCtx->codec_id);
	if (m_pAudioCodec == Q_NULLPTR)
	{
		m_pErrInfo.append(", m_pAudioCodec is null");
		return -1;
	}

	//打开音频解码器
	if (avcodec_open2(m_pAudioCodecCtx, m_pAudioCodec, NULL) < 0)
	{
		m_pErrInfo.append(", open audio avcodec failed");
		return -1;
	}
	m_pOriginalAudioFrame = av_frame_alloc();
	m_pOutputAudioFrame = av_frame_alloc();

	//===============================音频转码准备=============================//
	enum AVSampleFormat inSampleFmt = m_pAudioCodecCtx->sample_fmt;    //输入的采样格式
	m_nOutSampleRate = m_pAudioCodecCtx->sample_rate;//输入的采样率    音频采样率是指录音设备在一秒钟内对声音信号的采样次数，采样频率越高声音的还原就越真实越自然。
	uint64_t inChLayout = m_pAudioCodecCtx->channel_layout;//输入的声道布局  声道是指声音在录制或播放时在不同空间位置采集或回放的相互独立的音频信号，所以声道数也就是声音录制时的音源数量或回放时相应的扬声器数量。
	
	m_pAudioConvertCtx = swr_alloc();  //获得音频转码器
	swr_alloc_set_opts(m_pAudioConvertCtx, m_pOutChLayout, m_pOutSampleFmt, m_nOutSampleRate, 
		inChLayout, inSampleFmt, m_nOutSampleRate, 0, NULL);
	swr_init(m_pAudioConvertCtx);//初始化音频转码器

	m_nChannels = av_get_channel_layout_nb_channels(m_pOutChLayout);  //获取声道个数
	m_nOutSamples = m_pAudioCodecCtx->frame_size;//如果为m_out_nb_samples==0，则表示音频编码器支持在每个呼叫中​​接收不同数量的采样。
	if (m_nOutSamples)
	{
		int samplesSize = av_samples_get_buffer_size(NULL, m_nChannels, m_nOutSamples, m_pOutSampleFmt, 1);
		m_pAudioOutBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
		emit onSigOutAudioParams(m_nOutSampleRate, m_nChannels, m_nOutSamples);

		qDebug() << QStringLiteral("音频转码准备 OK");
	}
	else
	{
		m_pAudioOutBuffer = (uint8_t *)av_malloc(1024);  //存储pcm数据
		emit onSigOutAudioParams(m_nOutSampleRate, m_nChannels, 1024);

		qDebug() << QStringLiteral("音频转码准备 OK1024");
	}

	return 0;
}

void DecodecThread::ReleaseDecodec()
{
	if (m_pImgConvertCtx)
	{
		sws_freeContext(m_pImgConvertCtx);
		m_pImgConvertCtx = Q_NULLPTR;
	}

	if (m_pAudioConvertCtx)
	{
		swr_free(&m_pAudioConvertCtx);
	}

	if (m_pVideoOutBuffer)
	{
		av_free(m_pVideoOutBuffer);
		m_pVideoOutBuffer = Q_NULLPTR;
	}

	if (m_pAudioOutBuffer)
	{
		av_free(m_pAudioOutBuffer);
		m_pAudioOutBuffer = Q_NULLPTR;
	}

	if (m_pOutputVideoFrame)
	{
		av_free(m_pOutputVideoFrame);
		m_pOutputVideoFrame = Q_NULLPTR;
	}

	if (m_pOutputAudioFrame)
	{
		av_free(m_pOutputAudioFrame);
		m_pOutputAudioFrame = Q_NULLPTR;
	}

	if (m_pVideoCodecCtx)
	{
		avcodec_close(m_pVideoCodecCtx);
		m_pVideoCodecCtx = Q_NULLPTR;
	}

	if (m_pAudioCodecCtx)
	{
		avcodec_close(m_pAudioCodecCtx);
		m_pAudioCodecCtx = Q_NULLPTR;
	}

	if (m_pFormatCtx)
	{
		avformat_close_input(&m_pFormatCtx);
	}
}

void DecodecThread::run()
{
	QByteArray byteArray = m_pMediaSourcePath.toLatin1();
	char *charSource = byteArray.data();
	if (!m_pFormatCtx)
	{
		m_pFormatCtx = avformat_alloc_context();
	}

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "stimeout", "10000000", 0);  //10秒后没拉取到流就代表超时

	//avformat_open_input默认是阻塞的，用户可以通过设置“ic->flags |= AVFMT_FLAG_NONBLOCK; ”设置成非阻塞（通常是不推荐的）；或者是设置timeout设置超时时间；或者是设置interrupt_callback定义返回机制。
	//打开视频文件，信息保存在m_pFormatCtx中
	if (avformat_open_input(&m_pFormatCtx, charSource, NULL, &opts) != 0)
	{
		m_nInitRet = 1;
		m_pErrInfo = " open media source failed";
		return;
	}

	//根据打开的文件寻找其流信息
	if (avformat_find_stream_info(m_pFormatCtx, NULL) < 0)
	{
		m_nInitRet = 2;
		m_pErrInfo = "find stream info failed";
		return;
	}

	//循环查找视频中包含的流信息，知道找到视频类型的流
	//便将其记录下来，保存到videoStream变量中
	m_nVideoStream = -1;
	m_nAudioStream = -1;
	for (int i = 0; i < m_pFormatCtx->nb_streams; ++i)
	{
		if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_nVideoStream = i;
		}
		else if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			m_nAudioStream = i;
		}
	}

	m_nVideoInitRet = InitDecodecForVideo();
	m_nAudioInitRet = InitDecodecForAudio();
	if (m_nVideoInitRet == -1 && m_nAudioInitRet == -1)
	{
		m_nInitRet = -1;
		return;
	}

	m_pPacket = av_packet_alloc();
	m_nInitRet = 0;

	QMutexLocker locker(&m_pMutexForDecodecFinished);
	m_pLastDecodecWorkFinished = false;
	while (true)
	{
		msleep(15);

		QMutexLocker locker(&m_pMutexStopCircleRunByInitRet);
		if (m_nInitRet == -1)
		{
			break;
		}

		int ret = -1;
		if (av_read_frame(m_pFormatCtx, m_pPacket) < 0)
		{
			continue;
		}

		int gotPicture = 0;
		int gotFrame = 0;
		if (m_pPacket->stream_index == m_nVideoStream)
		{
			//解码
			ret = avcodec_decode_video2(m_pVideoCodecCtx, m_pOriginalVideoFrame, &gotPicture, m_pPacket);
			if (ret < 0)
			{
				return;
			}
			//基本上所有解码器解码之后得到的图像数据都是YUV420的格式，而这里我们需要将其保存成图片文件，因此需要将得到的YUV420数据转换成RGB格式，转换格式也是直接使用FFMPEG来完成：
			if (gotPicture)
			{
				sws_scale(m_pImgConvertCtx, //图片转码上下文
				   (uint8_t const * const *)m_pOriginalVideoFrame->data, //原始数据
					m_pOriginalVideoFrame->linesize, //原始参数
					0, //转码开始游标，一般为0
					m_pVideoCodecCtx->height, //行数
					m_pOutputVideoFrame->data, //转码后的数据
					m_pOutputVideoFrame->linesize);
				QImage image(m_pOutputVideoFrame->data[0], m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, QImage::Format_RGB888);
				QImage copyImage = image.copy();
				emit onSigDecodecOnePicture(copyImage, m_pMediaSourcePath);
			}
		}
		else if (m_pPacket->stream_index == m_nAudioStream)
		{
			if (avcodec_decode_audio4(m_pAudioCodecCtx, m_pOutputAudioFrame, &gotFrame, m_pPacket) >= 0)
			{
				if (gotFrame)
				{
					int autoSize = swr_get_out_samples(m_pAudioConvertCtx, m_pOutputAudioFrame->nb_samples);
					//音频格式转换
					swr_convert(m_pAudioConvertCtx, //音频转码上下文
						&m_pAudioOutBuffer, //输出缓存
						(m_nOutSamples ? m_nOutSamples : 1024), //
						(const uint8_t **)m_pOutputAudioFrame->data, //原始数据
						m_pOutputAudioFrame->nb_samples);  //输入
					if (m_nOutSamples)
					{
						m_nAudioOutBufferSize = av_samples_get_buffer_size(NULL, m_nChannels, m_nOutSamples, m_pOutSampleFmt, 1);
						emit onSigDecodecOneAudio(m_pAudioOutBuffer, m_nAudioOutBufferSize, m_pMediaSourcePath);
					}
					else
					{
						emit onSigDecodecOneAudio(m_pAudioOutBuffer, 1024, m_pMediaSourcePath);
					}
				}
			}
			else
			{
				memset(m_pAudioOutBuffer, 0, 1024);
				emit onSigDecodecOneAudio(m_pAudioOutBuffer, 1024, m_pMediaSourcePath);
			}
		}
		av_free_packet(m_pPacket);
	}
	
	QMutexLocker lock(&m_pMutexForDecodecFinished);
	m_pLastDecodecWorkFinished = true;
}
