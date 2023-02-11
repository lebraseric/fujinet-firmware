#ifdef BUILD_CX16

#include <cstring>
#include "cx16_i2c.h"
#include "../../include/debug.h"
#include "driver/i2c.h"
#include "../../include/pinmap.h"
#include "led.h"

uint8_t cx16_checksum(uint8_t *buf, unsigned short len)
{
    unsigned int chk = 0;

    for (int i = 0; i < len; i++)
        chk = ((chk + buf[i]) >> 8) + ((chk + buf[i]) & 0xff);

    return chk;
}

void virtualDevice::bus_to_computer(uint8_t *buf, uint16_t len, bool err)
{
    // TODO IMPLEMENT
}

uint8_t virtualDevice::bus_to_peripheral(uint8_t *buf, unsigned short len)
{
    uint8_t ck = 0;

    // TODO IMPLEMENT 

    return ck;
}

void virtualDevice::cx16_nak()
{
    // TODO IMPLEMENT
}

void virtualDevice::cx16_ack()
{
    // TODO IMPLEMENT
}

void virtualDevice::cx16_complete()
{
    // TODO IMPLEMENT
}

void virtualDevice::cx16_error()
{
    // TODO IMPLEMENT
}

systemBus virtualDevice::get_bus()
{
    return CX16;
}

uint8_t systemBus::get_byte()
{
    i2c_reset_rx_fifo(i2c_slave_port);
    i2c_reset_tx_fifo(i2c_slave_port);
    while (!(i2c_slave_read_buffer(i2c_slave_port,i2c_buffer,I2C_SLAVE_RX_BUF_LEN,400/portTICK_PERIOD_MS)));
    i2c_reset_rx_fifo(i2c_slave_port);
    i2c_reset_tx_fifo(i2c_slave_port);
    return i2c_buffer[0];
}

void systemBus::process_cmd()
{
    cmdFrame_t tempFrame;

    fnLedManager.set(eLed::LED_BUS, true);
    
    tempFrame.device = i2c_buffer[0];
    tempFrame.comnd = get_byte();
    tempFrame.aux1 = get_byte();
    tempFrame.aux2 = get_byte();
    tempFrame.cksum = get_byte();

    Debug_printf("\nCF: %02x %02x %02x %02x %02x\n",
                 tempFrame.device, tempFrame.comnd, tempFrame.aux1, tempFrame.aux2, tempFrame.cksum);

    uint8_t ck = cx16_checksum((uint8_t *)&tempFrame.commanddata, sizeof(tempFrame.commanddata)); // Calculate Checksum

    if (ck != tempFrame.checksum)
    {
        Debug_print("CHECKSUM ERROR.");
        fnLedManager.set(eLed::LED_BUS, false);
        return;
    }

    // Find device, and pass control to it, otherwise do nothing.
    virtualDevice *d = deviceById(tempFrame.device);

    if (d)
        d->process(tempFrame.commanddata,tempFrame.checksum);

    fnLedManager.set(eLed::LED_BUS, false);
}

void systemBus::process_queue()
{
    // TODO IMPLEMENT
}

void systemBus::service()
{
    int command_available=false;

    // TESTING
    memset(i2c_buffer,0,sizeof(i2c_buffer));
    command_available=i2c_slave_read_buffer(i2c_slave_port,i2c_buffer,I2C_SLAVE_RX_BUF_LEN,400/portTICK_PERIOD_MS);
    
    if (command_available)
        process_cmd();

    i2c_reset_rx_fifo(i2c_slave_port);
    i2c_reset_tx_fifo(i2c_slave_port);
}

void systemBus::setup()
{
    i2c_config_t conf_slave;

    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.sda_io_num = PIN_SDA;
    conf_slave.scl_io_num = PIN_SCL;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.slave.slave_addr = I2C_DEVICE_ID;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.clk_flags = 0;

    esp_err_t err = i2c_param_config(i2c_slave_port, &conf_slave);
    
    if (err != ESP_OK) {
        return;
    }

    i2c_driver_install(i2c_slave_port,conf_slave.mode,I2C_SLAVE_RX_BUF_LEN,I2C_SLAVE_TX_BUF_LEN, 0);
    Debug_printf("I²C installed on port %d as device 0x%02X\n",i2c_slave_port,I2C_DEVICE_ID);
}

void systemBus::addDevice(virtualDevice *pDevice, int device_id)
{
    if (!pDevice)
    {
        Debug_printf("systemBus::addDevice() pDevice == nullptr! returning.\n");
        return;
    }

    // TODO, add device shortcut pointer logic like others

    pDevice->_devnum = device_id;
    _daisyChain.push_front(pDevice);    
}

void systemBus::remDevice(virtualDevice *pDevice)
{
    if (!pDevice)
    {
        Debug_printf("system Bus::remDevice() pDevice == nullptr! returning\n");
        return;
    }

    _daisyChain.remove(pDevice);
}

void systemBus::changeDeviceId(virtualDevice *pDevice, int device_id)
{
    if (!pDevice)
    {
        Debug_printf("systemBus::changeDeviceId() pDevice == nullptr! returning.\n");
        return;
    }

    for (auto devicep : _daisyChain)
    {
        if (devicep == pDevice)
            devicep->_devnum = device_id;
    }
}

virtualDevice *systemBus::deviceById(int device_id)
{
    for (auto devicep : _daisyChain)
    {
        if (devicep->_devnum == device_id)
            return devicep;
    }
    return nullptr;
}

void systemBus::shutdown()
{
    shuttingDown = true;

    for (auto devicep : _daisyChain)
    {
        Debug_printf("Shutting down device %02x\n",devicep->id());
        devicep->shutdown();
    }
    Debug_printf("All devices shut down.\n");
}

systemBus CX16;

#endif /* BUILD_CX16 */