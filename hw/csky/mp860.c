#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "target/csky/cpu.h"
#include "hw/csky/csky.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "net/net.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "sysemu/block-backend.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "hw/csky/cskydev.h"
#include "hw/char/csky_uart.h"
#include "hw/timer/csky_mptimer.h"

static struct csky_boot_info mp860_binfo = {
    .loader_start   = 0,
    .dtb_addr       = 0x8f000000,
    .magic          = 0x20150401,
    .freq           = 50000000ll,
};

static void mp860_init(MachineState *machine)
{
    ObjectClass     *cpu_oc;
    Object          *cpuobj;
    CSKYCPU         *cpu = NULL, *last_cpu;

    DeviceState     *micdev, *mptimerdev;
    SysBusDevice    *micbusdev, *mptimerbusdev;
    int n, i;

    /*
     * Prepare RAM.
     */
    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *ram = g_new(MemoryRegion, 1);

    memory_region_allocate_system_memory(ram, NULL, "ram", 0x50000000);
    memory_region_add_subregion(sysmem, 0, ram);

    /*
     * Prepare CPU
     */
    machine->cpu_type = "ck860";
    cpu_oc = cpu_class_by_name(TYPE_CSKY_CPU, machine->cpu_type);
    if (!cpu_oc) {
            fprintf(stderr, "Unable to find CPU definition\n");
            exit(1);
    }
    for (n = 0; n < smp_cpus; n++) {
        cpuobj = object_new(object_class_get_name(cpu_oc));
        CPU(cpuobj)->cpu_index = n;
        /*add timer for each core here */
        object_property_set_bool(cpuobj, true, "realized", &error_fatal);
        CSKY_CPU(cpuobj)->env.features |= UNALIGNED_ACCESS;
        CSKY_CPU(cpuobj)->env.mmu_default = 1;
        if (n == 0) {
            cpu = CSKY_CPU(cpuobj);
        } else {
            last_cpu->env.next_cpu = &(CSKY_CPU(cpuobj)->env);
        }
        if (n == smp_cpus - 1) {
            CSKY_CPU(cpuobj)->env.next_cpu = &(cpu->env);
        }
        last_cpu = CSKY_CPU(cpuobj);
        csky_mic_init_cpu(&(CSKY_CPU(cpuobj)->env));
    }

    /*
     * use C-SKY MultiCore interrupt controller
     */

    micdev = qdev_create(NULL, "csky_mic");
    micbusdev = SYS_BUS_DEVICE(micdev);
    qdev_init_nofail(micdev);
    sysbus_mmio_map(micbusdev, 0, 0xfffe0000);

    uint32_t mic_cpu_base = 0xfffe8000;
    for (i = 0; i < smp_cpus; i++) {
        sysbus_mmio_map(micbusdev, i + 1, mic_cpu_base);
        mic_cpu_base += 0x1000;

    }
    DeviceState *cpudev = DEVICE(cpu);
    last_cpu = cpu;
    for (i = 0; i < smp_cpus; i++) {
        sysbus_connect_irq(micbusdev, i, qdev_get_gpio_in(cpudev, 0));
        sysbus_connect_irq(micbusdev, i + smp_cpus,
            qdev_get_gpio_in(cpudev, 1));
        last_cpu = csky_env_get_cpu(last_cpu->env.next_cpu);
        cpudev = DEVICE(last_cpu);
    }
    /*
     * use C-SKY MultiCore timer
     */

    mptimerdev = qdev_create(NULL, "csky_mptimer");
    mptimerbusdev = SYS_BUS_DEVICE(mptimerdev);
    qdev_init_nofail(mptimerdev);
    sysbus_mmio_map(mptimerbusdev, 0, 0xffffd000);

    last_cpu = cpu;
    for (n = 0; n < smp_cpus; n++) {
        sysbus_connect_irq(mptimerbusdev, n,
          qdev_get_gpio_in(micdev, n == 0 ? 16 : 16 * (n - 1) + 224));
        last_cpu->env.mptimerdev = mptimerdev;
        last_cpu = csky_env_get_cpu(last_cpu->env.next_cpu);
    }
    /*
     * use virtio-mmio
     */
    for (i = 0; i < 8; i++) {
        sysbus_create_simple("virtio-mmio", 0xffff0000 + i * 0x1000,
            qdev_get_gpio_in(micdev, 50 + i));
    }

    csky_uart_create(0xffffe000, qdev_get_gpio_in(micdev, 43),
    serial_hd(0));

    sysbus_create_simple("csky_exit", 0xffffc000, NULL);

    mp860_binfo.kernel_filename = machine->kernel_filename;
    csky_load_kernel(cpu, &mp860_binfo);
}

static void mp860_class_init(ObjectClass *oc, void *data)
{
    MACHINE_CLASS(oc)->desc = "C-SKY QEMU mp860 machine";
    MACHINE_CLASS(oc)->init = mp860_init;
    MACHINE_CLASS(oc)->max_cpus = 128;
}

static const TypeInfo mp860_type = {
    .name           = MACHINE_TYPE_NAME("mp860"),
    .parent         = TYPE_MACHINE,
    .class_init     = mp860_class_init,
};

static void mp860_machine_init(void)
{
    type_register_static(&mp860_type);
}

type_init(mp860_machine_init)

