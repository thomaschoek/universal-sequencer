## Precompute waveforms for common frequencies

For instance, if the synthesizer is tuned normally with A4 = 440 Hz, at construction time, given a samplerate and this tuning, precompute 1-second length waveforms from A0 through A7 for that samplerate and tuning. This allows for quick retrieval of waveforms during playback without needing to generate them on-the-fly.
