/*!
 @file toolchain.h
 @brief Common include to allow features based on the toolchain in use
 @author Ben Nahill <bnahill@gmail.com>
 */

#ifndef __TOOLCHAIN_H_
#define __TOOLCHAIN_H_

#ifdef __CC_ARM
	#define RESTRICT
	#define INLINE __inline
#elif defined ( __GNUC__ )
	#define RESTRICT restrict
	#define INLINE inline
#endif

#endif
