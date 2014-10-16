#include "transform.h"
#include <cmath>
#include <iostream>
#include <QPainter>
#include "math_utils.h"

const int screenMax = 1024;
const float timeFast = 0.1;
const float timeSlow = 4;


MickeyCurveShow::MickeyCurveShow(QWidget *parent) : QWidget(parent), img(NULL)
{
  //setBackgroundRole(QPalette::Base);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  setMinimumSize(100, 100);
  setAutoFillBackground(true);
}

void MickeyCurveShow::updatePixmap(QPointF curPoints[], QPointF newPoints[], int pointCount)
{
  float s = ((width() > height()) ? height() : width()) - 1;
  if((img == NULL) || (img->width() != s)){
    if(img != NULL){
      delete img;
    }
    img = new QPixmap(s, s);
  }
  for(int i = 0; i < pointCount; ++i){
    curPoints[i] *= (s - 1);
    curPoints[i].setY(s - 1 - curPoints[i].y());
    newPoints[i] *= (s - 1);
    newPoints[i].setY(s - 1 - newPoints[i].y());
  }
  img->fill();
  QPainter painter(img);
  painter.setPen(Qt::red);
  painter.drawPolyline(newPoints, pointCount);
  painter.setPen(Qt::black);
  painter.drawPolyline(curPoints, pointCount);
  painter.end();
  update();
}

void MickeyCurveShow::paintEvent(QPaintEvent * /* event */)
{
  QPainter painter(this);
  painter.drawPixmap(0,0,*img);
  painter.end();
}



MickeysAxis::MickeysAxis(): curveShow(NULL)
{
  curveShow = new MickeyCurveShow();
  GUI.getAxisViewLayout()->addWidget(curveShow);
  QObject::connect(curveShow, SIGNAL(resized()), this, SLOT(curveShow_resized()));
  QObject::connect(&GUI, SIGNAL(axisChanged()), this, SLOT(axisChanged()));
  setup.sensitivity = GUI.getSensitivity();
  setup.deadzone = GUI.getDeadzone();
  setup.curvature= GUI.getCurvature();
  setup.stepOnly = GUI.getStepOnly();
  setup.smoothing = GUI.getSmoothing();
  newSetup = setup;
}

MickeysAxis::~MickeysAxis()
{
}

float MickeysAxis::getSpeed(int sens)
{
  float slewTime = timeSlow + (timeFast-timeSlow) * (sens / 100.0);
  return screenMax / slewTime;
}

float MickeysAxis::response(float mag, setup_t *s)
{
  if(s == NULL){
    s = &setup;
  }
  //deadzone 0 - 50% of the maxValue
  float dz = 0.5 * ((float)s->deadzone) / 99.0f;
  if(mag <= dz){
    mag = 0;
  }else{
    //here can be curve or whatever...
    if(s->stepOnly){
      mag = 1;
    }else{
      mag = (mag - dz) / (1.0 - dz);
      if(s->curvature < 50){
        float c = 1.0 + ((50.0 - s->curvature) / 50.0) * 3.0; //c = (1:4);
        mag = expf(logf(mag) / c);
      }else{
        float c = 1.0 + ((s->curvature - 50.0) / 50.0) * 3.0; //c = (1:4);
        mag = expf(logf(mag) * c);
      }
    }
  }
  return mag;
}

void MickeysAxis::step(float valX, float valY, int elapsed, float &accX, float &accY)
{
  float mag = sqrtf(valX * valX + valY * valY);
  float angle = atan2f(valY, valX);
  if(mag > 1) mag = 1;
  
  mag = response(mag);
  
  accX += mag * cosf(angle) * getSpeed(setup.sensitivity) * (elapsed / 1000.0);
  accY += mag * sinf(angle) * getSpeed(setup.sensitivity) * (elapsed / 1000.0);
}


void MickeysAxis::smooth(float &valX, float &valY)
{
  valX = ltr_int_nonlinfilt(valX, prevX, setup.smoothing/3.0);
  valY = ltr_int_nonlinfilt(valY, prevY, setup.smoothing/3.0);
  prevX = valX;
  prevY = valY;
}

void MickeysAxis::updatePixmap()
{
  const int pointCount = 128;
  QPointF cPoints[pointCount];
  QPointF nPoints[pointCount];
  float x;
  for(int i = 0; i < pointCount; ++i){
    x = (i / (float)(pointCount-1));
    cPoints[i] = QPointF(x, response(x, &setup));
    nPoints[i] = QPointF(x, response(x, &newSetup));
  }
  if(curveShow != NULL){
    curveShow->updatePixmap(cPoints, nPoints, pointCount);
  }
}

void MickeysAxis::axisChanged(){
  newSetup.sensitivity = GUI.getSensitivity();
  newSetup.deadzone = GUI.getDeadzone();
  newSetup.curvature= GUI.getCurvature();
  newSetup.smoothing= GUI.getSmoothing();
  newSetup.stepOnly = GUI.getStepOnly();
  updatePixmap();
}

void MickeysAxis::applySettings()
{
  oldSetup = setup;
  setup = newSetup;
  updatePixmap();
}

void MickeysAxis::keepSettings()
{
  oldSetup = setup;
}

void MickeysAxis::revertSettings()
{
  setup = oldSetup;
  GUI.setSensitivity(setup.sensitivity);
  GUI.setDeadzone(setup.deadzone);
  GUI.setCurvature(setup.curvature);
  GUI.setSmoothing(setup.smoothing);
  GUI.setStepOnly(setup.stepOnly);
  updatePixmap();
}

MickeyTransform::MickeyTransform() : accX(0.0), accY(0.0), calibrating(false), axis()
{
  GUI.getMaxVal(maxValX, maxValY);
  prevMaxValX = maxValX;
  prevMaxValY = maxValY;
  currMaxValX = maxValX;
  currMaxValY = maxValY;
}

MickeyTransform::~MickeyTransform()
{
}

static float norm(float val)
{
  if(val > 1.0f){
    return 1.0;
  }
  if(val < -1.0){
    return -1.0;
  }
  return val;
}

/*
static float sign(float val)
{
  return (val >= 0 ? 1.0f : -1.0f); 
}

static float norm(float val, float limit, float &currentLimit)
{
  float absVal = fabsf(val);
  //when crossing the zero, equalize currentLimit
  if(sign(val) != sign(currentLimit)){
    currentLimit = sign(val) * limit;
  }
  //if we are above the limit, extend the limit until next zero crossing
  if(absVal > fabsf(currentLimit)){
    currentLimit = val;
  }
  return val / fabsf(currentLimit);
}
*/

void MickeyTransform::update(float valX, float valY, bool relative, int elapsed, float &x, float &y)
{
  axis.smooth(valX, valY);
  if(!calibrating){
    if(relative){
      axis.step(norm(-valX/maxValX), norm(-valY/maxValY), elapsed, accX, accY);
      x = accX;
      accX -= x;
      y = accY;
      accY -= y;
    }else{
//      x = norm(-valX, maxValX, currMaxValX);
//      y = norm(-valY, maxValY, currMaxValY);
      x = norm(-valX / maxValX);
      y = norm(-valY / maxValY);
      //std::cout<<"valX: "<<-valX<<"=> "<<x<<"   Limit: "<<maxValX<<"   CurrentLimit:"<<currMaxValX<<std::endl;
    }
  }else{
    if(valX > maxValX){
      maxValX = valX;
    }
    if(valY > maxValY){
      maxValY = valY;
    }
    if(valX < minValX){
      minValX = valX;
    }
    if(valY < minValY){
      minValY = valY;
    }
  }
}

void MickeyTransform::startCalibration()
{
  calibrating = true;
  //std::cout<<"Calibrating X: "<<prevMaxValX<<" Y: "<<prevMaxValY<<std::endl;
  prevMaxValX = maxValX;
  prevMaxValY = maxValY;
  maxValX = 0.0f;
  minValX = 0.0f;
  maxValY = 0.0f;
  minValY = 0.0f;
}

void MickeyTransform::finishCalibration()
{
  calibrating = false;
  //devise a reasonable profile...
  minValX = fabsf(minValX);
  maxValX = fabsf(maxValX);
  minValY = fabsf(minValY);
  maxValY = fabsf(maxValY);
  //get lower of those values, so we have full
  //  range in both directions (limit the bigger).
  maxValX = (minValX > maxValX)? maxValX: minValX;
  maxValY = (minValY > maxValY)? maxValY: minValY;
  GUI.setMaxVal(maxValX, maxValY);
  //std::cout<<"Finished X: "<<maxValX<<" Y: "<<maxValY<<std::endl;
  //std::cout<<"Saved X: "<<prevMaxValX<<" Y: "<<prevMaxValY<<std::endl;
}

void MickeyTransform::cancelCalibration()
{
  calibrating = false;
  GUI.getMaxVal(maxValX, maxValY);
}

void MickeyTransform::applySettings()
{
  //std::cout<<"Applying X: "<<maxValX<<" Y: "<<maxValY<<std::endl;
  //std::cout<<"Saved X: "<<prevMaxValX<<" Y: "<<prevMaxValY<<std::endl;
  axis.applySettings();
}

void MickeyTransform::keepSettings()
{
  //std::cout<<"Keeping X: "<<maxValX<<" Y: "<<maxValY<<std::endl;
  prevMaxValX = maxValX;
  prevMaxValY = maxValY;
  axis.keepSettings();
}

void MickeyTransform::revertSettings()
{
  //std::cout<<"Reverting to X: "<<prevMaxValX<<" Y: "<<prevMaxValY<<std::endl;
  maxValX = prevMaxValX;
  maxValY = prevMaxValY;
  GUI.setMaxVal(maxValX, maxValY);
  axis.revertSettings();
}



/*
//Deadzone - 0 - 50%...
//Sensitivity - normalize input, apply curve, return (-1, 1); outside - convert to speed...
//Dependency on updte rate...
*/
