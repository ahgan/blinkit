// -------------------------------------------------
// BlinKit - skia Library
// -------------------------------------------------
//   File Name: _pc.h
// Description: Pre-compiled Header File
//      Author: Ziming Li
//     Created: 2019-03-06
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

#ifndef BLINKIT_SKIA__PC_H
#define BLINKIT_SKIA__PC_H

#pragma once

#include "build/build_config.h"

#if OS_WIN

#   pragma warning(disable: 4244 4267 4291 4334 4819)

#   define _CRT_SECURE_NO_WARNINGS

#endif

#include "_skia.h"

#endif // BLINKIT_SKIA__PC_H