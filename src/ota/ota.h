#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

namespace saltlevel {

  // 0 = English, 1 = French
  struct Config {
    float   fullDistanceCm;   // tank FULL at this distance (hardware min)
    float   emptyDistanceCm;  // tank EMPTY at this distance (max depth)
    float   warnDistanceCm;   // Bark warning distance
    char    barkKey[128];     // Bark device key
    uint8_t language;         // 0 = EN, 1 = FR
    bool    barkEnabled;      // runtime Bark on/off
  };

  typedef float (*DistanceCallback)();
  typedef void  (*PublishCallback)(float);

  class OTA {
    public:
      void setup();
      void loop();

      void setDistanceCallback(DistanceCallback cb);
      void setPublishCallback(PublishCallback cb);
      void setConfig(Config* cfg);
  };

}

#endif
