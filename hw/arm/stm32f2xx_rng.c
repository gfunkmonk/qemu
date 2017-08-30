/*-
 * Copyright (c) 2017
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
/*
 * QEMU stm32f2xx RNG emulation
 */
#include "hw/sysbus.h"
#include "hw/arm/stm32.h"
#include "crypto/random.h"

#define R_RNG_CR            (0x00 / 4)
#define R_RNG_CR_RNGEN      (1 << 2)
#define R_RNG_CR_IE         (1 << 3)

#define R_RNG_SR            (0x04 / 4)
#define R_RNG_SR_DRDY       (1 << 0)
#define R_RNG_SR_CECS       (1 << 1)
#define R_RNG_SR_SECS       (1 << 2)
#define R_RNG_SR_CEIS       (1 << 5)
#define R_RNG_SR_SEIS       (1 << 6)

#define R_RNG_DR            (0x08 / 4)
#define R_RNG_MAX           (0x0c / 4)

typedef struct f2xx_rng {
    SysBusDevice busdev;
    MemoryRegion iomem;

    uint32_t cr;
    uint32_t sr;
} f2xx_rng;

static uint64_t
f2xx_rng_read(void *arg, hwaddr addr, unsigned int size)
{
    f2xx_rng *s = arg;

    if (size != 4) {
        STM32_NOT_IMPL_REG(addr, size);
        return 0;
    }

    uint32_t random = 0;

    addr >>= 2;
    switch(addr) {
    case R_RNG_CR:
        return s->cr;
    case R_RNG_SR:
        return s->sr;
    case R_RNG_DR:
        qcrypto_random_bytes((uint8_t *) &random, sizeof(random), NULL);
        return random;
    default:
        STM32_BAD_REG(addr << 2, size);
        return 0;
    }
    return 0;
}


static void
f2xx_rng_write(void *arg, hwaddr addr, uint64_t data, unsigned int size)
{
    f2xx_rng *s = arg;

    /* XXX Check periph clock enable. */
    if (size != 4) {
        STM32_NOT_IMPL_REG(addr, size);
        return;
    }

    addr >>= 2;
    switch(addr) {
    case R_RNG_CR:
        if (data & R_RNG_CR_RNGEN) {
            s->cr |= R_RNG_CR_RNGEN;
            s->sr |= R_RNG_SR_DRDY;
        }
        if (data & R_RNG_CR_IE)
            stm32_unimp("f2xx RNG Interrupt not implemented\n");
        break;
    case R_RNG_SR:
        STM32_NOT_IMPL_REG(addr << 2, size);
        break;
    case R_RNG_DR:
        STM32_WARN_RO_REG(addr << 2);
        break;
    default:
        STM32_BAD_REG(addr << 2, size);
        return;
    }
}

static const MemoryRegionOps f2xx_rng_ops = {
    .read = f2xx_rng_read,
    .write = f2xx_rng_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    }
};

static int
f2xx_rng_init(SysBusDevice *dev)
{
    f2xx_rng *s = FROM_SYSBUS(f2xx_rng, dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &f2xx_rng_ops, s, "rng", 0x400);
    sysbus_init_mmio(dev, &s->iomem);

    return 0;
}

static void
f2xx_rng_reset(DeviceState *ds)
{
    f2xx_rng *s = FROM_SYSBUS(f2xx_rng, SYS_BUS_DEVICE(ds));

    s->cr = 0;
    s->sr = 0;
}

static Property f2xx_rng_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void
f2xx_rng_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *sc = SYS_BUS_DEVICE_CLASS(klass);
    sc->init = f2xx_rng_init;
    dc->reset = f2xx_rng_reset;
    dc->props = f2xx_rng_properties;
}

static const TypeInfo
f2xx_rng_info = {
    .name          = "f2xx_rng",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(f2xx_rng),
    .class_init    = f2xx_rng_class_init,
};

static void
f2xx_rng_register_types(void)
{
    type_register_static(&f2xx_rng_info);
}

type_init(f2xx_rng_register_types)
