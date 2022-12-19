#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <Windows.h>
#endif  // WIN32

#include "common.h"

my_timer_t g_timer;
static int s_timeout = DEFAULT_TIMEOUT;
static int s_interval = DEFAULT_INTERVAL;

void set_timer(void)
{
    if (0 == g_timer.enable)
    {
        g_timer.enable = 1;
        g_timer.timeout = s_timeout;
        platform_mutex_init(&g_timer.mutex);
    }
    else
    {
        platform_mutex_lock(&g_timer.mutex);
        s_timeout = DEFAULT_TIMEOUT;
        g_timer.timeout = s_timeout;
        platform_mutex_unlock(&g_timer.mutex);
    }
}

void unset_timer(void)
{
    if (1 == g_timer.enable)
    {
        if (1 == platform_is_current_thread(&g_timer.pid))
        {
            g_timer.enable = 0;
            platform_mutex_destroy(&g_timer.mutex);
        }
        else  // Just let the timeout be as long as possible
        {
            platform_mutex_lock(&g_timer.mutex);
            s_interval = 0;
            g_timer.timeout = INT32_MAX;
            platform_mutex_unlock(&g_timer.mutex);
        }
    }
}

#ifndef WIN32
void *timer_thread(void *param)
#else
unsigned int WINAPI timer_thread(void *param)
#endif  // !WIN32
{
    set_timer();

    while (1)
    {
        platform_wait_us(s_interval);

        platform_mutex_lock(&g_timer.mutex);
        g_timer.timeout -= s_interval;

        if (g_timer.timeout <= 0)
        {
            s_timeout += s_interval;
            g_timer.timeout = s_timeout;
            platform_mutex_unlock(&g_timer.mutex);

            printf("Info: Timeout! No packets was arrived recently!\n");
            print_results();
        }
        else if (g_timer.timeout == INT32_MAX)
        {
            platform_mutex_unlock(&g_timer.mutex);
            break;
        }
        else
        {
            platform_mutex_unlock(&g_timer.mutex);
        }
    }

    unset_timer();

#ifndef WIN32
    return NULL;
#else
    CloseHandle(g_timer.pid);
    return 0;
#endif  // !WIN32
}