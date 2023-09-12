/*
 * QEMU OpenTitan SPI Device
 *
 * Copyright (c) 2023 lowRISC, CIC.
 *
 * Author(s):
 *  Hugo McNally <hugom@lowrisc.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "qemu/fifo8.h"
#include "qemu/log.h"
#include "qemu/main-loop.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "qapi/error.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "hw/opentitan/ot_alert.h"
#include "hw/opentitan/ot_spi_device.h"
#include "hw/qdev-properties-system.h"
#include "hw/qdev-properties.h"
#include "hw/registerfields.h"
#include "hw/riscv/ibex_common.h"
#include "hw/riscv/ibex_irq.h"

struct OtSPIDeviceState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    IbexIRQ irqs[12u]; /**< System bus IRQs */
    IbexIRQ alert; /**< OpenTitan alert */
};

/* this class is only required to manage on-hold reset */
struct OtSPIDeviceClass {
    SysBusDeviceClass parent_class;
    ResettablePhases parent_phases;
};

/* ------------------------------------------------------------------------ */
/* IRQ and alert management */
/* ------------------------------------------------------------------------ */

/** IRQ lines */
enum OtSPIDeviceIrq {
    IRQ_GENERIC_RX_FULL,
    IRQ_GENERIC_RX_WATERMARK,
    IRQ_GENERIC_TX_WATERMARK,
    IRQ_GENERIC_RX_ERROR,
    IRQ_GENERIC_RX_OVERFLOW,
    IRQ_GENERIC_TX_UNDERFLOW,
    IRQ_UPLOAD_CMDFIFO_NOT_EMPTY,
    IRQ_UPLOAD_PAYLOAD_NOT_EMPTY,
    IRQ_UPLOAD_PAYLOAD_OVERFLOW,
    IRQ_READBUF_WATERMARK,
    IRQ_READBUF_FLIP,
    IRQ_TPM_HEADER_NOT_EMPTY,
    _IRQ_COUNT,
};


/* ------------------------------------------------------------------------ */
/* State machine and I/O */
/* ------------------------------------------------------------------------ */

static uint64_t ot_spi_device_read(void *opaque, hwaddr addr, unsigned int size)
{
    return 0u;
};

static void ot_spi_device_write(void *opaque, hwaddr addr, uint64_t val64,
                              unsigned int size)
{
};

/* ------------------------------------------------------------------------ */
/* Device description/instanciation */
/* ------------------------------------------------------------------------ */

/* clang-format off */
static const MemoryRegionOps ot_spi_host_ops = {
    .read = ot_spi_device_read,
    .write = ot_spi_device_write,
    /* OpenTitan default LE */
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        /* although some registers only supports 2 or 4 byte write access */
        .min_access_size = 1u,
        .max_access_size = 4u,
    }
};
/* clang-format on */

static void ot_spi_device_instance_init(Object *obj)
{
    OtSPIDeviceState *s = OT_SPI_DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &ot_spi_host_ops, s, TYPE_OT_SPI_DEVICE,
                          /*size*/ 0x2000u);

    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    _Static_assert(_IRQ_COUNT == ARRAY_SIZE(s->irqs), "Incoherent IRQ count");

    ibex_qdev_init_irqs(obj, &s->irqs[0u], SYSBUS_DEVICE_GPIO_IRQ,
                        ARRAY_SIZE(s->irqs));
    ibex_qdev_init_irq(obj, &s->alert, OPENTITAN_DEVICE_ALERT);
}

static void ot_spi_device_class_init(ObjectClass *klass, void *data)
{
    __attribute__((unused))
    DeviceClass *dc = DEVICE_CLASS(klass);
    __attribute__((unused))
    ResettableClass *rc = RESETTABLE_CLASS(klass);
};

static const TypeInfo ot_spi_device_info = {
    .name = TYPE_OT_SPI_DEVICE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(OtSPIDeviceState),
    .instance_init = ot_spi_device_instance_init,
    .class_init = ot_spi_device_class_init,
    .class_size = sizeof(OtSPIDeviceClass),
};

static void ot_spi_device_register_types(void)
{
    type_register_static(&ot_spi_device_info);
}

type_init(ot_spi_device_register_types)
