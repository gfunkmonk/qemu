#include "hw/ssi.h"
#include "hw/boards.h"
#include "hw/block/flash.h"
#include "sysemu/sysemu.h"
#include "sysemu/blockdev.h"
#include "ui/console.h"
#include "stm32f4xx.h"
#include "exec/address-spaces.h"

typedef struct {
  int dbgserial_uart_index;

  uint32_t gpio_idr_masks[STM32F4XX_GPIO_COUNT];

  // memory sizes
  uint32_t flash_size;
  uint32_t ram_size;

} Stm32BoardConfig;

static const Stm32BoardConfig stm32f429_disco = {
    .dbgserial_uart_index = 0,       // USART3
    .flash_size = 4096,  /* Kbytes - larger to aid in development and debugging */
    .ram_size = 256,  /* Kbytes */
};

static void stm32f429i_disco_init(MachineState *machine, const Stm32BoardConfig *board_config)
{
    Stm32Gpio *gpio[STM32F4XX_GPIO_COUNT];
    Stm32Uart *uart[STM32F4XX_UART_COUNT];
    Stm32Timer *timer[STM32F4XX_TIM_COUNT];
    DeviceState *rtc_dev;
    struct stm32f4xx stm;
    ARMCPU *cpu;

    /* add additional ram */
    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *extram = g_new(MemoryRegion, 1);

    memory_region_init_ram(extram, NULL, "stm32f4xx.extram", 0x800000, &error_fatal);
    vmstate_register_ram_global(extram);
    memory_region_add_subregion(system_memory, 0x90000000, extram);

    // Note: allow for bigger flash images (4MByte) to aid in development and debugging
    stm32f4xx_init(board_config->flash_size,
                   board_config->ram_size,
                   machine->kernel_filename,
                   gpio,
                   board_config->gpio_idr_masks,
                   uart,
                   timer,
                   &rtc_dev,
                   8000000 /*osc_freq*/,
                   32768 /*osc2_freq*/,
                   &stm,
                   &cpu);

    // /* Storage flash (NOR-flash on Snowy) */
    // const uint32_t flash_size_bytes = 16 * 1024 * 1024;  /* 16 MBytes */
    // const uint32_t flash_sector_size_bytes = 32 * 1024;  /* 32 KBytes */
    // const uint32_t bank_size_bytes = 2 * 1024 * 1024;  /* 2 MBytes */
    // DriveInfo *dinfo = drive_get(IF_PFLASH, 0, 1);   /* Use the 2nd -pflash drive */
    // if (dinfo) {
    //     pflash_jedec_424_register(
    //         0x60000000,               /* flash_base*/
    //         NULL,                     /* qdev, not used */
    //         "mx29vs128fb",            /* name */
    //         flash_size_bytes,         /* size */
    //         blk_by_legacy_dinfo(dinfo),              /* driver state */
    //         flash_sector_size_bytes,  /* sector size */
    //         flash_size_bytes / flash_sector_size_bytes, /* number of sectors */
    //         bank_size_bytes,  /* size of each bank */
    //         2,                        /* width in bytes */
    //         0x00c2, 0x007e, 0x0065, 0x0001, /* id: 0, 1, 2, 3 */
    //         0                         /* big endian */
    //     );
    // }

    // Connect up the uarts
    stm32_uart_connect(uart[board_config->dbgserial_uart_index], serial_hds[0], 0);
}

static void stm32f429i_disco_machine_init(MachineState *machine)
{
    stm32f429i_disco_init(machine, &stm32f429_disco);
}

static void stm32f429i_disco_init_machine_init(MachineClass *mc)
{
    mc->desc = "STM32F429 disco";
    mc->init = stm32f429i_disco_machine_init;
}

DEFINE_MACHINE("stm32f429i-disco", stm32f429i_disco_init_machine_init)
