#include <rknn_api.h>
#include "MatchaRknnEngine.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
static constexpr int MEL_BINS = 80;
static constexpr int TOKEN_WIDTH = 80;
static constexpr int INTERNAL_FRAMES = 768;
// 39 Chinese characters produce at most 79 frontend tokens (BOS plus one
// pronunciation token/separator pair per character), fitting TOKEN_WIDTH=80.
// The actual 600-frame limit is enforced after Encoder inference and retries
// are split automatically.
static constexpr int MAX_SPEAKABLE_CHARS_PER_CHUNK = 39;


static constexpr int N_ODE_STEPS = 3;
static constexpr int N_TIME_BLOCKS = 6;
static constexpr int TIME_EMB_DIM = 256;
static constexpr int ESTIMATOR_BUCKETS[] =
{
    128, 256, 384, 512, 768
};

static constexpr float ODE_DT = 1.0f / 3.0f;

static constexpr float MEL_SIGMA = 2.7628188f;
static constexpr float MEL_MEAN  = -5.9870973f;

// Previously validated Vocos 22.05 kHz fixed-shape RKNN
static constexpr int VOCOS_FRAMES = 600;
static constexpr int VOCOS_FREQ_BINS = 513;

static constexpr int N_FFT = 1024;
static constexpr int HOP_LENGTH = 256;

static constexpr int VOCOS_SAMPLE_RATE = 22050;
static constexpr int OUTPUT_SAMPLE_RATE = 16000;

// Keep sentence joins compact while retaining enough context around speech.
static constexpr int SILENCE_WINDOW_MS = 10;
static constexpr float SILENCE_RMS_RATIO = 0.012f;
static constexpr int NATURAL_SENTENCE_GAP_MS = 60;
static constexpr int SHORT_PUNCTUATION_GAP_MS = 25;
static constexpr int SENTENCE_LEADING_PADDING_MS = 10;
static constexpr int SENTENCE_TRAILING_PADDING_MS = 25;

// Remove only abnormally long silence generated inside one punctuation-free
// model chunk. Measured long/short "测试文字" WAVs showed a 130 ms quiet run
// only in the long-context result; normal short-context gaps stayed below
// 100 ms at this relative threshold.
static constexpr float INTERNAL_SILENCE_RMS_RATIO = 0.018f;
static constexpr int INTERNAL_SILENCE_MIN_MS = 100;
static constexpr int INTERNAL_SILENCE_KEEP_MS = 35;
static constexpr int INTERNAL_SILENCE_CROSSFADE_MS = 5;

// Keep the model's original speaking rate.  Frame-limit chunk joins are
// handled separately and must not be mistaken for sentence boundaries.
static constexpr float TTS_LENGTH_SCALE = 1.0f;

static constexpr float PI_F = 3.14159265358979323846f;


// ============================================================
// Load binary model
// ============================================================

bool load_lexicon(
    const std::string& path,
    std::unordered_map<std::string,std::string>& lexicon)
{
    std::ifstream f(path);

    if(!f.is_open())
    {
        std::cerr<<"cannot open lexicon\n";
        return false;
    }

    std::string word;
    std::string py;

    while(f >> word >> py)
    {
        lexicon[word]=py;
    }

    std::cout
        <<"lexicon size="
        <<lexicon.size()
        <<std::endl;

    return true;
}

bool load_tokens(
    const std::string& path,
    std::unordered_map<std::string,int>& table)
{
    std::ifstream f(path);

    if(!f.is_open())
    {
        std::cerr<<"cannot open tokens\n";
        return false;
    }


    std::string line;

    while(std::getline(f,line))
    {
        if(line.empty())
            continue;


        std::stringstream ss(line);


        std::string tok;
        int id;


        ss >> tok >> id;


        if(ss.fail())
            continue;


        table[tok]=id;
    }



    std::cout
        <<"token size="
        <<table.size()
        <<std::endl;


    return true;
}

static bool is_ignored_text_symbol(
    const std::string& ch)
{
    // Ignore ASCII whitespace/punctuation and common CJK punctuation.  The
    // splitter keeps sentence terminators (for example "。"), while the
    // pronunciation lexicon normally contains spoken characters only.
    if (ch.size() == 1)
    {
        const unsigned char c =
            static_cast<unsigned char>(ch[0]);

        if (std::isspace(c) ||
            std::ispunct(c))
        {
            return true;
        }
    }

    static const std::vector<std::string> symbols =
    {
        "，", "。", "！", "？", "；", "：", "、",
        "（", "）", "【", "】", "《", "》", "〈", "〉",
        "“", "”", "‘", "’", "…", "—", "·", "～", "　"
    };

    return std::find(
        symbols.begin(),
        symbols.end(),
        ch) != symbols.end();
}

static int pause_after_text_ms(
    const std::string& text)
{
    size_t end = text.size();

    while (end > 0 &&
           std::isspace(
               static_cast<unsigned char>(
                   text[end - 1])))
    {
        --end;
    }

    const auto ends_with =
        [&](const std::string& suffix)
    {
        return end >= suffix.size() &&
               text.compare(
                   end - suffix.size(),
                   suffix.size(),
                   suffix) == 0;
    };

    static const std::vector<std::string> strong =
    {
        "。", "！", "？", ".", "!", "?", "；", ";"
    };

    for (const auto& symbol : strong)
    {
        if (ends_with(symbol))
            return NATURAL_SENTENCE_GAP_MS;
    }

    static const std::vector<std::string> weak =
    {
        "，", ",", "、", "：", ":"
    };

    for (const auto& symbol : weak)
    {
        if (ends_with(symbol))
            return SHORT_PUNCTUATION_GAP_MS;
    }

    // A model-window cut without punctuation is a continuous phrase. Do not
    // insert samples between this chunk and the next one.
    return 0;
}

static bool contains_speakable_text(
    const std::string& text)
{
    for (size_t i = 0;
         i < text.size();)
    {
        const unsigned char c =
            static_cast<unsigned char>(
                text[i]);

        size_t len = 1;

        if (c >= 0xF0)
            len = 4;
        else if (c >= 0xE0)
            len = 3;
        else if (c >= 0xC0)
            len = 2;

        len = std::min(
            len,
            text.size() - i);

        const std::string ch =
            text.substr(i, len);

        if (!is_ignored_text_symbol(ch))
            return true;

        i += len;
    }

    return false;
}

// contains_speakable_text() is intentionally permissive because it is also
// used by the punctuation/window splitter.  Before inference, perform the
// stricter check that at least one character has a pronunciation token.  A
// fragment such as "1." can then be ignored without aborting all later text.
static bool contains_tokenizable_text(
    const std::string& text,
    const std::unordered_map<std::string,std::string>& lexicon,
    const std::unordered_map<std::string,int>& table)
{
    for (size_t offset = 0;
         offset < text.size();)
    {
        const unsigned char c =
            static_cast<unsigned char>(text[offset]);

        size_t length = 1;
        if (c >= 0xF0)
            length = 4;
        else if (c >= 0xE0)
            length = 3;
        else if (c >= 0xC0)
            length = 2;

        length = std::min(
            length,
            text.size() - offset);

        const std::string ch =
            text.substr(offset, length);
        const auto pronunciation =
            lexicon.find(ch);

        if (pronunciation != lexicon.end() &&
            table.find(pronunciation->second) != table.end())
        {
            return true;
        }

        offset += length;
    }

    return false;
}

static size_t utf8_char_length(
    const std::string& text,
    size_t offset)
{
    const unsigned char c =
        static_cast<unsigned char>(text[offset]);

    size_t length = 1;
    if (c >= 0xF0)
        length = 4;
    else if (c >= 0xE0)
        length = 3;
    else if (c >= 0xC0)
        length = 2;

    return std::min(
        length,
        text.size() - offset);
}

static bool is_strong_boundary(
    const std::string& ch)
{
    static const std::vector<std::string> symbols =
    {
        "。", "！", "？", "；", ".", "!", "?", ";", "\n"
    };

    return std::find(
        symbols.begin(),
        symbols.end(),
        ch) != symbols.end();
}

static bool is_weak_boundary(
    const std::string& ch)
{
    static const std::vector<std::string> symbols =
    {
        "，", "、", "：", ",", ":"
    };

    return std::find(
        symbols.begin(),
        symbols.end(),
        ch) != symbols.end();
}

// Always split at spoken punctuation. The initial token-width safety split is
// deliberately wider than the Vocos window. After Encoder inference, any chunk
// above 600 frames is split again using its actual predicted duration.
static std::vector<std::string> split_for_model_window(
    const std::string& text)
{
    std::vector<std::string> punctuation_chunks;
    std::string current;

    for (size_t offset = 0;
         offset < text.size();)
    {
        const size_t length =
            utf8_char_length(text, offset);
        const std::string ch =
            text.substr(offset, length);

        current += ch;
        offset += length;

        if (is_strong_boundary(ch) ||
            is_weak_boundary(ch))
        {
            if (contains_speakable_text(current))
                punctuation_chunks.push_back(current);
            else if (!punctuation_chunks.empty())
                punctuation_chunks.back() += current;

            current.clear();
        }
    }

    if (!current.empty())
    {
        if (contains_speakable_text(current))
            punctuation_chunks.push_back(current);
        else if (!punctuation_chunks.empty())
            punctuation_chunks.back() += current;
    }

    std::vector<std::string> result;

    for (const auto& sentence : punctuation_chunks)
    {
        size_t begin = 0;

        while (begin < sentence.size())
        {
            size_t offset = begin;
            size_t last_weak_end = std::string::npos;
            int speakable_count = 0;

            while (offset < sentence.size())
            {
                const size_t length =
                    utf8_char_length(sentence, offset);
                const std::string ch =
                    sentence.substr(offset, length);
                const bool speakable =
                    !is_ignored_text_symbol(ch);

                if (speakable &&
                    speakable_count >=
                        MAX_SPEAKABLE_CHARS_PER_CHUNK)
                {
                    break;
                }

                offset += length;

                if (speakable)
                    ++speakable_count;

                if (is_weak_boundary(ch))
                    last_weak_end = offset;
            }

            size_t end = offset;

            if (offset < sentence.size() &&
                last_weak_end != std::string::npos &&
                last_weak_end > begin)
            {
                end = last_weak_end;
            }

            if (end <= begin)
            {
                end = begin +
                    utf8_char_length(sentence, begin);
            }

            const std::string piece =
                sentence.substr(begin, end - begin);

            if (contains_speakable_text(piece))
                result.push_back(piece);
            else if (!result.empty())
                result.back() += piece;

            begin = end;
        }
    }

    return result;
}

// Split an Encoder-overflowing chunk into two speakable pieces. Prefer weak
// punctuation near the middle; otherwise split at a UTF-8 character boundary.
static bool split_chunk_for_frame_retry(
    const std::string& text,
    std::string& left,
    std::string& right)
{
    struct Point
    {
        size_t end = 0;
        int speakable_after = 0;
        bool weak = false;
    };

    std::vector<Point> points;
    int total_speakable = 0;

    for (size_t offset = 0;
         offset < text.size();)
    {
        const size_t length =
            utf8_char_length(text, offset);
        const std::string ch =
            text.substr(offset, length);
        offset += length;

        if (!is_ignored_text_symbol(ch))
            ++total_speakable;

        points.push_back(
            Point{
                offset,
                total_speakable,
                is_weak_boundary(ch)});
    }

    if (total_speakable < 2)
        return false;

    const int target = total_speakable / 2;
    size_t fallback_end = std::string::npos;
    size_t weak_end = std::string::npos;
    int weak_distance = total_speakable + 1;

    for (const Point& point : points)
    {
        const int left_count = point.speakable_after;
        const int right_count = total_speakable - left_count;

        if (left_count < 1 || right_count < 1)
            continue;

        if (fallback_end == std::string::npos &&
            left_count >= target)
        {
            fallback_end = point.end;
        }

        const bool reasonably_balanced =
            left_count * 4 >= total_speakable &&
            right_count * 4 >= total_speakable;

        if (point.weak && reasonably_balanced)
        {
            const int distance =
                std::abs(left_count - target);
            if (distance < weak_distance)
            {
                weak_distance = distance;
                weak_end = point.end;
            }
        }
    }

    const size_t split_end =
        weak_end != std::string::npos
            ? weak_end
            : fallback_end;

    if (split_end == std::string::npos ||
        split_end == 0 ||
        split_end >= text.size())
    {
        return false;
    }

    left = text.substr(0, split_end);
    right = text.substr(split_end);

    return contains_speakable_text(left) &&
           contains_speakable_text(right);
}

bool text_to_tokens(
    const std::string& text,
    const std::unordered_map<std::string,std::string>& lexicon,
    const std::unordered_map<std::string,int>& table,
    std::vector<int64_t>& tokens)
{

    // UTF8中文拆分
    std::vector<std::string> chars;


    for(size_t i=0;i<text.size();)
    {
        unsigned char c=text[i];

        size_t len=1;

        if(c>=0xF0)
            len=4;
        else if(c>=0xE0)
            len=3;
        else if(c>=0xC0)
            len=2;


        chars.push_back(
            text.substr(i,len)
        );

        i+=len;
    }



    tokens.clear();


    // BOS
    tokens.push_back(1);


    for(auto& ch:chars)
    {
        auto it=lexicon.find(ch);

        if(it==lexicon.end())
        {
            if (is_ignored_text_symbol(ch))
                continue;

            std::cerr
                << "Skipping unsupported TTS character: "
                << ch
                << "\n";
            continue;
        }


        auto tt=table.find(it->second);

        if(tt==table.end())
        {
            std::cerr
                << "Skipping pronunciation not in token table: "
                << it->second
                << " (character "
                << ch
                << ")\n";
            continue;
        }


        tokens.push_back(tt->second);

        // separator
        tokens.push_back(1);
    }

    if (tokens.size() == 1)
    {
        std::cerr
            << "No speakable characters in text\n";
        return false;
    }
    std::cout<<"tokens: ";

    for(auto t:tokens)
    {
        std::cout<<t<<" ";
    }

    std::cout<<std::endl;

    return true;
}

static bool load_file(
    const std::string& path,
    std::vector<unsigned char>& data)
{
    std::ifstream f(
        path,
        std::ios::binary | std::ios::ate);

    if (!f.is_open())
    {
        std::cerr
            << "Failed to open: "
            << path << "\n";
        return false;
    }

    const std::streamsize size = f.tellg();

    if (size <= 0)
        return false;

    f.seekg(0, std::ios::beg);

    data.resize(
        static_cast<size_t>(size));

    return static_cast<bool>(
        f.read(
            reinterpret_cast<char*>(data.data()),
            size));
}


// ============================================================
// Load float32 .npy
// ============================================================

static bool load_npy_float32(
    const std::string& path,
    std::vector<float>& out)
{
    std::ifstream f(path, std::ios::binary);

    if (!f.is_open())
    {
        std::cerr
            << "Failed to open NPY: "
            << path << "\n";
        return false;
    }

    char magic[6];

    f.read(magic, 6);

    if (std::memcmp(
            magic,
            "\x93NUMPY",
            6) != 0)
    {
        std::cerr
            << "Invalid NPY: "
            << path << "\n";
        return false;
    }

    uint8_t major = 0;
    uint8_t minor = 0;

    f.read(
        reinterpret_cast<char*>(&major),
        1);

    f.read(
        reinterpret_cast<char*>(&minor),
        1);

    uint32_t header_len = 0;

    if (major == 1)
    {
        uint16_t len16 = 0;

        f.read(
            reinterpret_cast<char*>(&len16),
            2);

        header_len = len16;
    }
    else
    {
        f.read(
            reinterpret_cast<char*>(&header_len),
            4);
    }

    std::string header(
        header_len,
        '\0');

    f.read(
        header.data(),
        header_len);

    if (
        header.find("<f4") == std::string::npos &&
        header.find("'f4'") == std::string::npos)
    {
        std::cerr
            << "NPY is not float32: "
            << path << "\n";
        return false;
    }

    const std::streampos data_pos =
        f.tellg();

    f.seekg(0, std::ios::end);

    const std::streampos end_pos =
        f.tellg();

    const size_t bytes =
        static_cast<size_t>(
            end_pos - data_pos);

    if (bytes % sizeof(float) != 0)
    {
        std::cerr
            << "Invalid NPY size: "
            << path << "\n";
        return false;
    }

    out.resize(
        bytes / sizeof(float));

    f.seekg(data_pos);

    f.read(
        reinterpret_cast<char*>(out.data()),
        static_cast<std::streamsize>(bytes));

    return static_cast<bool>(f);
}


// ============================================================
// Statistics
// ============================================================

static void stats(
    const char* name,
    const float* p,
    size_t n)
{
    if (!p || n == 0)
        return;

    float mn = p[0];
    float mx = p[0];

    double sum = 0.0;
    double sq = 0.0;

    size_t bad = 0;

    for (size_t i = 0; i < n; ++i)
    {
        const float v = p[i];

        if (!std::isfinite(v))
        {
            ++bad;
            continue;
        }

        mn = std::min(mn, v);
        mx = std::max(mx, v);

        sum += v;
        sq +=
            static_cast<double>(v) *
            static_cast<double>(v);
    }

    std::cout
        << name
        << ": min=" << mn
        << " max=" << mx
        << " mean="
        << sum / static_cast<double>(n)
        << " rms="
        << std::sqrt(
            sq / static_cast<double>(n))
        << " nonfinite="
        << bad
        << "\n";
}


// ============================================================
// Model init
// ============================================================

static bool init_model(
    const std::string& path,
    std::vector<unsigned char>& model,
    rknn_context& ctx)
{
    if (!load_file(path, model))
        return false;

    const int ret =
        rknn_init(
            &ctx,
            model.data(),
            static_cast<uint32_t>(
                model.size()),
            0,
            nullptr);

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "rknn_init failed: "
            << path
            << " ret="
            << ret
            << "\n";

        return false;
    }

    // All Matcha contexts share NPU core 2. Only one context runs at a time,
    // but pinning every stage prevents AUTO from contending with fatigue
    // detection on core 0.
    const int core_ret =
        rknn_set_core_mask(
            ctx,
            RKNN_NPU_CORE_2);

    if (core_ret != RKNN_SUCC)
    {
        std::cerr
            << "rknn_set_core_mask(CORE_2) failed: "
            << path
            << " ret="
            << core_ret
            << "\n";
        rknn_destroy(ctx);
        ctx = 0;
        return false;
    }

    return true;
}


// ============================================================
// Save raw float32
// ============================================================

static bool save_float32(
    const std::string& path,
    const std::vector<float>& data)
{
    std::ofstream f(
        path,
        std::ios::binary);

    if (!f.is_open())
        return false;

    f.write(
        reinterpret_cast<const char*>(
            data.data()),
        static_cast<std::streamsize>(
            data.size() *
            sizeof(float)));

    return static_cast<bool>(f);
}



// ============================================================
// Radix-2 FFT / IFFT
// ============================================================

static void fft(
    std::vector<std::complex<float>>& a,
    bool inverse)
{
    const size_t n = a.size();

    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;

        for (; j & bit; bit >>= 1)
            j ^= bit;

        j ^= bit;

        if (i < j)
            std::swap(a[i], a[j]);
    }

    for (size_t len = 2;
         len <= n;
         len <<= 1)
    {
        const float angle =
            2.0f * PI_F /
            static_cast<float>(len) *
            (inverse ? 1.0f : -1.0f);

        const std::complex<float> wlen(
            std::cos(angle),
            std::sin(angle));

        for (size_t i = 0;
             i < n;
             i += len)
        {
            std::complex<float> w(
                1.0f,
                0.0f);

            for (size_t j = 0;
                 j < len / 2;
                 ++j)
            {
                const std::complex<float> u =
                    a[i + j];

                const std::complex<float> v =
                    a[i + j + len / 2] *
                    w;

                a[i + j] =
                    u + v;

                a[i + j + len / 2] =
                    u - v;

                w *= wlen;
            }
        }
    }

    if (inverse)
    {
        const float inv_n =
            1.0f /
            static_cast<float>(n);

        for (auto& v : a)
            v *= inv_n;
    }
}


// ============================================================
// Vocos ISTFT
//
// RKNN outputs:
//   mag/x/y [1, 513, 600]
//
// This follows the centered ISTFT behavior used by Vocos:
//   - periodic Hann window
//   - overlap-add
//   - divide by sum(window^2)
//   - center crop N_FFT/2 from both sides
// ============================================================

static std::vector<float> vocos_istft_centered(
    const float* mag,
    const float* vx,
    const float* vy,
    int storage_frames,
    int synthesis_frames)
{
    if (!mag || !vx || !vy ||
        storage_frames <= 0 ||
        synthesis_frames <= 0 ||
        synthesis_frames > storage_frames)
    {
        return {};
    }

    const int raw_len =
        N_FFT +
        (synthesis_frames - 1) *
        HOP_LENGTH;

    std::vector<float> audio(
        raw_len,
        0.0f);

    std::vector<float> window_sum(
        raw_len,
        0.0f);

    std::vector<float> window(
        N_FFT);

    // Periodic Hann:
    // 0.5 - 0.5*cos(2*pi*n/N)
    for (int n = 0;
         n < N_FFT;
         ++n)
    {
        window[n] =
            0.5f -
            0.5f *
            std::cos(
                2.0f *
                PI_F *
                static_cast<float>(n) /
                static_cast<float>(N_FFT));
    }

    std::vector<std::complex<float>>
        spectrum(
            N_FFT);

    for (int t = 0;
         t < synthesis_frames;
         ++t)
    {
        std::fill(
            spectrum.begin(),
            spectrum.end(),
            std::complex<float>(
                0.0f,
                0.0f));

        for (int k = 0;
             k < VOCOS_FREQ_BINS;
             ++k)
        {
            const size_t idx =
                static_cast<size_t>(k) *
                storage_frames +
                t;

            const float magnitude =
                mag[idx];

            float re =
                magnitude *
                vx[idx];

            float im =
                magnitude *
                vy[idx];

            if (
                k == 0 ||
                k == N_FFT / 2)
            {
                im = 0.0f;
            }

            spectrum[k] =
                std::complex<float>(
                    re,
                    im);

            if (
                k > 0 &&
                k < N_FFT / 2)
            {
                spectrum[
                    N_FFT - k] =
                    std::complex<float>(
                        re,
                        -im);
            }
        }

        fft(
            spectrum,
            true);

        const int start =
            t *
            HOP_LENGTH;

        for (int n = 0;
             n < N_FFT;
             ++n)
        {
            const float w =
                window[n];

            audio[
                start + n] +=
                spectrum[n].real() *
                w;

            window_sum[
                start + n] +=
                w * w;
        }
    }

    for (int i = 0;
         i < raw_len;
         ++i)
    {
        if (
            window_sum[i] >
            1.0e-10f)
        {
            audio[i] /=
                window_sum[i];
        }
        else
        {
            audio[i] =
                0.0f;
        }
    }

    const int crop =
        N_FFT / 2;

    if (
        raw_len <=
        crop * 2)
    {
        return {};
    }

    return std::vector<float>(
        audio.begin() + crop,
        audio.end() - crop);
}


// ============================================================
// Linear resample: validated Vocos 22050 Hz -> final 16000 Hz
// ============================================================

static std::vector<float> resample_linear(
    const std::vector<float>& input,
    int src_rate,
    int dst_rate)
{
    if (input.empty() ||
        src_rate <= 0 ||
        dst_rate <= 0)
    {
        return {};
    }

    if (src_rate == dst_rate)
        return input;

    const size_t out_count =
        static_cast<size_t>(
            std::llround(
                static_cast<double>(input.size()) *
                static_cast<double>(dst_rate) /
                static_cast<double>(src_rate)));

    if (out_count == 0)
        return {};

    std::vector<float> output(out_count);
    const double step =
        static_cast<double>(src_rate) /
        static_cast<double>(dst_rate);

    for (size_t i = 0;
         i < out_count;
         ++i)
    {
        const double pos =
            static_cast<double>(i) * step;
        const size_t i0 =
            std::min(
                static_cast<size_t>(pos),
                input.size() - 1);
        const size_t i1 =
            std::min(i0 + 1, input.size() - 1);
        const float fraction =
            static_cast<float>(
                pos - static_cast<double>(i0));

        output[i] =
            input[i0] * (1.0f - fraction) +
            input[i1] * fraction;
    }

    return output;
}


// ============================================================
// Trim model-generated silence at sentence boundaries
// ============================================================

static std::vector<float> trim_sentence_silence(
    const std::vector<float>& input,
    int sample_rate,
    int leading_padding_ms,
    int trailing_padding_ms)
{
    if (input.empty() || sample_rate <= 0)
        return input;

    float peak = 0.0f;

    for (float v : input)
    {
        if (std::isfinite(v))
        {
            peak = std::max(
                peak,
                std::fabs(v));
        }
    }

    if (peak <= 1.0e-8f)
        return input;

    const size_t window =
        std::max<size_t>(
            1,
            static_cast<size_t>(
                sample_rate * SILENCE_WINDOW_MS / 1000));

    const float rms_threshold =
        peak * SILENCE_RMS_RATIO;

    size_t first_active = input.size();
    size_t last_active = 0;

    for (size_t begin = 0;
         begin < input.size();
         begin += window)
    {
        const size_t end =
            std::min(
                begin + window,
                input.size());

        double sum_sq = 0.0;

        for (size_t i = begin;
             i < end;
             ++i)
        {
            const float v =
                std::isfinite(input[i])
                    ? input[i]
                    : 0.0f;

            sum_sq +=
                static_cast<double>(v) *
                static_cast<double>(v);
        }

        const float rms =
            static_cast<float>(
                std::sqrt(
                    sum_sq /
                    static_cast<double>(end - begin)));

        if (rms >= rms_threshold)
        {
            if (first_active == input.size())
                first_active = begin;

            last_active = end;
        }
    }

    // If no reliable speech boundary is found, keep the original audio.
    if (first_active == input.size())
        return input;

    const size_t leading_padding =
        static_cast<size_t>(
            sample_rate * leading_padding_ms / 1000);

    const size_t trailing_padding =
        static_cast<size_t>(
            sample_rate * trailing_padding_ms / 1000);

    const size_t trim_begin =
        first_active > leading_padding
            ? first_active - leading_padding
            : 0;

    const size_t trim_end =
        std::min(
            input.size(),
            last_active + trailing_padding);

    return std::vector<float>(
        input.begin() + trim_begin,
        input.begin() + trim_end);
}


// ============================================================
// Compress abnormal silence inside one punctuation-free chunk
// ============================================================

static std::vector<float> compress_abnormal_internal_silence(
    const std::vector<float>& input,
    int sample_rate,
    size_t& compressed_runs,
    size_t& removed_samples)
{
    compressed_runs = 0;
    removed_samples = 0;

    if (input.empty() || sample_rate <= 0)
        return input;

    float peak = 0.0f;
    for (float v : input)
    {
        if (std::isfinite(v))
            peak = std::max(peak, std::fabs(v));
    }

    if (peak <= 1.0e-8f)
        return input;

    const size_t window =
        std::max<size_t>(
            1,
            static_cast<size_t>(
                sample_rate * SILENCE_WINDOW_MS / 1000));

    const size_t minimum_silence =
        static_cast<size_t>(
            sample_rate * INTERNAL_SILENCE_MIN_MS / 1000);

    // Keep 35 ms before overlap; the 5 ms crossfade makes the audible retained
    // interval approximately 30 ms while avoiding a discontinuity.
    const size_t keep_silence =
        static_cast<size_t>(
            sample_rate * INTERNAL_SILENCE_KEEP_MS / 1000);

    const size_t crossfade =
        static_cast<size_t>(
            sample_rate * INTERNAL_SILENCE_CROSSFADE_MS / 1000);

    const float threshold =
        peak * INTERNAL_SILENCE_RMS_RATIO;

    struct SilenceCut
    {
        size_t begin = 0;
        size_t end = 0;
    };

    std::vector<SilenceCut> cuts;
    size_t quiet_begin = std::string::npos;

    for (size_t begin = 0;
         begin < input.size();
         begin += window)
    {
        const size_t end =
            std::min(begin + window, input.size());

        double sum_sq = 0.0;
        for (size_t i = begin; i < end; ++i)
        {
            const float v =
                std::isfinite(input[i])
                    ? input[i]
                    : 0.0f;
            sum_sq +=
                static_cast<double>(v) *
                static_cast<double>(v);
        }

        const float rms =
            static_cast<float>(
                std::sqrt(
                    sum_sq /
                    static_cast<double>(end - begin)));

        const bool quiet = rms < threshold;

        if (quiet && quiet_begin == std::string::npos)
            quiet_begin = begin;

        const bool run_ended =
            !quiet || end == input.size();

        if (run_ended && quiet_begin != std::string::npos)
        {
            const size_t quiet_end =
                quiet ? end : begin;

            const size_t quiet_size =
                quiet_end - quiet_begin;

            // Boundary silence is handled by trim_sentence_silence(). Only
            // compress an interior run with enough samples on both sides.
            if (quiet_size >= minimum_silence &&
                quiet_begin > crossfade &&
                quiet_end + crossfade < input.size() &&
                quiet_size > keep_silence)
            {
                const size_t keep_left =
                    keep_silence / 2;
                const size_t keep_right =
                    keep_silence - keep_left;

                cuts.push_back(
                    SilenceCut{
                        quiet_begin + keep_left,
                        quiet_end - keep_right});
            }

            quiet_begin = std::string::npos;
        }
    }

    if (cuts.empty())
        return input;

    std::vector<float> output;
    output.reserve(input.size());
    size_t cursor = 0;

    for (const SilenceCut& cut : cuts)
    {
        if (cut.begin <= cursor ||
            cut.end <= cut.begin ||
            cut.end + crossfade > input.size())
        {
            continue;
        }

        output.insert(
            output.end(),
            input.begin() + cursor,
            input.begin() + cut.begin);

        const size_t actual_crossfade =
            std::min(crossfade, output.size());

        for (size_t i = 0;
             i < actual_crossfade;
             ++i)
        {
            const float alpha =
                static_cast<float>(i + 1) /
                static_cast<float>(actual_crossfade + 1);

            const size_t output_index =
                output.size() - actual_crossfade + i;

            output[output_index] =
                output[output_index] * (1.0f - alpha) +
                input[cut.end + i] * alpha;
        }

        cursor = cut.end + actual_crossfade;
        ++compressed_runs;
    }

    output.insert(
        output.end(),
        input.begin() + cursor,
        input.end());

    removed_samples = input.size() - output.size();
    return output;
}


// ============================================================
// WAV helpers
// ============================================================

static void write_u16(
    std::ofstream& f,
    uint16_t value)
{
    const unsigned char b[2] = {
        static_cast<unsigned char>(
            value & 0xff),
        static_cast<unsigned char>(
            (value >> 8) & 0xff)
    };

    f.write(
        reinterpret_cast<
            const char*>(b),
        2);
}

static void write_u32(
    std::ofstream& f,
    uint32_t value)
{
    const unsigned char b[4] = {
        static_cast<unsigned char>(
            value & 0xff),
        static_cast<unsigned char>(
            (value >> 8) & 0xff),
        static_cast<unsigned char>(
            (value >> 16) & 0xff),
        static_cast<unsigned char>(
            (value >> 24) & 0xff)
    };

    f.write(
        reinterpret_cast<
            const char*>(b),
        4);
}


static bool write_wav_pcm16(
    const std::string& path,
    const std::vector<float>& input,
    int sample_rate)
{
    if (input.empty())
    {
        std::cerr
            << "Audio is empty\n";
        return false;
    }

    float peak = 0.0f;

    for (float v : input)
    {
        if (std::isfinite(v))
        {
            peak =
                std::max(
                    peak,
                    std::fabs(v));
        }
    }

    // Keep the same normalization used in the standalone
    // listening tests.
    float gain = 1.0f;

    if (peak > 1.0e-8f)
    {
        gain =
            0.95f /
            peak;
    }

    std::vector<int16_t> pcm(
        input.size());

    for (size_t i = 0;
         i < input.size();
         ++i)
    {
        float v =
            input[i] *
            gain;

        if (!std::isfinite(v))
            v = 0.0f;

        v =
            std::max(
                -1.0f,
                std::min(
                    1.0f,
                    v));

        pcm[i] =
            static_cast<int16_t>(
                std::lrint(
                    v *
                    32767.0f));
    }

    std::ofstream f(
        path,
        std::ios::binary);

    if (!f.is_open())
    {
        std::cerr
            << "Failed to create WAV: "
            << path
            << "\n";
        return false;
    }

    const uint16_t channels = 1;
    const uint16_t bits = 16;

    const uint32_t byte_rate =
        static_cast<uint32_t>(
            sample_rate *
            channels *
            bits / 8);

    const uint16_t block_align =
        channels *
        bits / 8;

    const uint32_t data_size =
        static_cast<uint32_t>(
            pcm.size() *
            sizeof(int16_t));

    f.write("RIFF", 4);
    write_u32(
        f,
        36 + data_size);

    f.write("WAVE", 4);

    f.write("fmt ", 4);
    write_u32(f, 16);
    write_u16(f, 1);
    write_u16(f, channels);
    write_u32(
        f,
        static_cast<uint32_t>(
            sample_rate));
    write_u32(
        f,
        byte_rate);
    write_u16(
        f,
        block_align);
    write_u16(
        f,
        bits);

    f.write("data", 4);
    write_u32(
        f,
        data_size);

    f.write(
        reinterpret_cast<
            const char*>(
                pcm.data()),
        static_cast<
            std::streamsize>(
                data_size));

    f.close();

    std::cout
        << "WAV: "
        << path
        << " samples="
        << input.size()
        << " rate="
        << sample_rate
        << " peak="
        << peak
        << " gain="
        << gain
        << "\n";

    return true;
}


// ============================================================
// In-process Matcha API
// ============================================================

bool matchaRknnSynthesize(
    const std::string &modelDir,
    const std::string &text,
    std::vector<float> *audio16k,
    const std::atomic<bool> *cancelRequested,
    std::string *error)
{
    if (!audio16k)
    {
        if (error)
            *error = "audio16k output is null";
        return false;
    }

    audio16k->clear();

    if (cancelRequested && cancelRequested->load())
        return true;


    std::cout
    <<"TEXT="
    <<text
    <<std::endl;
    const std::string root =
        !modelDir.empty() && modelDir.back() == '/'
            ? modelDir
            : modelDir + "/";

    static std::string initialized_root;
    static bool models_initialized = false;

    const std::string encoder_path =
        root +
        "matcha-encoder-768-no-ceil-fp16.rknn";

    const std::vector<int> estimator_buckets(
        std::begin(ESTIMATOR_BUCKETS),
        std::end(ESTIMATOR_BUCKETS));

    std::vector<std::string> estimator_paths;
    for (const int frames : estimator_buckets)
    {
        estimator_paths.push_back(
            root +
            "matcha-estimator-" +
            std::to_string(frames) +
            "-fp16.rknn");
    }

    const std::string vocos_path =
        root +
        "deploy_models/vocos-22khz-univ-fp16.rknn";
    static std::unordered_map<std::string,std::string> lexicon;

    static std::unordered_map<std::string,int> token_table;


    if (models_initialized && initialized_root != root)
    {
        if (error)
            *error = "Matcha is already initialized with a different model directory";
        return false;
    }

    if (!models_initialized &&
        (!load_lexicon(
            root + "lexicon.txt",
            lexicon) ||
        !load_tokens(
            root + "tokens.txt",
            token_table)))
    {
        if (error)
            *error = "Failed to load Matcha lexicon or tokens";
        return false;
    }

    // ==========================================   ==============
    // Init models
    // ========================================================

    static std::vector<unsigned char> encoder_model;
    static std::vector<std::vector<unsigned char>> estimator_models(
        estimator_buckets.size());
    static std::vector<unsigned char> vocos_model;

    static rknn_context encoder_ctx = 0;
    static std::vector<rknn_context> estimator_contexts(
        estimator_buckets.size(),
        0);
    static rknn_context vocos_ctx = 0;

    const auto destroy_estimators = [&]()
    {
        for (rknn_context& context : estimator_contexts)
        {
            if (context != 0)
            {
                rknn_destroy(context);
                context = 0;
            }
        }
    };

    if (!models_initialized && !init_model(
            encoder_path,
            encoder_model,
            encoder_ctx))
    {
        if (error)
            *error = "Failed to initialize Matcha Encoder";
        return false;
    }

    if (!models_initialized)
    {
        for (size_t i = 0;
             i < estimator_buckets.size();
             ++i)
        {
            if (!init_model(
                    estimator_paths[i],
                    estimator_models[i],
                    estimator_contexts[i]))
            {
                destroy_estimators();
                rknn_destroy(encoder_ctx);
                encoder_ctx = 0;
                if (error)
                    *error = "Failed to initialize a Matcha Estimator bucket";
                return false;
            }
        }
    }

    if (!models_initialized && !init_model(
            vocos_path,
            vocos_model,
            vocos_ctx))
    {
        destroy_estimators();

        rknn_destroy(
            encoder_ctx);

        encoder_ctx = 0;
        if (error)
            *error = "Failed to initialize Matcha Vocos";
        return false;
    }

    std::cout
        << "Encoder + 5 Estimator buckets + Vocos init = OK\n";

    // Reuse the exact NumPy default_rng(42) noise tensor used by the
    // previously validated pipeline. std::mt19937 does not generate the same
    // sequence even when both generators use the number 42 as their seed.
    static std::vector<float> fixed_noise;
    const std::string noise_path =
        root + "matcha-noise-seed42-768.npy";

    if (!models_initialized &&
        (!load_npy_float32(
            noise_path,
            fixed_noise) ||
        fixed_noise.size() !=
            static_cast<size_t>(
                MEL_BINS * INTERNAL_FRAMES)))
    {
        std::cerr
            << "Expected noise [1,80,768] ("
            << MEL_BINS * INTERNAL_FRAMES
            << " float32 values): "
            << noise_path
            << ", got "
            << fixed_noise.size()
            << "\n";

        rknn_destroy(vocos_ctx);
        destroy_estimators();
        rknn_destroy(encoder_ctx);
        encoder_ctx = 0;
        vocos_ctx = 0;
        if (error)
            *error = "Invalid Matcha seed-42 noise tensor";
        return false;
    }

    std::cout
        << "Loaded fixed NumPy seed-42 noise once: "
        << fixed_noise.size()
        << " values\n";

    // These tensors depend only on the three fixed ODE time steps, not on
    // the sentence.  Load them once so long text does not repeatedly touch
    // the filesystem between chunks.
    static std::vector<std::vector<float>> time_emb(
        N_ODE_STEPS);

    if (!models_initialized)
    {
        for (int step = 0;
             step < N_ODE_STEPS;
             ++step)
        {
            const std::string path =
                root +
                "time_emb_step" +
                std::to_string(step) +
                ".npy";

            if (!load_npy_float32(
                    path,
                    time_emb[step]))
            {
                rknn_destroy(vocos_ctx);
                destroy_estimators();
                rknn_destroy(encoder_ctx);
                encoder_ctx = 0;
                vocos_ctx = 0;
                if (error)
                    *error = "Failed to load Matcha time embedding";
                return false;
            }

            const size_t expected =
                static_cast<size_t>(
                    N_TIME_BLOCKS *
                    TIME_EMB_DIM);

            if (time_emb[step].size() != expected)
            {
                std::cerr
                    << "Unexpected time_emb size step="
                    << step
                    << " got="
                    << time_emb[step].size()
                    << " expected="
                    << expected
                    << "\n";

                rknn_destroy(vocos_ctx);
                destroy_estimators();
                rknn_destroy(encoder_ctx);
                encoder_ctx = 0;
                vocos_ctx = 0;
                if (error)
                    *error = "Unexpected Matcha time embedding size";
                return false;
            }
        }

        initialized_root = root;
        models_initialized = true;
    }

    std::cout << "Loaded 3 time embeddings once\n";

    // Empty text is used by TTSService::init() to validate and preload all
    // model files without running inference.
    if (text.empty())
        return true;

    // Run the existing single-sentence inference pipeline for every sentence.
    // The lambda captures the already initialized models and dictionaries, so
    // RKNN models are loaded only once for the complete input text.
    bool last_tts_needs_frame_split = false;

    auto run_tts =
        [&](const std::string& tts_text) -> std::vector<float>
    {
    last_tts_needs_frame_split = false;


    // ========================================================
    // Exact sherpa frontend tokens:
    //
    // “你好小车”
    //
    // [1,1116,1,597,1,1767,1,180,1]
    // ========================================================

    std::vector<int64_t> token_ids;

    if (!text_to_tokens(
            tts_text,
            lexicon,
            token_table,
            token_ids))
    {
        std::cerr
            << "Text-to-token conversion failed: "
            << tts_text
            << "\n";
        return {};
    }

    if (token_ids.size() > TOKEN_WIDTH)
    {
        last_tts_needs_frame_split = true;
        std::cerr
            << "Too many tokens in sentence: "
            << token_ids.size()
            << " (maximum "
            << TOKEN_WIDTH
            << ")\n";
        return {};
    }

    int64_t tokens[TOKEN_WIDTH] = {0};


    for(size_t i=0;i<token_ids.size();i++)
    {
        tokens[i]=token_ids[i];
    }

    int64_t x_length = token_ids.size();

    float noise_scale = 1.0f;
    float length_scale = TTS_LENGTH_SCALE;


    // ========================================================
    // Encoder
    // ========================================================

    rknn_input encoder_inputs[3]{};

    encoder_inputs[0].index = 0;
    encoder_inputs[0].buf = tokens;
    encoder_inputs[0].size = sizeof(tokens);
    encoder_inputs[0].type =
        RKNN_TENSOR_INT64;
    encoder_inputs[0].fmt =
        RKNN_TENSOR_UNDEFINED;
    encoder_inputs[0].pass_through = 0;

    encoder_inputs[1].index = 1;
    encoder_inputs[1].buf = &x_length;
    encoder_inputs[1].size =
        sizeof(x_length);
    encoder_inputs[1].type =
        RKNN_TENSOR_INT64;
    encoder_inputs[1].fmt =
        RKNN_TENSOR_UNDEFINED;
    encoder_inputs[1].pass_through = 0;

    encoder_inputs[2].index = 2;
    encoder_inputs[2].buf =
        &length_scale;
    encoder_inputs[2].size =
        sizeof(float);
    encoder_inputs[2].type =
        RKNN_TENSOR_FLOAT32;
    encoder_inputs[2].fmt =
        RKNN_TENSOR_UNDEFINED;
    encoder_inputs[2].pass_through = 0;

    int ret =
        rknn_inputs_set(
            encoder_ctx,
            3,
            encoder_inputs);

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "Encoder inputs_set failed: "
            << ret << "\n";

        return {};
    }


    const auto enc_begin =
        std::chrono::steady_clock::now();

    ret =
        rknn_run(
            encoder_ctx,
            nullptr);

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "Encoder run failed: "
            << ret << "\n";

        return {};
    }


    rknn_output encoder_outputs[2]{};

    for (int i = 0; i < 2; ++i)
    {
        encoder_outputs[i].index = i;
        encoder_outputs[i].want_float = 1;
        encoder_outputs[i].is_prealloc = 0;
    }


    ret =
        rknn_outputs_get(
            encoder_ctx,
            2,
            encoder_outputs,
            nullptr);

    const auto enc_end =
        std::chrono::steady_clock::now();

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "Encoder outputs_get failed: "
            << ret << "\n";

        return {};
    }


    const float* encoder_mu =
        reinterpret_cast<const float*>(
            encoder_outputs[0].buf);

    const float* encoder_mask =
        reinterpret_cast<const float*>(
            encoder_outputs[1].buf);

    std::vector<float> mu_data(
        encoder_mu,
        encoder_mu +
            MEL_BINS * INTERNAL_FRAMES);

    std::vector<float> mask_data(
        encoder_mask,
        encoder_mask + INTERNAL_FRAMES);

    rknn_outputs_release(
        encoder_ctx,
        2,
        encoder_outputs);

    float* mu = mu_data.data();
    float* mask = mask_data.data();


    const double encoder_ms =
        std::chrono::duration<
            double,
            std::milli>(
                enc_end - enc_begin)
            .count();


    std::cout
        << "\n========== ENCODER ==========\n";

    std::cout
        << "Encoder time = "
        << encoder_ms
        << " ms\n";

    stats(
        "mu",
        mu,
        MEL_BINS *
        INTERNAL_FRAMES);

    stats(
        "mask",
        mask,
        INTERNAL_FRAMES);

    // ========================================================
    // Mask check
    // ========================================================

    int mask_count = 0;
    int mask_last = 0;

    for (int t = 0;
         t < INTERNAL_FRAMES;
         ++t)
    {
        if (mask[t] > 0.5f)
        {
            ++mask_count;
            mask_last = t + 1;
        }
    }

    std::cout
        << "mask_count = "
        << mask_count
        << "\n";

    std::cout
        << "mask_last  = "
        << mask_last
        << "\n";

    if (mask_count <= 1)
    {
        std::cerr
            << "Encoder produced too few valid frames: "
            << mask_count
            << "\n";
        return {};
    }

    if (mask_count >= INTERNAL_FRAMES)
    {
        last_tts_needs_frame_split = true;
        std::cerr
            << "Encoder output reached the 768-frame limit. "
            << "Scheduling an automatic text split: "
            << tts_text
            << "\n";
        return {};
    }

    if (mask_count > VOCOS_FRAMES)
    {
        last_tts_needs_frame_split = true;
        std::cerr
            << "Encoder produced "
            << mask_count
            << " valid frames, exceeding the validated Vocos-600 window. "
            << "Scheduling an automatic text split: "
            << tts_text
            << "\n";
        return {};
    }

    size_t estimator_bucket_index =
        estimator_buckets.size() - 1;
    for (size_t i = 0;
         i < estimator_buckets.size();
         ++i)
    {
        if (mask_count <= estimator_buckets[i])
        {
            estimator_bucket_index = i;
            break;
        }
    }

    const int estimator_frames =
        estimator_buckets[estimator_bucket_index];
    const rknn_context estimator_ctx =
        estimator_contexts[estimator_bucket_index];

    if (mask_count > estimator_frames)
    {
        std::cerr
            << "No Estimator bucket can hold "
            << mask_count
            << " frames; selected capacity="
            << estimator_frames
            << "\n";
        last_tts_needs_frame_split = true;
        return {};
    }

    std::cout
        << "Estimator bucket = "
        << estimator_frames
        << " for "
        << mask_count
        << " valid frames\n";

    // Repack Encoder-768 output into the selected Estimator bucket. This is
    // essential because channel-major tensors use the frame count as stride.
    std::vector<float> estimator_mu(
        static_cast<size_t>(MEL_BINS) * estimator_frames,
        0.0f);
    std::vector<float> estimator_mask(
        estimator_frames,
        0.0f);
    std::vector<float> z(
        static_cast<size_t>(MEL_BINS) * estimator_frames,
        0.0f);

    for (int t = 0;
         t < mask_count;
         ++t)
    {
        estimator_mask[t] = mask[t];
    }

    for (int c = 0;
         c < MEL_BINS;
         ++c)
    {
        for (int t = 0;
             t < mask_count;
             ++t)
        {
            const size_t source =
                static_cast<size_t>(c) *
                INTERNAL_FRAMES + t;
            const size_t destination =
                static_cast<size_t>(c) *
                estimator_frames + t;

            estimator_mu[destination] = mu[source];
            z[destination] =
                fixed_noise[source] * noise_scale;
        }
    }


    stats(
        "CPU fixed z0",
        z.data(),
        z.size());


    // ========================================================
    // Estimator ×3 + CPU FP32 ODE
    // ========================================================

    double estimator_total_ms = 0.0;

    for (int step = 0;
         step < N_ODE_STEPS;
         ++step)
    {
        if (cancelRequested && cancelRequested->load())
            return {};

        const auto& te =
            time_emb[step];

        rknn_input inputs[9]{};


        // z
        inputs[0].index = 0;
        inputs[0].buf = z.data();
        inputs[0].size =
            static_cast<uint32_t>(
                z.size() *
                sizeof(float));
        inputs[0].type =
            RKNN_TENSOR_FLOAT32;
        inputs[0].fmt =
            RKNN_TENSOR_UNDEFINED;
        inputs[0].pass_through = 0;


        // mu
        inputs[1].index = 1;
        inputs[1].buf = estimator_mu.data();
        inputs[1].size =
            MEL_BINS *
            estimator_frames *
            sizeof(float);
        inputs[1].type =
            RKNN_TENSOR_FLOAT32;
        inputs[1].fmt =
            RKNN_TENSOR_UNDEFINED;
        inputs[1].pass_through = 0;


        // mask
        inputs[2].index = 2;
        inputs[2].buf = estimator_mask.data();
        inputs[2].size =
            estimator_frames *
            sizeof(float);
        inputs[2].type =
            RKNN_TENSOR_FLOAT32;
        inputs[2].fmt =
            RKNN_TENSOR_UNDEFINED;
        inputs[2].pass_through = 0;


        // time_emb_0 ... time_emb_5
        for (int i = 0;
             i < N_TIME_BLOCKS;
             ++i)
        {
            inputs[3 + i].index =
                3 + i;

            inputs[3 + i].buf =
                const_cast<float*>(
                    te.data() +
                    i *
                    TIME_EMB_DIM);

            inputs[3 + i].size =
                TIME_EMB_DIM *
                sizeof(float);

            inputs[3 + i].type =
                RKNN_TENSOR_FLOAT32;

            inputs[3 + i].fmt =
                RKNN_TENSOR_UNDEFINED;

            inputs[3 + i].pass_through =
                0;
        }


        ret =
            rknn_inputs_set(
                estimator_ctx,
                9,
                inputs);

        if (ret != RKNN_SUCC)
        {
            std::cerr
                << "Estimator inputs_set failed "
                << "step="
                << step
                << " ret="
                << ret
                << "\n";

            return {};
        }


        const auto begin =
            std::chrono::steady_clock::now();

        ret =
            rknn_run(
                estimator_ctx,
                nullptr);

        if (ret != RKNN_SUCC)
        {
            std::cerr
                << "Estimator run failed "
                << "step="
                << step
                << " ret="
                << ret
                << "\n";

            return {};
        }


        rknn_output output{};

        output.index = 0;
        output.want_float = 1;
        output.is_prealloc = 0;


        ret =
            rknn_outputs_get(
                estimator_ctx,
                1,
                &output,
                nullptr);

        const auto end =
            std::chrono::steady_clock::now();

        if (ret != RKNN_SUCC)
        {
            std::cerr
                << "Estimator outputs_get failed "
                << "step="
                << step
                << "\n";

            return {};
        }


        const double ms =
            std::chrono::duration<
                double,
                std::milli>(
                    end - begin)
                .count();

        estimator_total_ms += ms;


        float* velocity =
            reinterpret_cast<float*>(
                output.buf);


        std::cout
            << "\nEstimator step "
            << step
            << " = "
            << ms
            << " ms\n";

        stats(
            "velocity",
            velocity,
            MEL_BINS *
            estimator_frames);


        // CPU FP32 Euler accumulation
        for (size_t i = 0;
             i < z.size();
             ++i)
        {
            const size_t frame =
                i % estimator_frames;

            if (estimator_mask[frame] > 0.5f)
            {
                z[i] +=
                    ODE_DT *
                    velocity[i];
            }
            else
            {
                z[i] = 0.0f;
            }
        }


        rknn_outputs_release(
            estimator_ctx,
            1,
            &output);
    }


    // ========================================================
    // Denormalize valid frames
    // ========================================================

    std::vector<float> mel(
        MEL_BINS * mask_count);


    for(int c=0;c<MEL_BINS;c++)
    {
        for(int t=0;t<mask_count;t++)
        {
            const float raw =
                z[
                    c * estimator_frames +
                    t
                ];

            mel[
                c * mask_count +
                t
            ] =
                raw * MEL_SIGMA +
                MEL_MEAN;
        }
    }


    // ========================================================
    // Final stats
    // ========================================================

    std::cout
        << "\n========================================\n";

    std::cout
        << "FINAL PHRASE SPLIT RKNN MEL\n";

    std::cout
        << "shape = [1,80,"
        << mask_count
        << "]\n";

    stats(
        "mel",
        mel.data(),
        mel.size());

    std::cout
        << "\nEncoder time      = "
        << encoder_ms
        << " ms\n";

    std::cout
        << "Estimator x3 time = "
        << estimator_total_ms
        << " ms\n";

    std::cout
        << "========================================\n";


    // ========================================================
    // Validated Vocos 22.05 kHz fixed RKNN [1,80,600].
    // Copy valid Matcha frames and zero-pad the remaining model window,
    // exactly as in the previously good-sounding pipeline.
    // ========================================================

    if (cancelRequested && cancelRequested->load())
        return {};

    const int vocos_frames = VOCOS_FRAMES;

    std::vector<float> vocos_mel(
        static_cast<size_t>(
            MEL_BINS *
            vocos_frames),
        0.0f);

    for (int c = 0;
         c < MEL_BINS;
         ++c)
    {
        for(int t=0;t<mask_count;t++)
        {
            vocos_mel[
                static_cast<size_t>(c) *
                vocos_frames +
                t] =
                mel[
                    static_cast<size_t>(c) *
                    mask_count +
                    t];
        }
    }

    rknn_input vocos_input{};

    vocos_input.index = 0;
    vocos_input.buf =
        vocos_mel.data();
    vocos_input.size =
        static_cast<uint32_t>(
            vocos_mel.size() *
            sizeof(float));
    vocos_input.type =
        RKNN_TENSOR_FLOAT32;
    vocos_input.fmt =
        RKNN_TENSOR_UNDEFINED;
    vocos_input.pass_through = 0;

    ret =
        rknn_inputs_set(
            vocos_ctx,
            1,
            &vocos_input);

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "Vocos inputs_set failed: "
            << ret
            << "\n";
        return {};
    }

    const auto vocos_begin =
        std::chrono::steady_clock::now();

    ret =
        rknn_run(
            vocos_ctx,
            nullptr);

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "Vocos run failed: "
            << ret
            << "\n";
        return {};
    }

    rknn_output vocos_outputs[3]{};

    for (int i = 0;
         i < 3;
         ++i)
    {
        vocos_outputs[i].index = i;
        vocos_outputs[i].want_float = 1;
        vocos_outputs[i].is_prealloc = 0;
    }

    ret =
        rknn_outputs_get(
            vocos_ctx,
            3,
            vocos_outputs,
            nullptr);

    const auto vocos_end =
        std::chrono::steady_clock::now();

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "Vocos outputs_get failed: "
            << ret
            << "\n";
        return {};
    }

    const double vocos_ms =
        std::chrono::duration<
            double,
            std::milli>(
                vocos_end -
                vocos_begin)
            .count();

    float* mag =
        reinterpret_cast<float*>(
            vocos_outputs[0].buf);

    float* vx =
        reinterpret_cast<float*>(
            vocos_outputs[1].buf);

    float* vy =
        reinterpret_cast<float*>(
            vocos_outputs[2].buf);

    std::cout
        << "\n========== VOCOS 22K ==========\n";

    std::cout
        << "Vocos time = "
        << vocos_ms
        << " ms\n";

    stats(
        "mag",
        mag,
        static_cast<size_t>(
            VOCOS_FREQ_BINS *
            vocos_frames));

    stats(
        "x",
        vx,
        static_cast<size_t>(
            VOCOS_FREQ_BINS *
            vocos_frames));

    stats(
        "y",
        vy,
        static_cast<size_t>(
            VOCOS_FREQ_BINS *
            vocos_frames));


    // ========================================================
    // CPU centered ISTFT
    // ========================================================

    const auto istft_begin =
        std::chrono::steady_clock::now();

    // The returned waveform keeps only (mask_count - 1) hops after the
    // centered crop. For that prefix, frame mask_count is the final Vocos
    // frame whose N_FFT window can overlap the retained samples; later padded
    // frames start outside the retained range. Processing mask_count + 1
    // frames therefore reproduces the old 600-frame prefix exactly while
    // avoiding FFT work for the remaining padded frames.
    const int istft_frames =
        std::min(
            mask_count + 1,
            vocos_frames);

    std::cout
        << "ISTFT frames = "
        << istft_frames
        << "/"
        << vocos_frames
        << " (valid="
        << mask_count
        << ")\n";

    std::vector<float> audio_22k =
        vocos_istft_centered(
            mag,
            vx,
            vy,
            vocos_frames,
            istft_frames);

    // A valid T-frame Vocos input with centered ISTFT has
    // (T - 1) * hop samples.
    const size_t wanted_22k_samples =
        (mask_count - 1) *
        HOP_LENGTH;
    if (
        audio_22k.size() >
        wanted_22k_samples)
    {
        audio_22k.resize(
            wanted_22k_samples);
    }

    const auto istft_end =
        std::chrono::steady_clock::now();

    const double istft_ms =
        std::chrono::duration<
            double,
            std::milli>(
                istft_end -
                istft_begin)
            .count();

    stats(
        "audio_22k",
        audio_22k.data(),
        audio_22k.size());

    std::cout
        << "ISTFT time = "
        << istft_ms
        << " ms\n";

    std::cout
        << "22k samples = "
        << audio_22k.size()
        << "\n";

    std::cout
        << "22k duration = "
        << static_cast<double>(
               audio_22k.size()) /
               VOCOS_SAMPLE_RATE
        << " s\n";

    // ISTFT has consumed all three RKNN output buffers.  Release them before
    // any file I/O so every return path remains safe for the next sentence.
    rknn_outputs_release(
        vocos_ctx,
        3,
        vocos_outputs);


    const auto resample_begin =
        std::chrono::steady_clock::now();

    std::vector<float> audio_16k =
        resample_linear(
            audio_22k,
            VOCOS_SAMPLE_RATE,
            OUTPUT_SAMPLE_RATE);

    const auto resample_end =
        std::chrono::steady_clock::now();

    const double resample_ms =
        std::chrono::duration<
            double,
            std::milli>(
                resample_end -
                resample_begin)
            .count();

    if (audio_16k.empty())
    {
        std::cerr << "22k -> 16k resampling failed\n";
        return {};
    }

    stats(
        "audio_16k",
        audio_16k.data(),
        audio_16k.size());

    std::cout
        << "Resample time = "
        << resample_ms
        << " ms\n";

    std::cout
        << "\n========================================\n";

    std::cout
        << "FULL BOARD TTS PASS\n";

    std::cout
        << "Encoder       = "
        << encoder_ms
        << " ms\n";

    std::cout
        << "Estimator x3  = "
        << estimator_total_ms
        << " ms\n";

    std::cout
        << "Vocos         = "
        << vocos_ms
        << " ms\n";

    std::cout
        << "ISTFT         = "
        << istft_ms
        << " ms\n";

    std::cout
        << "Resample      = "
        << resample_ms
        << " ms\n";

    std::cout
        << "========================================\n";


    return audio_16k;
    };

    const std::vector<std::string> sentences =
        split_for_model_window(text);

    std::vector<float> final_audio;
    bool tts_ok = !sentences.empty();
    int previous_pause_ms = 0;
    size_t automatic_split_count = 0;

    std::deque<std::string> pending_chunks(
        sentences.begin(),
        sentences.end());

    while (!pending_chunks.empty())
    {
        if (cancelRequested && cancelRequested->load())
            return true;

        std::string sentence =
            std::move(pending_chunks.front());
        pending_chunks.pop_front();

        if (!contains_tokenizable_text(
                sentence,
                lexicon,
                token_table))
        {
            std::cerr
                << "Skipping TTS fragment with no supported characters: "
                << sentence
                << "\n";
            continue;
        }

        std::cout
            << "SENTENCE="
            << sentence
            << std::endl;

        std::vector<float> audio =
            run_tts(sentence);

        if (audio.empty() &&
            last_tts_needs_frame_split)
        {
            std::string left;
            std::string right;

            if (automatic_split_count >= 128 ||
                !split_chunk_for_frame_retry(
                    sentence,
                    left,
                    right))
            {
                std::cerr
                    << "Cannot split the oversized TTS chunk any further: "
                    << sentence
                    << "\n";
                tts_ok = false;
                break;
            }

            ++automatic_split_count;

            std::cout
                << "AUTO SPLIT (>600 frames): "
                << sentence
                << "\n  LEFT="
                << left
                << "\n  RIGHT="
                << right
                << "\n";

            // push_front is reversed so LEFT runs before RIGHT. The models
            // stay initialized; only these two shorter inputs are inferred.
            pending_chunks.push_front(std::move(right));
            pending_chunks.push_front(std::move(left));
            continue;
        }

        if (audio.empty())
        {
            std::cerr
                << "TTS failed for sentence: "
                << sentence
                << "\n";
            tts_ok = false;
            break;
        }

        const size_t untrimmed_samples =
            audio.size();

        const int current_pause_ms =
            pause_after_text_ms(sentence);

        const int leading_padding_ms =
            (final_audio.empty() || previous_pause_ms > 0)
                ? SENTENCE_LEADING_PADDING_MS
                : 0;

        const int trailing_padding_ms =
            current_pause_ms > 0
                ? SENTENCE_TRAILING_PADDING_MS
                : 0;

        audio = trim_sentence_silence(
            audio,
            OUTPUT_SAMPLE_RATE,
            leading_padding_ms,
            trailing_padding_ms);

        std::cout
            << "Sentence audio: "
            << untrimmed_samples
            << " -> "
            << audio.size()
            << " samples after silence trim\n";

        // A zero pause means the 600-frame safety split happened without any
        // punctuation at its cut point. The two chunks are concatenated as one
        // continuous phrase. Punctuation cuts retain their configured pause.
        if (!final_audio.empty() &&
            previous_pause_ms > 0)
        {
            const size_t gap_samples =
                static_cast<size_t>(
                    OUTPUT_SAMPLE_RATE *
                    previous_pause_ms /
                    1000);

            final_audio.insert(
                final_audio.end(),
                gap_samples,
                0.0f);
        }

        final_audio.insert(
            final_audio.end(),
            audio.begin(),
            audio.end());

        std::cout
            << "Pause after chunk = "
            << current_pause_ms
            << " ms"
            << (current_pause_ms == 0
                    ? " (unpunctuated continuous cut)\n"
                    : " (punctuation cut)\n");

        previous_pause_ms =
            current_pause_ms;
    }

    if (!tts_ok)
    {
        if (error)
            *error = "Matcha inference failed";
        return false;
    }

    if (cancelRequested && cancelRequested->load())
        return true;

    *audio16k = std::move(final_audio);
    return !audio16k->empty();
}

bool matchaRknnInitialize(
    const std::string &modelDir,
    std::string *error)
{
    std::vector<float> unused;
    return matchaRknnSynthesize(
        modelDir,
        std::string(),
        &unused,
        nullptr,
        error);
}
