#ifndef LTR_GUI__H
#define LTR_GUI__H

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif
 
#include <QCloseEvent>
#include <linuxtrack.h>

#include "ui_ltr.h"

class LtrGuiForm;
class LtrDevHelp;
class ModelEdit;
//class LtrTracking;
class LogView;
//class ScpForm;
class QSettings;
class HelpViewer;
class PluginInstall;
class DeviceSetup;
class ProfileSelector;

class LinuxtrackGui : public QWidget
{
  Q_OBJECT
 public:
  LinuxtrackGui(QWidget *parent = 0);
  ~LinuxtrackGui();
 protected:
  void closeEvent(QCloseEvent *event);
 private slots:
  void on_QuitButton_pressed();
  //void on_EditSCButton_pressed();
  void on_XplanePluginButton_pressed();
  void on_SaveButton_pressed();
  void on_ViewLogButton_pressed();
  void on_DefaultsButton_pressed();
  void on_DiscardChangesButton_pressed();
  void on_HelpButton_pressed();
  void on_LtrTab_currentChanged(int index);
  void trackerStateHandler(ltr_state_type current_state);
  void on_LegacyPose_stateChanged(int state);
 private:
  Ui::LinuxtrackMainForm ui;
  LtrGuiForm *showWindow;
  LtrDevHelp *helper;
  DeviceSetup *ds;
  ModelEdit *me;
  //LtrTracking *track;
  //ScpForm *sc;
  LogView *lv;
  PluginInstall *pi;
  ProfileSelector *ps;
  bool initialized;
  QSettings *gui_settings;
  void rereadPrefs();
};


#endif
