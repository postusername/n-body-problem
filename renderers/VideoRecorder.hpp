#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
}



namespace nbody {

class VideoRecorder {
public:
    VideoRecorder(const std::string& filename, int width, int height, int fps = 60);
    ~VideoRecorder();
    
    bool initialize();
    bool write_frame(const std::vector<uint8_t>& rgba_data);
    void finalize();
    
private:
    std::string filename_;
    int width_;
    int height_;
    int fps_;
    
    AVFormatContext* format_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    AVStream* video_stream_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    SwsContext* sws_ctx_ = nullptr;
    
    int frame_count_ = 0;
    bool initialized_ = false;
};

VideoRecorder::VideoRecorder(const std::string& filename, int width, int height, int fps)
    : filename_(filename), width_(width), height_(height), fps_(fps) {
}

VideoRecorder::~VideoRecorder() {
    finalize();
}

bool VideoRecorder::initialize() {
    if (initialized_) return true;
    
    avformat_alloc_output_context2(&format_ctx_, nullptr, nullptr, filename_.c_str());
    if (!format_ctx_) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось создать контекст вывода для " << filename_ << std::endl;
        return false;
    }
    
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "VideoRecorder -- ERROR: Кодек H.264 не найден" << std::endl;
        return false;
    }
    
    video_stream_ = avformat_new_stream(format_ctx_, nullptr);
    if (!video_stream_) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось создать поток" << std::endl;
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось создать контекст кодека" << std::endl;
        return false;
    }
    
    // Настройка параметров кодека
    codec_ctx_->codec_id = AV_CODEC_ID_H264;
    codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx_->width = width_;
    codec_ctx_->height = height_;
    codec_ctx_->time_base = {1, fps_};
    codec_ctx_->framerate = {fps_, 1};
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_->bit_rate = width_ * height_ * 3;
    
    // Наилучшее качество
    codec_ctx_->qmin = 10;
    codec_ctx_->qmax = 18;
    codec_ctx_->max_qdiff = 4;
    
    // Настройки для H.264
    av_opt_set(codec_ctx_->priv_data, "preset", "slow", 0);
    av_opt_set(codec_ctx_->priv_data, "crf", "18", 0);
    
    if (format_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось открыть кодек" << std::endl;
        return false;
    }

    if (avcodec_parameters_from_context(video_stream_->codecpar, codec_ctx_) < 0) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось скопировать параметры кодека" << std::endl;
        return false;
    }
    
    video_stream_->time_base = codec_ctx_->time_base;

    frame_ = av_frame_alloc();
    if (!frame_) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось создать кадр" << std::endl;
        return false;
    }
    
    frame_->format = codec_ctx_->pix_fmt;
    frame_->width = codec_ctx_->width;
    frame_->height = codec_ctx_->height;
    
    if (av_frame_get_buffer(frame_, 32) < 0) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось выделить буфер для кадра" << std::endl;
        return false;
    }

    packet_ = av_packet_alloc();
    if (!packet_) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось создать пакет" << std::endl;
        return false;
    }

    sws_ctx_ = sws_getContext(width_, height_, AV_PIX_FMT_RGBA,
                              width_, height_, AV_PIX_FMT_YUV420P,
                              SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!sws_ctx_) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось создать контекст масштабирования" << std::endl;
        return false;
    }

    if (!(format_ctx_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_ctx_->pb, filename_.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "VideoRecorder -- ERROR: Не удалось открыть файл " << filename_ << std::endl;
            return false;
        }
    }

    if (avformat_write_header(format_ctx_, nullptr) < 0) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось записать заголовок" << std::endl;
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool VideoRecorder::write_frame(const std::vector<uint8_t>& rgba_data) {
    if (!initialized_) {
        std::cerr << "VideoRecorder -- ERROR: VideoRecorder не инициализирован" << std::endl;
        return false;
    }
    
    if (rgba_data.size() != static_cast<size_t>(width_ * height_ * 4)) {
        std::cerr << "VideoRecorder -- ERROR: Неверный размер данных кадра" << std::endl;
        return false;
    }

    if (av_frame_make_writable(frame_) < 0) {
        std::cerr << "VideoRecorder -- ERROR: Не удалось сделать кадр записываемым" << std::endl;
        return false;
    }
    

    const uint8_t* src_data[4] = { rgba_data.data(), nullptr, nullptr, nullptr };
    int src_linesize[4] = { width_ * 4, 0, 0, 0 };
    
    sws_scale(sws_ctx_, src_data, src_linesize, 0, height_,
              frame_->data, frame_->linesize);
    
    frame_->pts = frame_count_++;

    int ret = avcodec_send_frame(codec_ctx_, frame_);
    if (ret < 0) {
        std::cerr << "VideoRecorder -- ERROR: Ошибка при отправке кадра кодеку" << std::endl;
        return false;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "VideoRecorder -- ERROR: Ошибка при получении пакета от кодека" << std::endl;
            return false;
        }
        
        av_packet_rescale_ts(packet_, codec_ctx_->time_base, video_stream_->time_base);
        packet_->stream_index = video_stream_->index;
        
        ret = av_interleaved_write_frame(format_ctx_, packet_);
        av_packet_unref(packet_);
        
        if (ret < 0) {
            std::cerr << "VideoRecorder -- ERROR: Ошибка при записи кадра" << std::endl;
            return false;
        }
    }
    
    return true;
}

void VideoRecorder::finalize() {
    if (!initialized_) return;
    
    if (codec_ctx_) {
        avcodec_send_frame(codec_ctx_, nullptr);
        
        int ret;
        while ((ret = avcodec_receive_packet(codec_ctx_, packet_)) >= 0) {
            av_packet_rescale_ts(packet_, codec_ctx_->time_base, video_stream_->time_base);
            packet_->stream_index = video_stream_->index;
            av_interleaved_write_frame(format_ctx_, packet_);
            av_packet_unref(packet_);
        }
    }

    if (format_ctx_) {
        av_write_trailer(format_ctx_);
    }
    
    // Очистка ресурсов
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
    
    if (frame_) {
        av_frame_free(&frame_);
    }
    
    if (packet_) {
        av_packet_free(&packet_);
    }
    
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    
    if (format_ctx_) {
        if (!(format_ctx_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx_->pb);
        }
        avformat_free_context(format_ctx_);
        format_ctx_ = nullptr;
    }
    
    initialized_ = false;
}

} // namespace nbody 