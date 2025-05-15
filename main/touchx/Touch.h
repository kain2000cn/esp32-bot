// Touch.h
#ifndef _TOUCH_H_
#define _TOUCH_H_

#include <string>
#include "application.h"

namespace touch_sensor_manager
{
    class Touch
    {

    public:
        static Touch &GetInstance()
        {
            static Touch instance;
            return instance;
        }

        // 删除拷贝构造函数和赋值运算符
        Touch(const Touch &) = delete;
        Touch &operator=(const Touch &) = delete;

        void Update();

        std::string Initialize();

    private:
        Touch() = default;
        ~Touch() = default;

        bool m_is_playing = false;

        DeviceState last_state = kDeviceStateUnknown;
    };

} // namespace touch_sensor_manager

#endif // _TOUCH_H_