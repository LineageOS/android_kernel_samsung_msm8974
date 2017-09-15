#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x4a49528b, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x9d36c454, __VMLINUX_SYMBOL_STR(kernel_kobj) },
	{ 0x1360c042, __VMLINUX_SYMBOL_STR(kobject_uevent) },
	{ 0xf0567051, __VMLINUX_SYMBOL_STR(kobject_put) },
	{ 0x60a158b, __VMLINUX_SYMBOL_STR(kobject_init_and_add) },
	{ 0x190f2071, __VMLINUX_SYMBOL_STR(kset_create_and_add) },
	{ 0x1616c8bc, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xeb71ec2a, __VMLINUX_SYMBOL_STR(kobject_uevent_env) },
	{ 0xdaf6b505, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

