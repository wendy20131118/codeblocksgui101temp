#include <windows.h>
#include <mmsystem.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846

// =========================
// 音階規則
// 1   = C   = Do
// 2   = D   = Re
// 3   = E   = Mi
// 4   = F   = Fa
// 5   = G   = So
// 6   = A   = La
// 7   = B   = Ti
//
// 黑鍵用 .5
// 1.5 = C#
// 2.5 = D#
// 4.5 = F#
// 5.5 = G#
// 6.5 = A#
//
// 比中央C更低用負號
// -1 = 低音 C
// -2 = 低音 D
//
// 0 = 休止
//
// 8~14 自動視為高一個八度
// 8 = 高音 C
// 9 = 高音 D
// =========================

// Hänschen klein（C大調常見版）
float masterSong[] = {
    // Hänschen klein ging allein
    5, 3, 3, 4, 2, 2, 1, 2, 3, 4, 5, 5, 5,
    0,

    // in die weite Welt hinein
    5, 3, 3, 4, 2, 2, 1, 3, 5, 5, 3, 3, 2, 2, 2,
    0,

    // Stock und Hut steht ihm gut
    5, 6, 5, 4, 3, 1, 5, 6, 5, 4, 3, 2, 1,
    0,

    // ist gar wohlgemut
    3, 5, 5, 3, 3, 2, 2, 1
};

float masterBeat[] = {
    // Hänschen klein ging allein
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    0.5f,

    // in die weite Welt hinein
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    0.5f,

    // Stock und Hut steht ihm gut
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    0.5f,

    // ist gar wohlgemut
    1, 1, 1, 1, 1, 1, 1, 4
};

int degreeToSemitoneInOctave(int degree) {
    switch (degree) {
        case 1: return 0;   // C
        case 2: return 2;   // D
        case 3: return 4;   // E
        case 4: return 5;   // F
        case 5: return 7;   // G
        case 6: return 9;   // A
        case 7: return 11;  // B
        default: return 0;
    }
}

double noteToFrequency(float note) {
    if (note == 0.0f) return 0.0;

    bool lower = (note < 0.0f);
    double a = std::fabs(note);

    int whole = (int)std::floor(a);
    double frac = a - whole;

    bool sharp = std::fabs(frac - 0.5) < 1e-6;

    int octaveGroup = (whole - 1) / 7;
    int degree = ((whole - 1) % 7) + 1;

    int semitone = degreeToSemitoneInOctave(degree);
    if (sharp) semitone += 1;

    int totalOffsetFromC4 = 0;
    if (!lower) {
        totalOffsetFromC4 = octaveGroup * 12 + semitone;
    } else {
        totalOffsetFromC4 = -(octaveGroup + 1) * 12 + semitone;
    }

    const double C4 = 261.625565; // 中央C
    return C4 * std::pow(2.0, totalOffsetFromC4 / 12.0);
}

void appendSilence(std::vector<short>& buffer, double seconds) {
    int samples = (int)(seconds * SAMPLE_RATE);
    buffer.insert(buffer.end(), samples, 0);
}

void appendSine(std::vector<short>& buffer, double freq, double seconds, double volume) {
    int samples = (int)(seconds * SAMPLE_RATE);
    int fadeSamples = std::min(256, std::max(8, samples / 16));

    for (int i = 0; i < samples; ++i) {
        double t = (double)i / SAMPLE_RATE;

        double env = 1.0;
        if (i < fadeSamples) {
            env = (double)i / fadeSamples;
        } else if (i >= samples - fadeSamples) {
            env = (double)(samples - 1 - i) / fadeSamples;
            if (env < 0.0) env = 0.0;
        }

        double s = std::sin(2.0 * PI * freq * t) * env;
        short v = (short)(32767.0 * volume * s);
        buffer.push_back(v);
    }
}

int main() {
    const double BPM = 132.0;
    const double beatSec = 60.0 / BPM;
    const double volume = 0.25;

    const int songCount = sizeof(masterSong) / sizeof(masterSong[0]);
    const int beatCount = sizeof(masterBeat) / sizeof(masterBeat[0]);

    if (songCount != beatCount) {
        std::cerr << "masterSong[] and masterBeat[] length mismatch\n";
        return 1;
    }

    std::vector<short> buffer;

    for (int i = 0; i < songCount; ++i) {
        float note = masterSong[i];
        float beats = masterBeat[i];
        double seconds = beats * beatSec;

        if (note == 0.0f) {
            appendSilence(buffer, seconds);
        } else {
            double freq = noteToFrequency(note);
            appendSine(buffer, freq, seconds, volume);
            appendSilence(buffer, 0.01);
        }
    }

    HMODULE hWinmm = LoadLibraryA("winmm.dll");
    if (!hWinmm) {
        std::cerr << "LoadLibraryA(winmm.dll) failed\n";
        return 1;
    }

    typedef MMRESULT (WINAPI* WaveOutOpenFn)(
        LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
    typedef MMRESULT (WINAPI* WaveOutPrepareHeaderFn)(
        HWAVEOUT, LPWAVEHDR, UINT);
    typedef MMRESULT (WINAPI* WaveOutWriteFn)(
        HWAVEOUT, LPWAVEHDR, UINT);
    typedef MMRESULT (WINAPI* WaveOutUnprepareHeaderFn)(
        HWAVEOUT, LPWAVEHDR, UINT);
    typedef MMRESULT (WINAPI* WaveOutCloseFn)(
        HWAVEOUT);

    WaveOutOpenFn pWaveOutOpen =
        (WaveOutOpenFn)GetProcAddress(hWinmm, "waveOutOpen");
    WaveOutPrepareHeaderFn pWaveOutPrepareHeader =
        (WaveOutPrepareHeaderFn)GetProcAddress(hWinmm, "waveOutPrepareHeader");
    WaveOutWriteFn pWaveOutWrite =
        (WaveOutWriteFn)GetProcAddress(hWinmm, "waveOutWrite");
    WaveOutUnprepareHeaderFn pWaveOutUnprepareHeader =
        (WaveOutUnprepareHeaderFn)GetProcAddress(hWinmm, "waveOutUnprepareHeader");
    WaveOutCloseFn pWaveOutClose =
        (WaveOutCloseFn)GetProcAddress(hWinmm, "waveOutClose");

    if (!pWaveOutOpen || !pWaveOutPrepareHeader || !pWaveOutWrite ||
        !pWaveOutUnprepareHeader || !pWaveOutClose) {
        std::cerr << "GetProcAddress failed\n";
        FreeLibrary(hWinmm);
        return 1;
    }

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HWAVEOUT hWaveOut = nullptr;
    MMRESULT result = pWaveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveOutOpen failed: " << result << "\n";
        FreeLibrary(hWinmm);
        return 1;
    }

    WAVEHDR header = {};
    header.lpData = (LPSTR)buffer.data();
    header.dwBufferLength = (DWORD)(buffer.size() * sizeof(short));

    result = pWaveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveOutPrepareHeader failed: " << result << "\n";
        pWaveOutClose(hWaveOut);
        FreeLibrary(hWinmm);
        return 1;
    }

    result = pWaveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveOutWrite failed: " << result << "\n";
        pWaveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        pWaveOutClose(hWaveOut);
        FreeLibrary(hWinmm);
        return 1;
    }

    while ((header.dwFlags & WHDR_DONE) == 0) {
        Sleep(10);
    }

    pWaveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    pWaveOutClose(hWaveOut);
    FreeLibrary(hWinmm);

    return 0;
}
