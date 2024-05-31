#include "wl_pcm_bean.h"

WLPcmBean::WLPcmBean(SAMPLETYPE *buffer, int size) {
    this->m_buffer = (char *) malloc(size);
    m_buffsize = size;
    memcpy(m_buffer, buffer, size);
};

WLPcmBean::~WLPcmBean() {
    free(m_buffer);
    m_buffer = NULL;
}