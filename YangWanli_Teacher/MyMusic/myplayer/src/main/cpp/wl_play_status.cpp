#include "wl_play_status.h"

WLPlayStatus::WLPlayStatus() {
    m_is_exit = false;
    m_load = true;
    m_seek = false;
    m_pause = false;
}

WLPlayStatus::~WLPlayStatus() {
    m_is_exit = false;
    m_load = true;
    m_seek = false;
    m_pause = false;
}
