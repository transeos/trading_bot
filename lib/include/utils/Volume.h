//
// Created by subhagato on 4/23/18.
//

#pragma once

#include "utils/TraderUtils.h"
#include <cmath>

class Volume {
 private:
  double bvolume_;
  double svolume_;

 public:
  Volume() {
    bvolume_ = 0;
    svolume_ = 0;
  }

  Volume(double volume) {
    setVolume(volume);
  }

  void setVolume(double volume) {
    if (volume > 0) {
      bvolume_ = volume;
      svolume_ = 0;
    } else {
      svolume_ = -volume;
      bvolume_ = 0;
    }
  }

  double getVolume() const {
    return (bvolume_ + svolume_);
  }

  double getBuyVolume() const {
    return bvolume_;
  }

  double getSellVolume() const {
    return svolume_;
  }

  double& getBuyVolume() {
    return bvolume_;
  }

  double& getSellVolume() {
    return svolume_;
  }

  friend Volume operator+(const Volume& v1, const Volume& v2) {
    Volume v;
    v.bvolume_ = v1.bvolume_ + v2.bvolume_;
    v.svolume_ = v1.svolume_ + v2.svolume_;
    return v;
  }

  friend Volume operator-(const Volume& v1, const Volume& v2) {
    Volume v;
    v.bvolume_ = v1.bvolume_ - v2.bvolume_;
    v.svolume_ = v1.svolume_ - v2.svolume_;
    return v;
  }

  friend Volume operator*(const Volume& v1, double d) {
    Volume v;
    v.bvolume_ = v1.bvolume_ * d;
    v.svolume_ = v1.svolume_ * d;
    return v;
  }

  friend Volume operator/(const Volume& v1, double d) {
    Volume v;
    v.bvolume_ = v1.bvolume_ / d;
    v.svolume_ = v1.svolume_ / d;
    return v;
  }

  friend Volume operator+(const Volume& v1, double d) {
    return v1 + Volume(d);
  }

  friend Volume operator-(const Volume& v1, double d) {
    return v1 - Volume(d);
  }

  explicit operator double() const {
    return getVolume();
  }

  Volume& operator=(const double d) {
    setVolume(d);
    return *this;
  }

  friend bool operator==(const Volume& lhs, const Volume& rhs) {
    return (lhs.svolume_ == rhs.svolume_ && lhs.bvolume_ == rhs.bvolume_);
  }
  friend bool operator!=(const Volume& lhs, const Volume& rhs) {
    return !(lhs == rhs);
  }
  friend bool operator>=(const Volume& lhs, const Volume& rhs) {
    return lhs.getVolume() >= rhs.getVolume();
  }
  friend bool operator<=(const Volume& lhs, const Volume& rhs) {
    return lhs.getVolume() <= rhs.getVolume();
  }
  friend bool operator>(const Volume& lhs, const Volume& rhs) {
    return lhs.getVolume() > rhs.getVolume();
  }
  friend bool operator<(const Volume& lhs, const Volume& rhs) {
    return lhs.getVolume() < rhs.getVolume();
  }

  friend bool operator==(const Volume& lhs, double rhs) {
    return rhs == lhs.getVolume();
  }
  friend bool operator!=(const Volume& lhs, double rhs) {
    return !(lhs == rhs);
  }
  friend bool operator>=(const Volume& lhs, double rhs) {
    return lhs.getVolume() >= rhs;
  }
  friend bool operator<=(const Volume& lhs, double rhs) {
    return lhs.getVolume() <= rhs;
  }
  friend bool operator>(const Volume& lhs, double rhs) {
    return lhs.getVolume() > rhs;
  }
  friend bool operator<(const Volume& lhs, double rhs) {
    return lhs.getVolume() < rhs;
  }

  friend std::ostream& operator<<(std::ostream& os, const Volume& v) {
    os << v.getVolume();
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }
};
