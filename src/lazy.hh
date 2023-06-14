#pragma once
#ifndef HGUARD__LAZY_HH_
#define HGUARD__LAZY_HH_

#include "Common.hh"

inline namespace MAIN_PACKAGE_NAMESPACE {
namespace testing::lazy {
/****************************************************************************************/


#ifdef _WIN32
static constexpr wchar_t fname_raw[] = LR"(D:\ass\VisualStudio\FUCK_WINDOWS\src\testing01.cc)";
static constexpr wchar_t path_raw[]  = LR"(D:\ass\VisualStudio\FUCK_WINDOWS)";
#else
UU static constexpr wchar_t const fname_raw[] = LR"(/home/bml/files/projects/FUCK_WINDOWS/src/testing01.cc)";
UU static constexpr wchar_t const path_raw[]  = LR"(/home/bml/files/projects/FUCK_WINDOWS)";
#endif


/****************************************************************************************/
} // namespace testing::lazy
} // namespace MAIN_PACKAGE_NAMESPACE
#endif
