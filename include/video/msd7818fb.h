#ifndef MSD7818_FB_H
#define MSD7818_FB_H
#include <linux/io.h>
#include <linux/types.h>

#define MODE_ARGB4444 0
struct plat_msd7818_fb {
    unsigned long	iobase;		/* io base address */
    void __iomem	*membase;	/* ioremap cookie or NULL */
    resource_size_t	size;	/* framebuffer size */
    __u16 width;
    __u16 height;
    __u8 depth;
    __u8 mode;
    __u8 duplicate_paints; // if set - duplicate frame
};


#endif
