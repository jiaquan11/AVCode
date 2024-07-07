#include "wl_pcm_bean.h"

WLPcmBean::WLPcmBean(char* buffer, int size) {
    m_buffer = static_cast<char *>(malloc(size));
    m_buffsize = size;
    memcpy(m_buffer, buffer, size);
};

WLPcmBean::~WLPcmBean() {
    free(m_buffer);
    m_buffer = NULL;
}