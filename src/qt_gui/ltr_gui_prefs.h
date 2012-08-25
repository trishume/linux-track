#ifndef LTR_GUI_PREFS__H
#define LTR_GUI_PREFS__H

#include "pref.hpp"
#include "prefs_link.h"
#include "pref_global.h"
#include <QString>
#include <QStringList>
#define PREF PrefProxy::Pref()

class PrefProxy{
 private:
  PrefProxy();
  ~PrefProxy();
  static PrefProxy *prf;
  static prefs &ltrPrefs;
  QString prefix;
 public:
  static PrefProxy& Pref();
  static void ClosePrefs();
  static void SavePrefsOnExit();
  bool activateDevice(const QString &sectionName);
  bool getActiveDevice(deviceType_t &devType, QString &id);
  bool activateModel(const QString &sectionName);
  bool getActiveModel(QString &model);
  bool getKeyVal(const QString &sectionName, const QString &keyName, 
                 QString &result);
//  bool getKeyVal(const QString &keyName, QString &result);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
		 const QString &value);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
                 const int &value);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
                 const float &value);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
                 const double &value);
  bool getFirstDeviceSection(const QString &devType, QString &result);
  bool getFirstDeviceSection(const QString &devType, 
			     const QString &devId, QString &result);
  bool createSection(QString &sectionName);
  bool addKeyVal(const QString &sectionName, const QString &keyName, 
		 const QString &value);
  bool getModelList(QStringList &list);
  bool getProfiles(QStringList &list);
  
  bool getProfileSection(const QString &name, QString &section);
//  QString getCustomSectionName();
//  QString getCustomSectionTitle();
//  bool setCustomSection(const QString &name);
  static bool savePrefs();
  bool rereadPrefs();
  bool makeRsrcDir();
  bool copyDefaultPrefs();
  void announceModelChange();
  static QString getDataPath(QString file);
  static QString getLibPath(QString file);
  static QString getRsrcDirPath();
 private:
  bool checkPrefix(bool save);
};



#endif
