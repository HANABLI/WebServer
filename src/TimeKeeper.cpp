/**
 * @file TimeKeeper.cpp
 * 
 * This module is an implementation of 
 * the TimeKeeper class.
 * 
 * © 2024 by Hatem Nabli
 */

#include "TimeKeeper.hpp"
#include <SystemUtils/Time.hpp>

/**
 * 
 */
struct TimeKeeper::Impl
{
    /* data */
    /**
     * This is used to interface with the operating system's time
     */
    SystemUtils::Time time;
};


TimeKeeper::~TimeKeeper() noexcept = default;

TimeKeeper::TimeKeeper(): _impl(new Impl()){};

double TimeKeeper::GetCurrentTime() {
    return _impl->time.GetTime();
}