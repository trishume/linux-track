#include <iostream>
#include "ltr_gui.h"
#include "ltr_gui_prefs.h"
#include "tir_driver_prefs.h"
#include "ltr_gui_prefs.h"
#include "tir_prefs.h"
#include "pathconfig.h"
#include "dyn_load.h"
#include <QFile>
#include <QMessageBox>

static QString currentId = QString("None");
static QString currentSection = QString();
static int tirType = 0;
bool TirPrefs::firmwareOK = false;
bool TirPrefs::permsOK = false;

typedef int (*probe_tir_fun_t)(bool *have_firmware, bool *have_permissions);
static probe_tir_fun_t probe_tir_fun = NULL;
static lib_fun_def_t functions[] = {
  {(char *)"ltr_int_tir_found", (void*) &probe_tir_fun},
  {NULL, NULL}
};


static int probeTir(bool &fwOK, bool &permOK)
{
  void *libhandle = NULL;
  int res = 0;
  if((libhandle = ltr_int_load_library((char *)"libtir", functions)) != NULL){
    res = probe_tir_fun(&fwOK, &permOK);
    ltr_int_unload_library(libhandle, functions);
  }
  return res;
}


void TirPrefs::Connect()
{
  QObject::connect(gui.TirThreshold, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirThreshold_valueChanged(int)));
  QObject::connect(gui.TirMinBlob, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirMinBlob_valueChanged(int)));
  QObject::connect(gui.TirMaxBlob, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirMaxBlob_valueChanged(int)));
  QObject::connect(gui.TirStatusBright, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirStatusBright_valueChanged(int)));
  QObject::connect(gui.TirIrBright, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirIrBright_valueChanged(int)));
  QObject::connect(gui.TirSignalizeStatus, SIGNAL(stateChanged(int)),
    this, SLOT(on_TirSignalizeStatus_stateChanged(int)));
  QObject::connect(gui.TirInstallFirmware, SIGNAL(pressed()),
    this, SLOT(on_TirInstallFirmware_pressed()));
}

TirPrefs::TirPrefs(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  Connect();
  dlfw = NULL;
}

TirPrefs::~TirPrefs()
{
  if(dlfw != NULL){
    dlfw ->close();
    delete dlfw;
  }
}

bool TirPrefs::Activate(const QString &ID, bool init)
{
  initializing = init;
  QString sec;
  if(PREF.getFirstDeviceSection(QString("Tir"), sec)){
    if(!initializing) PREF.activateDevice(sec);
    currentSection = sec;
  }else{
    sec = "TrackIR";
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Tir");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(140));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(4));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(230));
      PREF.addKeyVal(sec, (char *)"Status-led-brightness", QString::number(0));
      PREF.addKeyVal(sec, (char *)"Ir-led-brightness", QString::number(7));
      PREF.addKeyVal(sec, (char *)"Status-signals", (char *)"on");
      PREF.activateDevice(sec);
      currentSection = sec;
    }else{
      initializing = false;
      return false;
    }
  }
  ltr_int_tir_init_prefs();
  currentId = ID;
  gui.TirThreshold->setValue(ltr_int_tir_get_threshold());
  gui.TirMaxBlob->setValue(ltr_int_tir_get_max_blob());
  gui.TirMinBlob->setValue(ltr_int_tir_get_min_blob());
  gui.TirIrBright->setValue(ltr_int_tir_get_ir_brightness());
  gui.TirStatusBright->setValue(ltr_int_tir_get_status_brightness());
  Qt::CheckState state = (ltr_int_tir_get_status_indication()) ? 
                          Qt::Checked : Qt::Unchecked;
  gui.TirSignalizeStatus->setCheckState(state);
  if(firmwareOK){
    if(tirType < 4){
      gui.TirFwLabel->setText("Firmware not needed!");
    }else{
      gui.TirFwLabel->setText("Firmware found!");
    }
    gui.TirInstallFirmware->setDisabled(true);
  }else{
    gui.TirFwLabel->setText("Firmware not found - TrackIr will not work!");
  }
  if(tirType < 5){
    gui.TirIrBright->setDisabled(true);
    gui.TirIrBright->setHidden(true);
    gui.TirStatusBright->setDisabled(true);
    gui.TirStatusBright->setHidden(true);
    gui.StatusBrightLabel->setHidden(true);
    gui.IRBrightLabel->setHidden(true);
  }
  initializing = false;
  return true;
}



bool TirPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool tir_selected = false;
  
  tirType = probeTir(firmwareOK, permsOK);
  if(!permsOK){
    QMessageBox::warning(NULL, QString("TrackIR permissions problem"), 
        QString("TrackIR device was found, but you don't have permissions to access it.\n \
Please install the file 51-TIR.rules to the udev rules directory\n\
(consult help and your distro documentation for details).\n\
You are going to need administrator privileges to do that.")
                        );
    return false;
  }
  if(tirType == 0){
    return res;
  }
  
  if(PREF.getActiveDevice(dt,id)){
    if(dt == TIR){
      tir_selected = true;
    }
  }
  
  PrefsLink *pl = new PrefsLink(TIR, (char *)"Tir");
  QVariant v;
  v.setValue(*pl);
  combo.addItem((char *)"TrackIR", v);
  if(tir_selected){
    combo.setCurrentIndex(combo.count() - 1);
    res = true;
  }
  return res;
}

void TirPrefs::on_TirThreshold_valueChanged(int i)
{
  if(!initializing) ltr_int_tir_set_threshold(i);
}

void TirPrefs::on_TirMinBlob_valueChanged(int i)
{
  if(!initializing) ltr_int_tir_set_min_blob(i);
}

void TirPrefs::on_TirMaxBlob_valueChanged(int i)
{
  if(!initializing) ltr_int_tir_set_max_blob(i);
}

void TirPrefs::on_TirStatusBright_valueChanged(int i)
{
  if(!initializing) ltr_int_tir_set_status_brightness(i);
}

void TirPrefs::on_TirIrBright_valueChanged(int i)
{
  if(!initializing) ltr_int_tir_set_ir_brightness(i);
}

void TirPrefs::on_TirSignalizeStatus_stateChanged(int state)
{
  if(!initializing) ltr_int_tir_set_status_indication(state == Qt::Checked);
}

void TirPrefs::on_TirFirmwareDLFinished(bool state)
{
  if(state){
    dlfw->hide();
    probeTir(firmwareOK, permsOK);    
    if(firmwareOK){
      gui.TirFwLabel->setText("Firmware found!");
      gui.TirInstallFirmware->setDisabled(true);
    }else{
      gui.TirFwLabel->setText("Firmware not found - TrackIr will not work!");
    }
  }
}

void TirPrefs::on_TirInstallFirmware_pressed()
{
  if(dlfw == NULL){
    dlfw = new dlfwGui();
    QObject::connect(dlfw, SIGNAL(finished(bool)),
      this, SLOT(on_TirFirmwareDLFinished(bool)));
  }
  dlfw->show();
}
