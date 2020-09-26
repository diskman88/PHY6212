#ifndef __DEV_AUDIO_H__
#define __DEV_AUDIO_H__

#include "hal/audio_impl.h"

#define audio_open(name) device_open(name)
#define audio_open_id(name, id) device_open_id(name, id)
#define audio_close(dev) device_close(dev)

int audio_send(dev_t *dev, const void *data, uint32_t size);
int audio_recv(dev_t *dev, void *data, uint32_t size, unsigned int timeout_ms);
void audio_set_event(dev_t *dev, void (*event)(dev_t *dev, int event_id, void *priv), void *priv);
int audio_get_len(dev_t *dev, int type);
int audio_config(dev_t *dev, audio_config_t *config);
void audio_config_default(audio_config_t *config);
int audio_set_gain(dev_t *dev, int l, int r);
int audio_start(dev_t *dev);
int audio_pause(dev_t *dev);


#endif
