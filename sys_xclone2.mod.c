#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0x11ae19b5, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x1d47defc, __VMLINUX_SYMBOL_STR(sysptr) },
	{ 0x388add66, __VMLINUX_SYMBOL_STR(filp_close) },
	{ 0x365e3533, __VMLINUX_SYMBOL_STR(filp_open) },
	{ 0x373db350, __VMLINUX_SYMBOL_STR(kstrtoint) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x587cffa0, __VMLINUX_SYMBOL_STR(vfs_write) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x19f82a02, __VMLINUX_SYMBOL_STR(vfs_read) },
	{ 0x16b021c5, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0xf29a148e, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x122e6e41, __VMLINUX_SYMBOL_STR(cpu_tss) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xeec70376, __VMLINUX_SYMBOL_STR(crypto_alloc_base) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "B8316FFF23E1910A635AC35");
