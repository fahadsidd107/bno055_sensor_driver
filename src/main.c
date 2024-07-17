#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <sensor/bmp180/bmp180.h>
#include <math.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdio.h>



#define SLEEP_TIME_MS 1000

#define UART1_NODE DT_NODELABEL(uart1)
static const struct device *uart_dev = DEVICE_DT_GET(UART1_NODE);

static char tx_buf[64];

void send_data_over_uart(const char *data) {
    uart_tx(uart_dev, data, strlen(data), SYS_FOREVER_MS);
}

int main(void) {
    const struct device *const sht_dev = DEVICE_DT_GET_ONE(sensirion_sht3xd);
    const struct device *const bmp_dev = DEVICE_DT_GET_ANY(bosch_bmp180);
    int rc;

    if (!device_is_ready(sht_dev)) {
        printf("SHT3XD device is not ready\n");
        return 0;
    }

    if (!device_is_ready(bmp_dev)) {
        printf("BMP180 device is not ready\n");
        return 0;
    }

    if (!device_is_ready(uart_dev)) {
        printf("UART device not ready\n");
        return 0;
    }

    while (true) {
        struct sensor_value temp, hum, press;

        // Fetch temperature and humidity from SHT3XD
        rc = sensor_sample_fetch(sht_dev);
        if (rc == 0) {
            rc = sensor_channel_get(sht_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        }
        if (rc == 0) {
            rc = sensor_channel_get(sht_dev, SENSOR_CHAN_HUMIDITY, &hum);
        }
        if (rc != 0) {
            printf("SHT3XD: failed: %d\n", rc);
            break;
        }

        double temperature = sensor_value_to_double(&temp);
        double humidity = sensor_value_to_double(&hum);

        // Fetch pressure from BMP180
        rc = bmp180_sample_fetch(bmp_dev, SENSOR_CHAN_ALL);
        if (rc == 0) {
            rc = bmp180_channel_get(bmp_dev, SENSOR_CHAN_PRESS, &press);
        }
        if (rc != 0) {
            printf("BMP180: failed: %d\n", rc);
            break;
        }

        double pressure = press.val1 + (press.val2 / 1000000.0);

        // Format the data as a string
        snprintf(tx_buf, sizeof(tx_buf), "Temperature: %.2f C, Humidity: %.2f%%, Pressure: %.2f Pa\n", temperature, humidity, pressure);

        // Send the data over UART
        send_data_over_uart(tx_buf);

        printf("Sent over UART: %s", tx_buf);

        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
