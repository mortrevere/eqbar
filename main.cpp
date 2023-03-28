/*This program demonstrates how to read in a raw
  data file and write it to the sound device to be played.
  The program uses the ALSA library.
  Use option -lasound on compile line.*/
 
#include </usr/include/alsa/asoundlib.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
#define PI 3.14159265
using namespace std;

static char *device = "default";    /*default playback device */

double rf() {
  return rand() / static_cast<float>(RAND_MAX);
}

double noiz = 0;

double nn() {
  double rnd = rf();
  int f = 2;
  noiz = (f*noiz+rnd)/(f+1);
  printf("rnd = %f, noiz = %f\n", rnd, noiz);
  return noiz*noiz*noiz*noiz;
}

int16_t sum_samples(int16_t s1, int16_t s2) {
  long s1_ = (long)s1;
  long s2_ = (long)s2;
  long sum = s1_ + s2_;
  long abs_sum = abs(s1_) + abs(s2_);
  int16_t out = 0;
  if(abs(sum) > 32000) { //we should clip
    //decide which way to clip
    if(s1_ < 0 && abs(s1_) > abs(s2_)) {
      out = -32000;
    } else {
      out = 32000;
    }
    out = sum/2;
    printf("clipping ! s1 = %d, s2 = %d, out = %d\n", s1, s2, out); 
  } else {
    out = (int16_t)sum;
  }
  return out;
}

void sum_sounds(int numSamples, int16_t* sound1, int16_t* sound2, int16_t* output) {
  for(int i = 0; i < numSamples; i++) {
    output[i] = sum_samples(sound1[i], sound2[i]);
  }
}

void render_kick(int numSamples, double base_freq, double p1, double p2, int16_t* samples) {
  for(unsigned int i = 0; i < numSamples; i++) {
    double t = (double)i;
    //base_freq = 120, p1 = 2

    //goes from 0 to 1
    double t_01 = t/(double)numSamples;

    //lin amp
    //double amp = 1.0f - t_01;
    //sqrt amp
    //double amp = 1.0f - sqrt(t_01);

    //square amp
    double amp = 1.0f - t_01*t_01;

    double click_amp = 1.0f - 50*t_01;
    if(click_amp < 0) {
      click_amp = 0;
    }
    int16_t click_signal = 3000*click_amp*sin((3.14159*base_freq)*(t/44100.0)*2*PI);
    //double freq = 1 - exp((2*t_01)-2);
    // freq drop for the kick, plus optional noise
    double freq = exp(-1*p1*t_01 + 0.00*amp*rf());
    // additional freq drop, deeper kick
    freq = freq - p2*((1-t_01)/2);

    //gen final kick body
    int16_t value = 10000*amp*sin(base_freq*freq*(t/44100.0)*2*PI);
    //if (i%1000 == 0)

    samples[i] = sum_samples(value, click_signal);
    //printf("i = %d, t_01 = %f, freq = %f, amp = %f, value = %d, click_signal = %d, out = %d\n", i, t_01, freq, amp, value, click_signal, samples[i]);
    //printf("value = %d, sample = %d\n", value, samples[i]);
 
    //samples[i+1] = value;
  }
}

void render_snare(int numSamples, double base_freq, double decay, double tone, int16_t* samples) {
  for(unsigned int i = 0; i < numSamples; i++) {
    double t = (double)i;
    double t_01 = t/(double)numSamples;

    double click_amp = 1.0f - 7*t_01;
    if(click_amp < 0) {
      click_amp = 0;
    }
    int16_t click_signal = 3000*click_amp*sin((3.14159*base_freq)*(t/44100.0)*2*PI);

    double amp = exp(-1*decay*t_01);
    double noise = rf()*amp;
    noise = 3000*pow(noise, 10);

    samples[i] = 1.2*sum_samples(noise, click_signal);
  }
}

void render_hat(int numSamples, double base_freq, double decay, double tone, int16_t* samples) {
  for(unsigned int i = 0; i < numSamples; i++) {
    double t = (double)i;
    double t_01 = t/(double)numSamples;

    double click_amp = 1.0f - 50*t_01;
    if(click_amp < 0) {
      click_amp = 0;
    }
    int16_t click_signal = 2000*click_amp*sin(6000*(t/44100.0)*2*PI);

    double amp = exp((-4+nn())*decay*t_01*t_01);
    double noise = rf()*amp;
    noise = 5000*(noise*noise*noise*noise*noise*noise*noise*noise*noise*noise*noise);
    //noise = 5000*noise;
    samples[i] = 1.2*sum_samples(click_signal, noise*amp);
    //samples[i] = 5000*nn()*amp;
  }
}

int main(int argc, char* argv[])
{
  int err, numSamples;
  snd_pcm_t *handle;
  snd_pcm_sframes_t frames;
  numSamples = atoi(argv[1]);
  double p1, p2, base_freq = 100;
  base_freq = atof(argv[2]);
  p1 = atof(argv[3]);
  p2 = atof(argv[4]);
  int sampleLength = numSamples;
  int16_t* samples_kick = (int16_t*) malloc((size_t) sampleLength*2);
  int16_t* samples_snare = (int16_t*) malloc((size_t) sampleLength*2);
  int16_t* samples_hat = (int16_t*) malloc((size_t) sampleLength*2);
  int16_t* mix_buffer = (int16_t*) malloc((size_t) sampleLength*2);
  //FILE *inFile = fopen(argv[2], "rb");
  //int numRead = fread(samples, 1, numSamples, inFile);
  //fclose(inFile);
  if ((err=snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
  if ((err = snd_pcm_set_params(handle,SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,1,44100, 1, 100000) ) < 0 ) {
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }

  render_snare(numSamples, base_freq, p1, p2, samples_snare);
  render_kick(numSamples, base_freq, p1, p2, samples_kick);
  render_hat(numSamples, base_freq, p1, p2, samples_hat);
  sum_sounds(numSamples, samples_kick, samples_snare, mix_buffer);

  for(int i = 0; i < 400; i++) {
    //snd_pcm_writei(handle, samples_hat, sampleLength);
    //K |!...|!...|!...|!...
    //H |..!!|..!.|..!.|..!.
    //S |....|!...|....|!..!
    //render_kick(numSamples, base_freq, p1, (1.0/(i%10)), samples_kick);
    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_kick, 3*sampleLength/4);
    snd_pcm_writei(handle, samples_kick, 3*sampleLength/4);
    //snd_pcm_writei(handle, 0, sampleLength/2);
    snd_pcm_writei(handle, samples_kick, sampleLength/2);
    /*snd_pcm_writei(handle, samples_hat, sampleLength/2);
    snd_pcm_writei(handle, samples_hat, sampleLength/2);
    //frames = snd_pcm_writei(handle, samples_hat, sampleLength);
    snd_pcm_writei(handle, mix_buffer, sampleLength);
    snd_pcm_writei(handle, samples_hat, sampleLength);

    snd_pcm_writei(handle, samples_kick, sampleLength);
    snd_pcm_writei(handle, samples_hat, sampleLength);
    //frames = snd_pcm_writei(handle, samples_hat, sampleLength);
    snd_pcm_writei(handle, mix_buffer, sampleLength);
    snd_pcm_writei(handle, samples_hat, 3*sampleLength/4);
    snd_pcm_writei(handle, samples_snare, sampleLength/4);*/
    //p1 += 1;
    //p2 += 1;
    //render(numSamples, p1, p2, samples);
    if (frames < 0)
      frames = snd_pcm_recover(handle, frames, 0);
    if (frames < 0) {
      printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
    }
  }
  snd_pcm_close(handle);
  return 0;
}
