#ifndef __GRILLPID_H__
#define __GRILLPID_H__

#include <wiring.h>

#define TEMP_PIT    0
#define TEMP_FOOD1  1
#define TEMP_FOOD2  2
#define TEMP_AMB    3
#define TEMP_COUNT  4

#define PROBE_NAME_SIZE 13

// Probe types used in probeType config
#define PROBETYPE_DISABLED 0  // do not read
#define PROBETYPE_INTERNAL 1  // read via analogRead()
#define PROBETYPE_RF12     2  // RFM12B wireless

#define STEINHART_COUNT 4

struct __eeprom_probe
{
  char name[PROBE_NAME_SIZE];
  char tempOffset;
  int alarmHigh;
  int alarmLow;
  boolean alHighEnabled;
  boolean alLowEnabled;
  unsigned char probeType;
  float steinhart[STEINHART_COUNT];  // The last one is actually Rknown
};

class ProbeAlarm
{
private:
  int _high;
  int _low;
public:
  ProbeAlarm(void) 
    // : _high(0), _low(0), Status(0)
    {};

  // High and Low thresholds
  // Combination of constants below  
  unsigned char Status;   
  // Check value against High/Low
  void updateStatus(int value);
  void setHigh(int value);
  void setLow(int value);
  int getHigh(void) const { return _high; };
  int getLow(void) const { return _low; };
  // Any Enabled and ringing but not silenced
  boolean getActionNeeded(void) const;
  
/*
  ProbeAlarm ALARM constants used in ProbeAlarm::Status
  ENABLED: Whether the check is enabled.  A disabled alarm
           will not ring
  RINGING: The alarm is "going off", in that the check has 
           reached or passed the high/low bound
  SILENCED: The alarm has failed the check and is ringing,
           but the user has requested the alarm stop notifying
  */          
  static const unsigned char NONE         = 0x00;
  static const unsigned char HIGH_ENABLED = 0x01;
  static const unsigned char LOW_ENABLED  = 0x02;
  static const unsigned char ANY_ENABLED  = (HIGH_ENABLED | LOW_ENABLED);
  static const unsigned char HIGH_RINGING = 0x04;
  static const unsigned char LOW_RINGING  = 0x08;
  static const unsigned char ANY_RINGING  = (HIGH_RINGING | LOW_RINGING);
  static const unsigned char HIGH_SILENCED = 0x10;
  static const unsigned char LOW_SILENCED  = 0x20;
  static const unsigned char HIGH_NOTIFIED = 0x40;
  static const unsigned char LOW_NOTIFIED  = 0x80;
  static const unsigned char HIGH_MASK    = (HIGH_ENABLED | HIGH_RINGING | HIGH_SILENCED | HIGH_NOTIFIED);
  static const unsigned char LOW_MASK     = (LOW_ENABLED | LOW_RINGING | LOW_SILENCED | LOW_NOTIFIED);
};

class TempProbe
{
private:
  const unsigned char _pin; 
  unsigned int _accumulator;
  
public:
  TempProbe(const unsigned char pin) : 
    _pin(pin), TemperatureAvg(-1.0f)
    // Temperature(0), Offset(0)
    {};
  
  // Last averaged temperature reading
  float Temperature;
  // Temperature moving average 
  float TemperatureAvg;
  // Offset (in degrees) applied when calculating temperature
  char Offset;
  // Do the duty of reading ADC
  // num is the number of times the ADC has been read already this cycle
  void readTemp(unsigned char num);
  // Convert ADC to Temperature
  void calcTemp(void);
  // Steinhart coefficients
  float Steinhart[STEINHART_COUNT];
  // Probe Type
  unsigned char ProbeType;  
  
  ProbeAlarm Alarms;
};

// Indexes into the pid array
#define PIDB 0
#define PIDP 1
#define PIDI 2
#define PIDD 3

class GrillPid
{
private:
  const unsigned char _blowerPin;
  unsigned char _fanSpeed;
  unsigned long _lastTempRead;
  unsigned char _accumulatedCount;
  boolean _pitTemperatureReached;
  int _setPoint;
  boolean _manualFanMode;
  // Counter used for "long PWM" mode
  unsigned char _longPwmTmr;
  
  void calcFanSpeed(TempProbe *controlProbe);
  void commitFanSpeed(void);
public:
  float _pidErrorSum;
  GrillPid(const unsigned char blowerPin) : 
    _blowerPin(blowerPin), FanSpeedAvg(-1.0f)
    //_lastTempRead(0), _accumulatedCount(0), 
    //_pitTemperatureReached(false), FanSpeed(0), _fanSpeedPwm(0),
    //_pidErrorSum(0.0f), _longPwmTmr(0), _manualFanMode(false),
    // MaxFanSpeed(0)
    {};
  
  TempProbe *Probes[TEMP_COUNT];
  
  /* Configuration */
  int getSetPoint(void) const { return _setPoint; };
  void setSetPoint(int value); 
  // The number of degrees the temperature drops before automatic lidopen mode
  unsigned char LidOpenOffset;
  // The ammount of time to turn off the blower when the lid is open 
  unsigned int LidOpenDuration;
  // The PID constants
  float Pid[4];
  // The maximum fan speed that will be used in automatic mode
  unsigned char MaxFanSpeed;
  
  /* Runtime Data */
  // Current fan speed in percent, setting this will put the fan into manual mode
  unsigned char getFanSpeed() const { return _fanSpeed; };
  void setFanSpeed(int value);
  boolean getManualFanMode(void) const { return _manualFanMode; };
  // Fan speed moving average
  float FanSpeedAvg;
  // Seconds remaining in the lid open countdown
  unsigned int LidOpenResumeCountdown;
  // true if any probe has a non-zero temperature
  boolean isAnyFoodProbeActive(void) const;
  
  // Call this in loop()
  boolean doWork(void);
  void resetLidOpenResumeCountdown(void);
};

#endif /* __GRILLPID_H__ */
