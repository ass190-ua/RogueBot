#include "Game.hpp"
#include <cmath>
#include <cstdlib>

// Generador de ondas procedurales (Sintetizador)
Sound Game::generateSound(int type) {
    const int sampleRate = 44100;
    
    // CAMBIO: 60 segundos exactos.
    const int durationFrames = (type == SND_AMBIENT) ? 44100 * 60 : 
                               (type == SND_WIN || type == SND_LOOSE) ? 44100 * 4 : 
                               44100 / 2;
    
    Wave wave;
    wave.frameCount = durationFrames;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = malloc(wave.frameCount * wave.sampleSize / 8);
    
    short *data = (short *)wave.data;
    
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / sampleRate; // Tiempo en segundos
        float val = 0.0f;
        
        switch (type) {
            case SND_HIT: // Ruido blanco corto + Onda cuadrada baja
                if (t < 0.1f) val = ((float)GetRandomValue(-100, 100) / 100.0f) * (1.0f - t/0.1f);
                else val = (sinf(2 * PI * 100 * t) > 0 ? 0.5f : -0.5f) * (1.0f - t);
                break;
                
            case SND_EXPLOSION: // Ruido puro con decay largo
                val = ((float)GetRandomValue(-100, 100) / 100.0f);
                val *= (1.0f - (float)i / wave.frameCount); // Fade out lineal
                break;
                
            case SND_PICKUP: // Onda cuadrada aguda (Coin)
                // Frecuencia sube de 800 a 1200
                {
                    float freq = 800.0f + (t * 4000.0f); 
                    val = (sinf(2 * PI * freq * t) > 0 ? 0.3f : -0.3f);
                    val *= (1.0f - t*4.0f); // Muy corto
                }
                break;
                
            case SND_POWERUP: // Onda triangular subiendo (Powerup)
                {
                    float freq = 300.0f + (t * 1000.0f);
                    val = asinf(sinf(2 * PI * freq * t)) * 0.5f; 
                }
                break;
                
            case SND_HURT: // Sierra descendente (Auch)
                {
                    float freq = 200.0f - (t * 400.0f);
                    if (freq < 50) freq = 50;
                    val = (float)(fmod(t * freq, 1.0) * 2.0 - 1.0) * 0.5f;
                }
                break;

            case SND_AMBIENT: 
                {
                    // --- TEMA: "CYBER COMBAT LOOP" (60s Acción Pura) ---
                    // Sin intro suave. Directo al grano.
                    
                    float bpm = 120.0f;
                    float beatLen = 60.0f / bpm; // 0.5 segundos exactos
                    
                    // Variables de tiempo
                    float measurePos = t / (beatLen * 4.0f); 
                    int measure = (int)measurePos;           
                    float beatPos = fmod(t, beatLen);        
                    float subBeat = fmod(t, beatLen / 4.0f); 
                    
                    // --- 1. KICK (EL MOTOR) ---
                    // Suena DESDE EL PRINCIPIO. Golpe seco y potente.
                    float kick = 0.0f;
                    if (beatPos < 0.2f) {
                        // Frecuencia estable para que pegue fuerte
                        float kFreq = 100.0f - (beatPos * 400.0f);
                        if (kFreq < 40.0f) kFreq = 40.0f;
                        kick = sinf(2 * PI * kFreq * t);
                        kick *= expf(-15.0f * beatPos); 
                        kick *= 0.9f; // Volumen alto
                    }

                    // --- 2. BASSLINE (LA ENERGÍA) ---
                    // Bajo "Rolling" constante desde el segundo 0.
                    float bassFreq = 55.0f; // La (A1)
                    
                    // Pequeña variación de nota cada 8 compases (16s) para no aburrir
                    if ((measure / 8) % 2 == 1) bassFreq = 43.65f; // Fa (F1)

                    // Onda de Sierra filtrada (agresiva pero controlada)
                    float saw = (fmod(t * bassFreq, 1.0f) * 2.0f) - 1.0f;
                    float bass = (saw * 0.4f) + (sinf(2 * PI * bassFreq * t) * 0.6f);
                    
                    // SIDECHAIN: El bajo se "agacha" cuando pega el Kick
                    float sidechain = fmod(t, beatLen) / beatLen; 
                    sidechain = powf(sidechain, 0.5f); // Curva rápida
                    bass *= sidechain * 0.6f;

                    // --- 3. HI-HATS (VELOCIDAD) ---
                    // Ritmo de semicorcheas constante "tik-tik-tik-tik"
                    float noise = ((float)GetRandomValue(-100, 100) / 100.0f);
                    // Filtro High-Pass simulado (solo frecuencias altas)
                    noise *= sinf(2 * PI * 6000.0f * t);
                    
                    float hat = noise * expf(-80.0f * subBeat) * 0.15f;
                    // Acento en el "off-beat" (el "y" del 1 y 2...)
                    if (fmod(t, beatLen) > beatLen/2.0f) hat *= 1.8f; 

                    // --- 4. ARPEGIO (LA MELODÍA) ---
                    // Entra y sale para dar variedad, pero el ritmo de fondo sigue.
                    float arp = 0.0f;
                    
                    // La melodía suena en los segundos 0-15 y 30-45. 
                    // Deja huecos de "solo ritmo" en 15-30 y 45-60.
                    bool playArp = ((int)(t / 16.0f) % 2 == 0); 
                    
                    if (playArp) {
                        int noteIdx = (int)(t * 8.0f); // 8 notas/segundo
                        // Patrón simple y pegadizo
                        float scale[] = { 1.0f, 1.0f, 1.5f, 1.2f, 2.0f, 1.5f, 1.2f, 1.5f };
                        int idx = noteIdx % 8;
                        float arpFreq = bassFreq * 4.0f * scale[idx];
                        
                        // Onda "Cristal"
                        arp = sinf(2 * PI * arpFreq * t);
                        // Delay
                        arp += sinf(2 * PI * arpFreq * (t - 0.25f)) * 0.4f;
                        
                        float env = fmod(t, 0.125f) / 0.125f;
                        arp *= (1.0f - env) * 0.15f; // Volumen sutil
                    }

                    // --- MEZCLA FINAL ---
                    val = kick + bass + hat + arp;
                    
                    // Compresor/Limitador
                    if (val > 0.9f) val = 0.9f;
                    if (val < -0.9f) val = -0.9f;
                    
                    // Fade micro-rápido en los bordes del bucle (0.05s) para evitar "clicks"
                    if (t < 0.05f) val *= (t / 0.05f);
                    if (t > 59.95f) val *= (60.0f - t) / 0.05f;
                }
                break;
            case SND_DASH: // Ruido rosa/aire rápido
                {
                    // Ruido aleatorio suave
                    float noise = ((float)GetRandomValue(-100, 100) / 100.0f);
                    // Volumen baja muy rápido (0.2 segundos)
                    val = noise * 0.4f * (1.0f - t/0.2f); 
                    if (val < 0) val = 0;
                }
                break;

            case SND_WIN: // Acorde mayor arpegiado
                {
                    float note = 440.0f; // La
                    if (t > 0.2) note = 554.37f; // Do#
                    if (t > 0.4) note = 659.25f; // Mi
                    if (t > 0.6) note = 880.0f;  // La (8va)
                    val = sinf(2 * PI * note * t) * 0.5f;
                    val *= (1.0f - t/2.0f);
                }
                break;

            case SND_LOOSE: // Tono descendente triste
                {
                    float freq = 300.0f * (1.0f - t/2.0f);
                    val = (sinf(2 * PI * freq * t) > 0 ? 0.4f : -0.4f);
                }
                break;
        }
        
        data[i] = (short)(val * 32000.0f);
    }
    
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}