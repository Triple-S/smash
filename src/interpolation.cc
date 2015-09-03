#include "include/interpolation.h"

#include <iostream>
#include <sstream>

InterpolateDataSpline::InterpolateDataSpline(const std::vector<double>& x,
                                             const std::vector<double>& y) {
    const auto N = x.size();
    if (y.size() != N) {
      throw std::runtime_error("Need two vectors of equal length for interpolation.");
    }
    if (N < 3) {
      throw std::runtime_error("Need at least 3 data points for cubic spline interpolation.");
    }
    const auto p = generate_sort_permutation(
        x, [&](double const& a, double const& b) {
          return a < b;
        });
    const std::vector<double> sorted_x = std::move(apply_permutation(x, p));
    const std::vector<double> sorted_y = std::move(apply_permutation(y, p));
    for (size_t i = 0; i < sorted_x.size() - 1; i++) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wfloat-equal"
        if (sorted_x[i] == sorted_x[i + 1]) {
        #pragma GCC diagnostic pop
          std::stringstream error_msg;
          error_msg << "InterpolateDataSpline: Each x value must be unique. \""
                    << sorted_x[i] << "\" was found twice.";
          throw std::runtime_error(error_msg.str());
        }
    }
    first_x_ = sorted_x.front();
    last_x_ = sorted_x.back();
    first_y_ = sorted_y.front();
    last_y_ = sorted_y.back();
    acc_ = gsl_interp_accel_alloc();
    spline_ = gsl_spline_alloc(gsl_interp_cspline, N);
    gsl_spline_init(spline_, &(*sorted_x.begin()), &(*sorted_y.begin()), N);
}

InterpolateDataSpline::~InterpolateDataSpline() {
    gsl_spline_free(spline_);
    gsl_interp_accel_free(acc_);
}

double InterpolateDataSpline::operator()(double xi) const {
    // constant extrapolation
    if (xi < first_x_) {
        return first_y_;
    }
    if (xi > last_x_) {
        return last_y_;
    }
    // cubic spline interpolation
    return gsl_spline_eval(spline_, xi, acc_);
}