#ifndef LTR_TRACKING__H
#define LTR_TRACKING__H

#include "ltr_gui.h"


class LtrTracking : public QObject
{
  Q_OBJECT
 public:
  LtrTracking(const Ui::LinuxtrackMainForm &ui);
  ~LtrTracking();
  void refresh();
 signals:
  void customSectionChanged();
 private slots:
  void axisChanged(int axis, int elem);
  void on_FilterSlider_valueChanged(int value);
  void ffChanged(float f);
  void on_Profiles_currentIndexChanged(const QString &text);
  void on_CreateNewProfile_pressed();
  
  void on_PitchEnable_stateChanged(int state);
  void on_RollEnable_stateChanged(int state);
  void on_YawEnable_stateChanged(int state);
  void on_XEnable_stateChanged(int state);
  void on_YEnable_stateChanged(int state);
  void on_ZEnable_stateChanged(int state);  
  void on_PitchUpSpin_valueChanged(double d);
  void on_PitchDownSpin_valueChanged(double d);
  void on_YawLeftSpin_valueChanged(double d);
  void on_YawRightSpin_valueChanged(double d);
  void on_TiltLeftSpin_valueChanged(double d);
  void on_TiltRightSpin_valueChanged(double d);
  void on_MoveLeftSpin_valueChanged(double d);
  void on_MoveRightSpin_valueChanged(double d);
  void on_MoveUpSpin_valueChanged(double d);
  void on_MoveDownSpin_valueChanged(double d);
  void on_MoveBackSpin_valueChanged(double d);
  void on_MoveForthSpin_valueChanged(double d);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  bool initializing;
};

#endif
