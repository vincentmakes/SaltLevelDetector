#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

namespace saltlevel {

  // Language enumeration
  enum class Language : uint8_t {
    ENGLISH = 0,
    FRENCH = 1
  };

  // Configuration structure
  struct Config {
    float    fullDistanceCm;      // Tank FULL at this distance (hardware min)
    float    emptyDistanceCm;     // Tank EMPTY at this distance (max depth)
    float    warnDistanceCm;      // Bark warning distance
    char     barkKey[128];        // Bark device key
    char     otaPassword[64];     // OTA update password
    char     ntfyTopic[64];       // ntfy topic name
    Language language;            // UI language
    bool     barkEnabled;         // Runtime Bark on/off
    bool     ntfyEnabled;         // Runtime ntfy on/off
  };

  // Callback types
  typedef float (*DistanceCallback)();
  typedef bool  (*PublishCallback)(float);

  class OTA {
    public:
      void setup();
      void loop();

      void setDistanceCallback(DistanceCallback cb);
      void setPublishCallback(PublishCallback cb);
      void setConfig(Config* cfg);
      
      // Validation
      static bool validateConfig(const Config* cfg);
  };

}

#endif // OTA_H