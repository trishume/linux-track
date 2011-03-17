#ifndef LTR_AXIS__H
#define LTR_AXIS__H

#include <QWidget>
#include <axis.h>

class AppProfile;

typedef enum{
  ENABLED, LFACTOR, RFACTOR, LCURV, RCURV, DZONE, LIMITS, RELOAD
}AxisElem_t;

class LtrAxis : public QWidget{
  Q_OBJECT
 public:
  LtrAxis(const AppProfile* parent, enum axis_t a);
  ~LtrAxis();

  void reload();
  bool changeEnabled(bool enabled);
  bool changeLFactor(float val);
  bool changeRFactor(float val);
  bool changeLCurv(float val);
  bool changeRCurv(float val);
  bool changeDZone(float val);
  bool changeLimits(float val);
  
  bool getEnabled();
  float getLFactor();
  float getRFactor();
  float getLCurv();
  float getRCurv();
  float getDZone();
  float getLimits();
  float getValue(float val);
  
  bool isSymetrical();
 signals:
  void axisChanged(AxisElem_t what);
 private:
  LtrAxis();
  LtrAxis(const LtrAxis&); 
  const AppProfile *parent;
  enum axis_t axis;
};

#endif

