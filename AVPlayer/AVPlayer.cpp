#include <QAudioFormat>  
#include <QDebug>
#include "AVPlayer.h"
#pragma comment(lib, "AVEditor.lib")

using namespace avstudio;
constexpr auto knMaxBufferSize = 10240 * 2;
constexpr auto kSleepDelay = 20;

AVPlayer::AVPlayer(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

	connect(ui.btnPlay, &QPushButton::clicked,
		this, &AVPlayer::OnPlayClicked);

	connect(ui.btnStop, &QPushButton::clicked,
		this, &AVPlayer::OnStopClicked);

	connect(this, &AVPlayer::OnVideoArrived,
		this, &AVPlayer::slotVideoArrived);

	SetMediaFile("1.mp4");
	SetPlayView(ui.label);

	m_Editor.SetFinishedCallback(
		[this]() {

			if (m_View) m_View->clear();
		});

}

AVPlayer::~AVPlayer()
{
	JoinThread();

	m_Editor.Stop();
	m_Editor.Join();
}

void AVPlayer::SetMediaFile(const std::string& n_sMediaFile)
{
	m_sMediaFile = n_sMediaFile;
}

void AVPlayer::SetPlayView(QLabel* n_View)
{
	m_View = n_View;
}

void AVPlayer::Load()
{
	auto Input = m_Editor.OpenInputFile(m_sMediaFile);
	auto Output = m_Editor.AllocOutputFile("");

	auto vCodec = Input->VideoParts.Codec;

	Output->IOHandle = this;
	m_nSelectedStreams = 0;
	m_nStreamMark = 0;

	if (vCodec && vCodec->Context)
	{
		Output->EnableStream(AVMediaType::AVMEDIA_TYPE_VIDEO);
		auto vctx = Output->BuildCodecContext(
			AVCodecID::AV_CODEC_ID_RAWVIDEO, vCodec->Context);

		vctx->Context->pix_fmt = GetSupportedPixelFormat(
			vctx->Context->codec,
			AVPixelFormat::AV_PIX_FMT_RGB24);

		m_nSelectedStreams |= 1;
	}

	AVCodecContext* aoCodec = nullptr;
	if (Input->AudioParts.Stream)
	{
		Output->EnableStream(AVMediaType::AVMEDIA_TYPE_AUDIO);
		auto actx = Output->BuildCodecContext(Input->AudioParts.Stream);

		if (actx->Context)
			actx->Context->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;

		aoCodec = actx->Context;
		m_nSelectedStreams |= 2;
	}

	if (aoCodec)
	{
		QAudioFormat format;
		format.setSampleRate(aoCodec->sample_rate);						// 例如：44.1 kHz  
		format.setChannelCount(aoCodec->ch_layout.nb_channels);			// 例如：立体声  
		int nFmt = GetBytesPerSample(aoCodec->sample_fmt);
		format.setSampleSize(nFmt * 8);									// 例如：16位样本  
		format.setCodec("audio/pcm");									// 例如：PCM编码  
		format.setByteOrder(QAudioFormat::LittleEndian);				// 字节序  
		format.setSampleType(QAudioFormat::SignedInt);					// 样本类型
		m_nBytesPerSample = nFmt * aoCodec->ch_layout.nb_channels;
		m_dDurionPerSample = 1.0 / aoCodec->sample_rate;

		if (!m_AudioOutput || m_AudioOutput->format() != format)
		{
			QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

			if (m_AudioOutput) m_AudioOutput->deleteLater(); 
			m_AudioOutput = new QAudioOutput(info, format, this);
			m_AudioOutput->setBufferSize(knMaxBufferSize);
		}

		m_Device = m_AudioOutput->start();
	}

	// for section select
	//Input->PickupFragment(5, 10);
	m_Editor.SetMaxBufferSize(30);

	m_nFreeBytes = knMaxBufferSize;
	m_dTime = 0;

	JoinThread();
	if (m_nSelectedStreams & 1)
		tVideo = std::thread(std::bind(&AVPlayer::PlayVideo, this));
	if (m_nSelectedStreams & 2)
		tAudio = std::thread(std::bind(&AVPlayer::PlayAudio, this));
}

void AVPlayer::VideoFrameArrived(const AVFrame* n_Frame)
{
	if (!n_Frame) return;

	double dTimestamp = n_Frame->pts * av_q2d(n_Frame->time_base);
	int offset = (dTimestamp - m_dTime) * 1000 * 1000;

	if ((m_nSelectedStreams & (1 << AVMediaType::AVMEDIA_TYPE_AUDIO) == 0)
	{
		// The audio stream is not exists
		m_dTime = dTimestamp;
	}

	if (offset > 0)
		std::this_thread::sleep_for(std::chrono::microseconds(offset));

	QImage img(n_Frame->data[0],
		n_Frame->width, n_Frame->height, n_Frame->linesize[0], QImage::Format_RGB888);

	img = img.scaled(m_View->size());
	emit OnVideoArrived(QPixmap::fromImage(img));
}

void AVPlayer::AudioFrameArrived(const AVFrame* n_Frame)
{
	if (!m_AudioOutput) return;

	QAudio::State st = m_AudioOutput->state();
	if (m_Device &&
		(st == QAudio::State::IdleState || st == QAudio::State::ActiveState))
	{
		int nFree = m_AudioOutput->bytesFree();

		if (n_Frame)
		{
			while (nFree < n_Frame->nb_samples * m_nBytesPerSample)
			{
				nFree = m_AudioOutput->bytesFree();
				std::this_thread::sleep_for(std::chrono::milliseconds(kSleepDelay));
			}

			UpdateTime(nFree);

			m_Device->write(reinterpret_cast<const char*>(n_Frame->data[0]), n_Frame->linesize[0]);
			m_nFreeBytes = nFree - n_Frame->nb_samples * m_nBytesPerSample;
		}
		else
		{
			int nSamples = (knMaxBufferSize - m_nFreeBytes) / m_nBytesPerSample;
			double dTime = nSamples * m_dDurionPerSample;
			
			std::this_thread::sleep_for(std::chrono::milliseconds(int(dTime * 1000)));
			m_dTime += dTime;

			m_AudioOutput->stop();
		}
	}
}

void AVPlayer::UpdateTime(int n_nFree)
{
	int nSamplesCost = (n_nFree - m_nFreeBytes) / m_nBytesPerSample;
	m_dTime += nSamplesCost * m_dDurionPerSample;
}

void AVPlayer::PlayVideo()
{
	// wait for data coming
	while (IsEnd(AVMediaType::AVMEDIA_TYPE_VIDEO) == AVERROR_EOF)
		std::this_thread::sleep_for(std::chrono::milliseconds(kSleepDelay));

	m_nStreamMark |= 1;

	// video and audio sync
	while (m_nStreamMark != m_nSelectedStreams)
		std::this_thread::sleep_for(std::chrono::milliseconds(kSleepDelay));

	while (true)
	{
		auto item = PopData(AVMediaType::AVMEDIA_TYPE_VIDEO);
		if (!item) break;

		VideoFrameArrived(item->f());
		AVFreeDataPtr(&item);
	}
}

void AVPlayer::PlayAudio()
{
	// wait for data coming
	while (IsEnd(AVMediaType::AVMEDIA_TYPE_AUDIO) == AVERROR_EOF)
		std::this_thread::sleep_for(std::chrono::milliseconds(kSleepDelay));

	m_nStreamMark |= 1;

	// video and audio sync
	while (m_nStreamMark != m_nSelectedStreams)
		std::this_thread::sleep_for(std::chrono::milliseconds(kSleepDelay));

	while (true)
	{
		auto item = PopData(AVMediaType::AVMEDIA_TYPE_AUDIO);
		if (!item) break;

		AudioFrameArrived(item->f());
		AVFreeDataPtr(&item);
	}
}

void AVPlayer::JoinThread()
{
	if (tVideo.joinable()) tVideo.join();
	if (tAudio.joinable()) tVideo.tAudio();
}

void AVPlayer::OnPlayClicked()
{
	if (m_sMediaFile.empty()) return;

	try
	{
		Load();

		m_Editor.Start();
		
		//double dLength = m_Editor.GetInputContext()->Length();
	}
	catch (const std::exception& e)
	{
		qDebug() << e.what();
	}
}

void AVPlayer::OnStopClicked()
{
	m_Editor.Stop();
}

void AVPlayer::slotVideoArrived(const QPixmap n_Pixmap)
{
	if (!n_Pixmap.isNull() && m_View)
	{
		m_View->setPixmap(n_Pixmap);
	}
}
