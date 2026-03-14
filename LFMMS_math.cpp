// LFMMS_logical_volume_check.cpp
module LFMMS;

#include "LFMMS_dp.h"

namespace lfmms_f {
	#define EXIT_FAILURE 1

	#if defined(LFMMS_INCLUDE_TEST)
	#define __CODING_ERROR__(mes)\
		mes().out(); \
		throw mes();
	#else
	#define __CODING_ERROR__(mes)\
		mes().out();std::exit(EXIT_FAILURE);
	#endif

	#define __SYS_ERROR__(mes)\
		mes().out(); \
		throw mes();

	#define __RSM__ mes_out = mes::a_mes()        //reset mes_out
	#define __CAR__ if(mes_out.code != 0)return   //check and return

    size_t math::nf_add(size_t a, size_t b) {
        if (a > SIZE_MAX - b) {
            __CODING_ERROR__(lfmms_m::err::coding_err_lfmms_add_overflow);
        }
        return a + b;
    }

    size_t math::nf_sub(size_t a, size_t b) {
        if (a < b) {
            __CODING_ERROR__(lfmms_m::err::coding_err_lfmms_sub_overflow);
        }
        return a - b;
    }

    size_t math::nf_mul(size_t a, size_t b) {
        if (b != 0 && a > SIZE_MAX / b) {
            __CODING_ERROR__(lfmms_m::err::coding_err_lfmms_mul_overflow);
        }
        return a * b;
    }

    size_t math::nf_div(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_lfmms_div_by_zero);
        }
        return a / b;
    }

    size_t math::nf_mod(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_lfmms_mod_by_zero);
        }
        if (b == 1) {
            return 0;
        }
        if ((b & (b - 1)) == 0) {
            return a & (b - 1);
        }
        return a % b;
    }
};