#include "Touch.h"
#include <esp_log.h>
#include "driver/touch_pad.h"
#include "application.h"
#include "assets/lang_config.h"

#define TAG "TouchSensor"

#define TOUCH_LIMIT_THRESHOLD_MAX 200000 // 最大阈值，用于判断触摸状态是否改变
#define TOUCH_CHANGE_CONFIG 0

namespace touch_sensor_manager
{

    struct touch_sensor_cof
    {
        touch_pad_t gpio;
        int threshold;
        const std::string_view &sound;
    };

    const std::array<touch_sensor_cof, 5> sensors{
        {
            touch_sensor_cof{TOUCH_PAD_NUM3, 52000, Lang::Sounds::P3_HELLO},    // 左12
            touch_sensor_cof{TOUCH_PAD_NUM13, 52000, Lang::Sounds::P3_BIEMOLE}, // 右17
            touch_sensor_cof{TOUCH_PAD_NUM14, 52000, Lang::Sounds::P3_BASHI},   // 右16
            touch_sensor_cof{TOUCH_PAD_NUM12, 52000, Lang::Sounds::P3_HELLO},   // 左17
            touch_sensor_cof{TOUCH_PAD_NUM11, 52000, Lang::Sounds::P3_HELLO},   // 左16

            // touch_sensor_cof{TOUCH_PAD_NUM5, 52000, Lang::Sounds::P3_BASHI}, //会阻塞xiaozhi
            // touch_sensor_cof{TOUCH_PAD_NUM6, 52000},  6不能用，会阻塞其他的gpio
            // touch_sensor_cof{TOUCH_PAD_NUM7, 52000, Lang::Sounds::P3_HELLO}, //会阻塞xiaozhi
            // touch_sensor_cof{TOUCH_PAD_NUM8, 52000, Lang::Sounds::P3_HELLO}, //会阻塞xiaozhi
            // touch_sensor_cof{TOUCH_PAD_NUM9, 52000, Lang::Sounds::P3_HELLO}, //会阻塞xiaozhi
        }};

    std::string Touch::Initialize()
    {
        touch_pad_init(); // 初始化触摸传感器硬件和软件状态机

        for (auto &sensor : sensors)
        {
            ESP_LOGI(TAG, "GPIO: %d, Threshold: %d", sensor.gpio, sensor.threshold);
            touch_pad_config(sensor.gpio); // 配置触摸引脚
            touch_pad_set_cnt_mode(sensor.gpio, TOUCH_PAD_SLOPE_DEFAULT, TOUCH_PAD_TIE_OPT_DEFAULT);

#if TOUCH_CHANGE_CONFIG
            touch_pad_set_cnt_mode(sensor.gpio, TOUCH_PAD_SLOPE_DEFAULT, TOUCH_PAD_TIE_OPT_DEFAULT);
#endif
        }

#if TOUCH_CHANGE_CONFIG
        /* If you want change the touch sensor default setting, please write here(after initialize). There are examples: */
        touch_pad_set_measurement_interval(TOUCH_PAD_SLEEP_CYCLE_DEFAULT);
        touch_pad_set_charge_discharge_times(TOUCH_PAD_MEASURE_CYCLE_DEFAULT);
        touch_pad_set_voltage(TOUCH_PAD_HIGH_VOLTAGE_THRESHOLD, TOUCH_PAD_LOW_VOLTAGE_THRESHOLD, TOUCH_PAD_ATTEN_VOLTAGE_THRESHOLD);
        touch_pad_set_idle_channel_connect(TOUCH_PAD_IDLE_CH_CONNECT_DEFAULT);
#endif
        /* Denoise setting at TouchSensor 0. */
        touch_pad_denoise_t denoise = {
            /* The bits to be cancelled are determined according to the noise level. */
            .grade = TOUCH_PAD_DENOISE_BIT4,
            .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
        };
        touch_pad_denoise_set_config(&denoise);
        touch_pad_denoise_enable();
        ESP_LOGI(TAG, "Denoise function init");

        // 设置FSM模式为定时器模式
        touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
        touch_pad_fsm_start();
        return "Touch sensor Initialize.";
    }

    void Touch::Update()
    {
        if (m_is_playing)
            return;

        auto &application = Application::GetInstance();

        printf("STATE: %d ", application.GetDeviceState());

        if (application.GetDeviceState() < kDeviceStateIdle)
        {
            return;
        }

        // 获取原始触摸值
        uint32_t touch_value;
        for (auto &sensor : sensors)
        {
            touch_pad_read_raw_data(sensor.gpio, &touch_value); // read raw data.
            printf("T%d: [%4" PRIu32 "] ", sensor.gpio, touch_value);

            if (touch_value > sensor.threshold && touch_value < TOUCH_LIMIT_THRESHOLD_MAX)
            {
                // 检查是否接近阈值，表示有触摸动作发生
                printf("Touch detected: [%s] ", sensor.sound.data());
                if (!sensor.sound.empty())
                {
                    // application.ResetDecoder();
                    last_state = application.GetDeviceState();
                    application.CustomPlaySound(sensor.sound, &last_state);
                    m_is_playing = true;

                    break;
                }
            }
        }

        if (m_is_playing)
        {
            vTaskDelay(pdMS_TO_TICKS(3000));
            application.CustomRecoverSound(last_state);
            m_is_playing = false;
            last_state = kDeviceStateUnknown;
        }
        printf("\n");
    }

}
