/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_MIOS_H
#define KERNEL_MIOS_H

#include <kernel.h>
#include <types.h>

struct mios_url {
	unsigned char	*schema;

	unsigned char	*path;
};

/*
 * MIOS handle
 */

#define MIOS_HANDLE_MAGIC	0x9948BDEC

struct mios_handle {
	unsigned int		magic;

	void			*private;
	struct mios_module	*module;

	struct mios_url		url;
	unsigned		flags;
};

/*
 * MIOS module
 */

#define MIOS_MODULE_MAGIC	0x11093EE2


struct mios_module_ops {
	err_t (*open)(struct mios_handle *handle);
	err_t (*close)(struct mios_handle *handle);
};

struct mios_module_info {
	unsigned char		*schema;

	unsigned char 		*module_author;
	unsigned char 		*module_license;

	unsigned 		remote;
};
	
struct mios_module {
	unsigned int		magic;

	struct mios_module_info info;
	struct mios_module_ops	ops;

	void			*private;
};






/* From MIOS/init.c */
__init__ err_t mios_init(void);

/* From MIOS/path.c */
err_t mios_url_explode(unsigned char *url, struct mios_url *exploded);
err_t mios_path_explode(unsigned char *path, unsigned char **new_path, unsigned char **buffer);
inline void mios_url_free(struct mios_url *url);

/* From MIOS/ops.c */
err_t mios_open(unsigned char *url, unsigned flags, handle_t ret_handle);
err_t mios_close(handle_t handle);

/* From MIOS/module.c */
err_t mios_module_add(struct mios_module *module);
void mios_module_enumerate(int (*enumfunc)(struct mios_module_info info));
struct mios_module *mios_module_lookup(unsigned char *schema);



#endif /* !defined KERNEL_MIOS_H */
