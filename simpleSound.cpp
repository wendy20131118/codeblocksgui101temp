#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <vector>
#include <iostream>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846

int main() {
    const int duration = 2;              // 播放 2 秒
    const int numSamples = SAMPLE_RATE * duration;
    const int frequency = 440;           // A4

    // 1. 產生音訊資料 (16-bit PCM)
    std::vector<short> buffer(numSamples);
    for (int n = 0; n < numSamples; ++n) {
        double time = (double)n / SAMPLE_RATE;
        buffer[n] = (short)(32767 * sin(2 * PI * frequency * time));
    }

    // 2. 動態載入 winmm.dll
    HMODULE hWinmm = LoadLibraryA("winmm.dll");
    if (!hWinmm) {
        std::cerr << "LoadLibraryA(winmm.dll) failed\n";
        return 1;
    }

    // 3. 定義函式指標型別
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

    // 4. 取得函式位址
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

    // 5. 設定音訊格式
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.nAvgBytesPerSec = SAMPLE_RATE * 2;

    HWAVEOUT hWaveOut = nullptr;
    MMRESULT result = pWaveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "waveOutOpen failed: " << result << "\n";
        FreeLibrary(hWinmm);
        return 1;
    }

    // 6. 準備 Header
    WAVEHDR header = {};
    header.lpData = (LPSTR)buffer.data();
    header.dwBufferLength = numSamples * sizeof(short);

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

    // 7. 等待播放完畢
    Sleep(duration * 1000);

    // 8. 清理
    pWaveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    pWaveOutClose(hWaveOut);
    FreeLibrary(hWinmm);

    return 0;
}
