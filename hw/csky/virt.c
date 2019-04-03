#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "target/csky/cpu.h"
#include "hw/csky/csky.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "net/net.h"
#include "sysemu/device_tree.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "sysemu/block-backend.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "hw/csky/cskydev.h"
#include "hw/char/csky_uart.h"
#include "hw/timer/csky_mptimer.h"

#include <libfdt.h>
static struct csky_boot_info virt_binfo = {
    .loader_start   = 0,
    .dtb_addr       = 0x8f000000,
    .magic          = 0x20150401,
    .freq           = 50000000ll,
};

static void *create_smp_fdt(void)
{
    void *fdt;
    int i, cpu;
    char *nodename;
    int fdt_size;

    fdt = create_device_tree(&fdt_size);
    if (!fdt) {
        error_report("create_device_tree() failed");
        exit(1);
    }

    qemu_fdt_setprop_string(fdt, "/", "model", "csky ck860_platform");
    qemu_fdt_setprop_string(fdt, "/", "compatible", "csky,ck860_platform");
    qemu_fdt_setprop_cell(fdt, "/", "#size-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/", "#address-cells", 0x1);

    qemu_fdt_add_subnode(fdt, "/soc");
    qemu_fdt_setprop(fdt, "/soc", "ranges", NULL, 0);
    qemu_fdt_setprop_string(fdt, "/soc", "compatible", "simple-bus");
    qemu_fdt_setprop_cell(fdt, "/soc", "#size-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/soc", "#address-cells", 0x1);

    qemu_fdt_add_subnode(fdt, "/aliases");
    qemu_fdt_setprop_string(fdt, "/aliases", "serial0", "/soc/serial@ffffe000");

    nodename = g_strdup_printf("/memory");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0x0, 0x50000000);
    qemu_fdt_setprop_string(fdt, nodename, "device_type", "memory");
    g_free(nodename);

    qemu_fdt_add_subnode(fdt, "/cpus");
    qemu_fdt_setprop_cell(fdt, "/cpus", "#size-cells", 0x0);
    qemu_fdt_setprop_cell(fdt, "/cpus", "#address-cells", 0x1);

    for (cpu = smp_cpus - 1; cpu >= 0; cpu--) {
        nodename = g_strdup_printf("/cpus/cpu@%d", cpu);
        qemu_fdt_add_subnode(fdt, nodename);
        qemu_fdt_setprop_string(fdt, nodename, "status", "enable");
        qemu_fdt_setprop_cell(fdt, nodename, "reg", 0x0);
        qemu_fdt_setprop_string(fdt, nodename, "device_type", "cpu");
        g_free(nodename);
    }

    nodename = g_strdup_printf("/soc/interrupt-controller");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_cell(fdt, nodename, "#interrupt-cells", 1);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "csky,mpintc");
    qemu_fdt_setprop(fdt, nodename, "interrupt-controller", NULL, 0);
    qemu_fdt_setprop_cells(fdt, nodename, "phandle", 0x2);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/timer");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "csky,mptimer");
    qemu_fdt_setprop_cells(fdt, nodename, "clocks", 0x1);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 0x10);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
    g_free(nodename);


    nodename = g_strdup_printf("/soc/serial@ffffe000");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "ns16550a");
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0xffffe000, 0x1000);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 0x2b);
    qemu_fdt_setprop_cells(fdt, nodename, "clocks", 0x1);
    qemu_fdt_setprop_string(fdt, nodename, "clock-names", "baudclk");
    qemu_fdt_setprop_cells(fdt, nodename, "reg-shift", 0x2);
    qemu_fdt_setprop_cells(fdt, nodename, "reg-io-width", 0x4);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/apb-clock");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "fixed-clock");
    qemu_fdt_setprop_cells(fdt, nodename, "clock-frequency", 0x2faf080);
    qemu_fdt_setprop_string(fdt, nodename, "clock-output-names", "dummy_apb");
    qemu_fdt_setprop_cells(fdt, nodename, "#clock-cells", 0x0);
    qemu_fdt_setprop_cells(fdt, nodename, "phandle", 0x1);
    g_free(nodename);

   for (i = 0; i < 8; i++) {
        nodename = g_strdup_printf("/soc/virtio_mmio@%x",
            0xffff0000 + i * 0x1000);
        qemu_fdt_add_subnode(fdt, nodename);
        qemu_fdt_setprop_string(fdt, nodename, "compatible", "virtio,mmio");
        qemu_fdt_setprop_cells(fdt, nodename, "reg",
            0xffff0000 + i * 0x1000, 0x1000);
        qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
        qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 50 + i);
        g_free(nodename);
    }

    qemu_fdt_add_subnode(fdt, "/chosen");
    //"console=ttyS0,115200
    qemu_fdt_setprop_string(fdt, "/chosen", "bootargs",
    "console=ttyS0,115200 \
     rdinit=/sbin/init root=/dev/ram0 \
     clk_ignore_unused loglevel=7");
    return fdt;
}

static void *create_fdt(void)
{
    void *fdt;
    int i, cpu;
    char *nodename;
    int fdt_size;

    fdt = create_device_tree(&fdt_size);
    if (!fdt) {
        error_report("create_device_tree() failed");
        exit(1);
    }

    qemu_fdt_setprop_string(fdt, "/", "model", "csky");
    qemu_fdt_setprop_string(fdt, "/", "compatible", "csky");
    qemu_fdt_setprop_cell(fdt, "/", "#size-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/", "#address-cells", 0x1);

    qemu_fdt_add_subnode(fdt, "/soc");
    qemu_fdt_setprop(fdt, "/soc", "ranges", NULL, 0);
    qemu_fdt_setprop_string(fdt, "/soc", "compatible", "simple-bus");
    qemu_fdt_setprop_cell(fdt, "/soc", "#size-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/soc", "#address-cells", 0x1);

    nodename = g_strdup_printf("/memory");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0x0, 0x40000000);
    qemu_fdt_setprop_string(fdt, nodename, "device_type", "memory");
    g_free(nodename);

    qemu_fdt_add_subnode(fdt, "/cpus");
    qemu_fdt_setprop_cell(fdt, "/cpus", "#size-cells", 0x0);
    qemu_fdt_setprop_cell(fdt, "/cpus", "#address-cells", 0x0);

    for (cpu = smp_cpus - 1; cpu >= 0; cpu--) {
        nodename = g_strdup_printf("/cpus/cpu@%d", cpu);
        qemu_fdt_add_subnode(fdt, nodename);
        qemu_fdt_setprop_string(fdt, nodename, "compatible", "csky");
        qemu_fdt_setprop_cell(fdt, nodename, "ccr", 0x417d);
        qemu_fdt_setprop_cell(fdt, nodename, "hint", 0xe);
        qemu_fdt_setprop_string(fdt, nodename, "device_type", "cpu");
        g_free(nodename);
    }

    nodename = g_strdup_printf("/soc/interrupt-controller");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_cell(fdt, nodename, "#interrupt-cells", 1);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "csky,apb-intc");
    qemu_fdt_setprop(fdt, nodename, "interrupt-controller", NULL, 0);
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0xfffff000, 0x1000);
    qemu_fdt_setprop_cells(fdt, nodename, "phandle", 0x2);
    qemu_fdt_setprop_cells(fdt, nodename, "linux,phandle", 0x2);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/timer0");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "snps,dw-apb-timer");
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0xffffd000, 0x1000);
    qemu_fdt_setprop_cells(fdt, nodename, "clocks", 0x1);
    qemu_fdt_setprop_string(fdt, nodename, "clock-names", "timer");
    qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 0x1);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/timer1");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "snps,dw-apb-timer");
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0xffffd014, 0x800);
    qemu_fdt_setprop_cells(fdt, nodename, "clocks", 0x1);
    qemu_fdt_setprop_string(fdt, nodename, "clock-names", "timer");
    qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 0x2);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/serial0");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "ns16550a");
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0xffffe000, 0x1000);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 0x3);
    qemu_fdt_setprop_cells(fdt, nodename, "clocks", 0x1);
    qemu_fdt_setprop_cells(fdt, nodename, "baud", 0x1c200);
    qemu_fdt_setprop_cells(fdt, nodename, "reg-shift", 0x2);
    qemu_fdt_setprop_cells(fdt, nodename, "reg-io-width", 0x1);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/apb-clock");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "fixed-clock");
    qemu_fdt_setprop_cells(fdt, nodename, "clock-frequency", 0x2625a00);
    qemu_fdt_setprop_string(fdt, nodename, "clock-output-names", "dummy_apb");
    qemu_fdt_setprop_cells(fdt, nodename, "#clock-cells", 0x0);
    qemu_fdt_setprop_cells(fdt, nodename, "linux,phandle", 0x1);
    qemu_fdt_setprop_cells(fdt, nodename, "phandle", 0x1);
    g_free(nodename);

    nodename = g_strdup_printf("/soc/qemu-exit");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "csky,qemu-exit");
    qemu_fdt_setprop_cells(fdt, nodename, "reg", 0xffffc000, 0x1000);
    g_free(nodename);

    for (i = 0; i < 8; i++) {
        nodename = g_strdup_printf("/soc/virtio_mmio%x",
            0xffff0000 + i * 0x1000);
        qemu_fdt_add_subnode(fdt, nodename);
        qemu_fdt_setprop_string(fdt, nodename, "compatible", "virtio,mmio");
        qemu_fdt_setprop_cells(fdt, nodename, "reg",
            0xffff0000 + i * 0x1000, 0x1000);
        qemu_fdt_setprop_cells(fdt, nodename, "interrupt-parent", 0x2);
        qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 10 + i);
        g_free(nodename);
    }

    qemu_fdt_add_subnode(fdt, "/chosen");
    qemu_fdt_setprop_string(fdt, "/chosen", "bootargs",
    "console=ttyS0,115200 rdinit=/sbin/init root=/dev/ram0");
    return fdt;
}

static bool get_ramsize_from_dtb(unsigned int *ram_size, unsigned int *base)
{
    void *fdt = NULL;
    int size;
    int lenp = 0;
    unsigned int *res;
    virt_binfo.dtb_filename = qemu_opt_get(qemu_get_machine_opts(), "dtb");
    if (virt_binfo.dtb_filename) {
        char *filename;
        filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, virt_binfo.dtb_filename);
        if (!filename) {
            fprintf(stderr, "Couldn't open dtb file %s\n",
                    virt_binfo.dtb_filename);
            return false;
        }

        fdt = load_device_tree(filename, &size);
        if (!fdt) {
            fprintf(stderr, "Couldn't open dtb file %s\n", filename);
            g_free(filename);
            return false;
        }
        g_free(filename);
    } else {
        fprintf(stderr, "Board was unable to create a dtb blob\n");
        return false;
    }

    qemu_fdt_dumpdtb(fdt, size);

    /* get reg<base size> to point "res" */
    res = (unsigned int *)
        qemu_fdt_getprop(fdt, "/memory", "reg", &lenp, &error_fatal);

    if (lenp == 8) {
        *base = __builtin_bswap32(*res);
        *ram_size = __builtin_bswap32(*(res + 1));
        return true;
    } else {
        return false;
    }
}

static void mp860_init(MachineState *machine)
{
    ObjectClass     *cpu_oc;
    Object          *cpuobj;
    CSKYCPU         *cpu = NULL, *last_cpu;

    DeviceState     *micdev, *mptimerdev;
    SysBusDevice    *micbusdev, *mptimerbusdev;
    int n, i;
    void *fdt;
    /* load dtb or create fdt */
    virt_binfo.dtb_filename = qemu_opt_get(qemu_get_machine_opts(), "dtb");
    if (!virt_binfo.dtb_filename) {
        fdt = create_smp_fdt();
        ram_size = 0x50000000;
        qemu_fdt_dumpdtb(fdt, fdt_totalsize(fdt));
        rom_add_blob_fixed_as("mrom.fdt", fdt, fdt_totalsize(fdt),
                virt_binfo.dtb_addr & 0x1fffffff,&address_space_memory);
    }
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

    virt_binfo.kernel_filename = machine->kernel_filename;
    csky_load_kernel(cpu, &virt_binfo);
}

static void virt_init(MachineState *machine)
{
        Object          *cpuobj;
        CSKYCPU         *cpu;

        DeviceState     *intc;
        int i;
        unsigned int ram_size =0, base = 0;
        void *fdt;

        if (machine->cpu_type) {
            if (strstr(machine->cpu_type, "860")) {
                mp860_init(machine);
                return;
            }
        }
        if (smp_cpus > 1) {
            mp860_init(machine);
            return;
        }

        /* load dtb or create fdt */
        virt_binfo.dtb_filename = qemu_opt_get(qemu_get_machine_opts(), "dtb");
        if (virt_binfo.dtb_filename) {
            /* read ram base and size from dtb file. */
            if (!get_ramsize_from_dtb(&ram_size, &base)) {
                fprintf(stderr,
                    "qemu: fail to read RAM base/size in dtb '%s'\n",
                    virt_binfo.dtb_filename);
                exit(1);
            }
        } else {
            fdt = create_fdt();
            ram_size = 0x40000000;
            qemu_fdt_dumpdtb(fdt, fdt_totalsize(fdt));
            rom_add_blob_fixed_as("mrom.fdt", fdt, fdt_totalsize(fdt),
                virt_binfo.dtb_addr & 0x1fffffff,&address_space_memory);
        }
        /*
         * Prepare RAM.
         */
        MemoryRegion *sysmem = get_system_memory();
        MemoryRegion *ram = g_new(MemoryRegion, 1);

        memory_region_allocate_system_memory(ram, NULL, "ram", ram_size);
        memory_region_add_subregion(sysmem, base, ram);

        cpuobj = object_new(machine->cpu_type);

        object_property_set_bool(cpuobj, true, "realized", &error_fatal);

        cpu = CSKY_CPU(cpuobj);

        /*
         * use C-SKY interrupt controller
         */
        intc = sysbus_create_simple(
                        "csky_intc",
                        0xfffff000,
                        *csky_intc_init_cpu(&cpu->env));

        /*
         * use dw-apb-timer
         */
        csky_timer_set_freq(virt_binfo.freq);
        sysbus_create_varargs(
                        "csky_timer",
                        0xffffd000,
                        qdev_get_gpio_in(intc, 1),
                        qdev_get_gpio_in(intc, 2),
                        NULL);

        /*
         * use 16650a uart.
         */
        csky_uart_create(
                        0xffffe000,
                        qdev_get_gpio_in(intc, 3),
                        serial_hd(0));

        /*
         * for qemu exit, use cmd poweroff.
         */
        sysbus_create_simple("csky_exit", 0xffffc000, NULL);

        if (virt_binfo.dtb_filename) {
            /*
             * add net, io-len is 2K.
             */
            csky_mac_v2_create(&nd_table[0], 0xffffa000, qdev_get_gpio_in(intc, 4));
        }
        /*
         * use virtio-mmio
         */
        for (i = 0; i < 8; i++) {
            sysbus_create_simple("virtio-mmio", 0xffff0000 + i * 0x1000,
                qdev_get_gpio_in(intc, 10 + i));
        }
        /*
         * boot up kernel with unaligned_access and mmu on.
         */
#ifdef TARGET_CSKYV2
        cpu->env.features |= UNALIGNED_ACCESS;
#endif
        cpu->env.mmu_default = 1;

        virt_binfo.kernel_filename = machine->kernel_filename;
        csky_load_kernel(cpu, &virt_binfo);
}

static void virt_class_init(ObjectClass *oc, void *data)
{
        MACHINE_CLASS(oc)->desc = "C-SKY QEMU virt machine";
        MACHINE_CLASS(oc)->init = virt_init;
        MACHINE_CLASS(oc)->max_cpus = 128;
#ifdef TARGET_CSKYV2
        MACHINE_CLASS(oc)->default_cpu_type = CSKY_CPU_TYPE_NAME("ck810f");
#else
        MACHINE_CLASS(oc)->default_cpu_type = CSKY_CPU_TYPE_NAME("ck610ef");
#endif
}

static const TypeInfo virt_type = {
        .name           = MACHINE_TYPE_NAME("virt"),
        .parent         = TYPE_MACHINE,
        .class_init     = virt_class_init,
};

static void virt_machine_init(void)
{
        type_register_static(&virt_type);
}

type_init(virt_machine_init)

