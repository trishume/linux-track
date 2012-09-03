#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ui_m_wcft_setup.h"
#include "macwebcamft_prefs.h"
#include "macwebcam_info.h"
#include "ltr_gui_prefs.h"
#include "wc_driver_prefs.h"
#include <utils.h>

static QString currentId = QString("None");

/*
void WebcamFtPrefs::Connect()
{
  QObject::connect(ui.WebcamFtResolutionsMac, SIGNAL(activated(int)),
    this, SLOT(on_WebcamResolutions_activated(int)));
  QObject::connect(ui.FindCascadeMac, SIGNAL(pressed()),
    this, SLOT(on_FindCascade_pressed()));
  QObject::connect(ui.CascadePathMac, SIGNAL(editingFinished()),
    this, SLOT(on_CascadePath_editingFinished()));
  QObject::connect(ui.ExpFilterFactorMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_ExpFilterFactor_valueChanged(int)));
  QObject::connect(ui.OptimLevelMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_OptimLevel_valueChanged(int)));
}
*/

WebcamFtPrefs::WebcamFtPrefs(const QString &dev_id, QWidget parent) : QWidget(parent), id(dev_id)
{
//  Connect();
  ui.setupUi(this);
  prefInit = true;
  Activate(id, prefInit);
  prefInit = false;
}

WebcamFtPrefs::~WebcamFtPrefs()
{
  ltr_int_wc_close_prefs();
}

static WebcamInfo *wc_info = NULL;

void WebcamFtPrefs::on_WebcamFtResolutionsMac_activated(int index)
{
  (void) index;
  QString res;
  res = ui.WebcamFtResolutionsMac->currentText();
  
  int x,y;
  WebcamInfo::decodeRes(res, x, y);
  if(!initializing) ltr_int_wc_set_resolution(x, y);
}

bool WebcamFtPrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString("Webcam-face"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = "Webcam-face";
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Webcam-face");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      PREF.activateDevice(sec);
    }else{
      initializing = false;
      return false;
    }
  }
  if(!ltr_int_wc_init_prefs()){
      initializing = false;
    return false;
  }
  currentId = ID;
  ui.WebcamFtResolutionsMac->clear();
  if((currentId != "None") && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new WebcamInfo(currentId);
    ui.WebcamFtResolutionsMac->clear();
    ui.WebcamFtResolutionsMac->addItems(wc_info->getResolutions());
    int res_index = 0;
    int res_x, res_y;
    if(ltr_int_wc_get_resolution(&res_x, &res_y)){
      res_index = wc_info->findRes(res_x, res_y);
      ui.WebcamFtResolutionsMac->setCurrentIndex(res_index);
    }
    on_WebcamResolutions_activated(res_index);
  }
  prefInit = true;
  QString cascadePath(ltr_int_wc_get_cascade());
  ui.CascadePathMac->setText(cascadePath);
  int n = (2.0 / ltr_int_wc_get_eff()) - 2;
  ui.ExpFilterFactorMac->setValue(n);
  on_ExpFilterFactor_valueChanged(n);
  ui.OptimLevelMac->setValue(ltr_int_wc_get_optim_level());
  prefInit = false;
  ltr_int_wc_close_prefs();
  initializing = false;
  return true;
}

bool WebcamFtPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == WEBCAM_FT)){
    webcam_selected = true;
  }
  QStringList &webcams = WebcamInfo::EnumerateWebcams();
  QStringList::iterator i;
  PrefsLink *pl;
  QVariant v;
  for(i = webcams.begin(); i != webcams.end(); ++i){
    pl = new PrefsLink(WEBCAM_FT, *i);
    v.setValue(*pl);
    combo.addItem((*i)+" face tracker", v);
    if(webcam_selected && (*i == id)){
      combo.setCurrentIndex(combo.count() - 1);
      res = true;
    }
  }
  delete(&webcams);
  return res;
}

void WebcamFtPrefs::on_FindCascadeMac_pressed()
{
  QString path = ui.CascadePathMac->text();
  if(path.isEmpty()){
    path = QString(ltr_int_get_data_path(""));
  }else{
    QDir tmp(path);
    path = tmp.filePath(path);
  }
  QString fileName = QFileDialog::getOpenFileName(NULL,
     "Find Harr/LBP cascade", path, "xml Files (*.xml)");
  ui.CascadePathMac->setText(fileName);
  on_CascadePath_editingFinished();
}

void WebcamFtPrefs::on_CascadePathMac_editingFinished()
{
  if(!prefInit){
    ltr_int_wc_set_cascade(ui.CascadePathMac->text().toAscii().data());
  }
}

void WebcamFtPrefs::on_ExpFilterFactorMac_valueChanged(int value)
{
  float a = 2 / (value + 2.0); //EWMA window size
  //ui.ExpFiltFactorValMac->setText(QString("%1").arg(a, 0, 'g', 2));
  if(!prefInit){
    ltr_int_wc_set_eff(a);
  }
}

void WebcamFtPrefs::on_OptimLevelMac_valueChanged(int value)
{
  if(!prefInit){
    ltr_int_wc_set_optim_level(value);
  }
}
