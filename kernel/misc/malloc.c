/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <csi_config.h>
#include <stdlib.h>
#include <umm_heap.h>
#include <mm.h>
#include <k_api.h>

void *malloc(size_t size)
{
    void *ret;

    if (size < 1) {
        return NULL;
    }
    size_t irq_flags;
    irq_flags = cpu_intrpt_save();
    ret = mm_malloc(USR_HEAP, size, NULL);
    cpu_intrpt_restore(irq_flags);
    return ret;
}

void free(void *ptr)
{
    size_t irq_flags;
    irq_flags = cpu_intrpt_save();
    mm_free(USR_HEAP, ptr, NULL);
    cpu_intrpt_restore(irq_flags);
}

void *realloc(void *ptr, size_t size)
{
    void *new_ptr;

    new_ptr = malloc(size);

    if (new_ptr == NULL) {
        return new_ptr;
    }

    if (ptr) {
        memcpy(new_ptr, ptr, size);
        free(ptr);
    }

    return new_ptr;
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;

    ptr = malloc(size * nmemb);

    if (ptr) {
        memset(ptr, 0, size * nmemb);
    }

    return ptr;
}
