/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//20140709 MetaExtractor_Editor

#ifndef _TAG_TEMPLATE_BASE_H_
#define _TAG_TEMPLATE_BASE_H_

#include <utils/Log.h>

namespace android {

static const int debug_template_log = 1;

template <typename T>
class metaTag {
    public:
        inline metaTag() : m_ptr(0) { if(debug_template_log) ALOGD("#### metaTag<T>::metaTag() #### construction!"); }
        ~metaTag();
        metaTag(T* other);
        metaTag(const metaTag<T>& other);
        template<typename U> metaTag(U* other);
        template<typename U> metaTag(const metaTag<U>& other);

        void clear();
        
        metaTag& operator = (T* other);
        metaTag& operator = (const metaTag<T>& other);    
        template<typename U> metaTag& operator = (const metaTag<U>& other);
        template<typename U> metaTag& operator = (U* other);

        inline  T&      operator* () const  { return *m_ptr; }
        inline  T*      operator-> () const { return m_ptr;  }
        inline  T*      get() const         { return m_ptr; }

        inline bool operator == (const metaTag<T>& o) const {
            return m_ptr == o.m_ptr;
        }
        inline bool operator == (const T* o) const {
            return m_ptr == o;
        }
        inline bool operator != (const metaTag<T>& o) const {
            return m_ptr != o.m_ptr;
        }
        inline bool operator != (const T* o) const {
            return m_ptr != o;
        }
    
    private :
        T* m_ptr;
};

template<typename T>
metaTag<T>::metaTag(T* other)
: m_ptr(other) {
    if(debug_template_log) ALOGD("#### metaTag<T>::metaTag(T* other) #### ");
}

template<typename T>
metaTag<T>::metaTag(const metaTag<T>& other)
: m_ptr(other.m_ptr) {
    if(debug_template_log) ALOGD("#### metaTag<T>::metaTag(const metaTag<T>& other) #### ");
}

template<typename T> template<typename U>
metaTag<T>::metaTag(U* other) 
: m_ptr(other) {
    if(debug_template_log) ALOGD("#### metaTag<T>::metaTag(U* other)  #### ");
}

template<typename T> template<typename U>
metaTag<T>::metaTag(const metaTag<U>& other)
: m_ptr(other.m_ptr) {
    if(debug_template_log) ALOGD("#### metaTag<T>::metaTag(const metaTag<U>& other) #### ");
}

template<typename T>
metaTag<T>& metaTag<T>::operator = (const metaTag<T>& other) {
    if(debug_template_log) ALOGD("#### metaTag<T>& metaTag<T>::operator = (const metaTag<T>& other) #### ");
    T* otherPtr(other.m_ptr);
    m_ptr = otherPtr;
    return *this;
}

template<typename T>
metaTag<T>& metaTag<T>::operator = (T* other) {
    if(debug_template_log) ALOGD("#### metaTag<T>& metaTag<T>::operator = (T* other) #### ");
    m_ptr = other;
    return *this;
}

template<typename T> template<typename U>
metaTag<T>& metaTag<T>::operator = (const metaTag<U>& other) {
    if(debug_template_log) ALOGD("#### metaTag<T>& metaTag<T>::operator = (const metaTag<U>& other) #### ");
    T* otherPtr(other.m_ptr);
    m_ptr = otherPtr;
    return *this;
}

template<typename T> template<typename U>
metaTag<T>& metaTag<T>::operator = (U* other) {
    if(debug_template_log) ALOGD("#### metaTag<T>& metaTag<T>::operator = (U* other) #### ");
    m_ptr = other;
    return *this;
}

template<typename T>
metaTag<T>::~metaTag() {
    if(debug_template_log) ALOGD("#### metaTag<T>::~metaTag #### deconstruction!");
}

template<typename T>
void metaTag<T>::clear() {
    if(debug_template_log) ALOGD("#### void sp<T>::clear() #### ");
    if(m_ptr) {
        if(debug_template_log) ALOGI("#### Deleting metaTag #### m_ptr = 0 ");
        delete m_ptr;
        m_ptr = 0;
    }
}

}  // namespace android
#endif  // _TAG_TEMPLATE_BASE_H_