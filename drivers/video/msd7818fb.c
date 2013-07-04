// Based on simplefb

#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <video/msd7818fb.h>

static struct fb_fix_screeninfo msd7818_fix = {
    .id		= "msd7818",
    .type		= FB_TYPE_PACKED_PIXELS,
    .visual		= FB_VISUAL_TRUECOLOR,
    .accel		= FB_ACCEL_NONE,
};

static struct fb_var_screeninfo msd7818_var = {
    .height		= -1,
    .width		= -1,
    .activate	= FB_ACTIVATE_NOW,
    .vmode		= FB_VMODE_NONINTERLACED,
};

// TODO: remove hardcodes
#define screen_w (1920)
#define screen_h (1080)
#define frame_size (1920 * 1080 * 2)

inline void draw_pixel_screen(void *dst, int x, int y, __u16 col) {
    ((__u16*)dst)[ y * screen_w + x ] = col;
}

// Copies data from primary to secondary buffer
static void update_second_buffer(struct fb_info * info, int sx, int sy, int ex, int ey) {
    if (sx > ex || sy > ey) {
        pr_err("%s(): Unable to update buffer\n", __FUNCTION__);
        return;
    }

    for (; sy < ey; ++sy)
        memcpy(info->screen_base + frame_size + (sy * screen_w + sx) * 2,
               info->screen_base + (sy * screen_w + sx) * 2, (ex - sx) * 2);
}

static void fillrect(struct fb_info *info, const struct fb_fillrect *rect) {
    cfb_fillrect(info, rect);
    update_second_buffer(info, rect->dx, rect->dy, rect->dx + rect->width, rect->dy + rect->height);
}

// TODO: check if working properly
static void copyarea(struct fb_info *info, const struct fb_copyarea *area) {
    pr_err("%s()\n", __FUNCTION__);
    cfb_copyarea(info, area);
    update_second_buffer(info, area->dx, area->dy, area->dx + area->width, area->dy + area->height);
}

static void imageblit(struct fb_info *info, const struct fb_image *image) {
    int x = image->dx, y = image->dy;
    if (image->depth != 1)
        cfb_imageblit(info, image);
    else {
        __u32 *pal = info->pseudo_palette;
        __u16 fg = 0xFFFF, bg = 0xFFFF;
        if (image->fg_color < 16)
            fg = pal[image->fg_color];
        if (image->bg_color < 16)
            bg = pal[image->bg_color];

        for (y = 0; y < image->height; ++y) {
            for (x = 0; x < image->width; ++x) {
                if (image->data[(y * image->width + x) / 8] & (1 << (7 - (x % 8))))
                    draw_pixel_screen(info->screen_base, x + image->dx, y + image->dy, fg);
                else
                    draw_pixel_screen(info->screen_base, x + image->dx, y + image->dy, bg);
            }
        }
    }
    // copy to bottom buffer
    update_second_buffer(info, image->dx, image->dy, image->dx + image->width, image->dy + image->height);
}

static ssize_t fb_read(struct fb_info *info, char __user *buf,
           size_t count, loff_t *ppos) {
    if (count > frame_size)
        return -EFBIG;
    copy_to_user(buf, info->screen_base, count);
    pr_err("%s()\n", __FUNCTION__);
    return count;
}

static ssize_t fb_write(struct fb_info *info, const char __user *buf,
            size_t count, loff_t *ppos) {
    if (count > frame_size)
        return -EFBIG;
    copy_from_user(info->screen_base, buf, count);
    copy_from_user(info->screen_base+frame_size, buf, count);
    pr_err("%s()\n", __FUNCTION__);
    return count;
}

// For now disallow mmap()
static int fb_mmap(struct fb_info *fbi, struct vm_area_struct *vma) {
    return -EINVAL;
}


static int fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                  u_int transp, struct fb_info *info)
{
    u32 *pal = info->pseudo_palette;

    if (regno >= 16)
        return -EINVAL;

    pal[regno] = ( 0xF000 | ((red << 4) & 0xF00) |
                   (green & 0xF0) |
                   ((blue >> 4) & 0xF) );

    return 0;
}

static struct fb_ops msd7818fb_ops = {
    .owner		= THIS_MODULE,
    .fb_fillrect	= fillrect,
    .fb_copyarea	= copyarea,
    .fb_imageblit	= imageblit,
    .fb_read = fb_read,
    .fb_write = fb_write,
    .fb_mmap = fb_mmap,
    .fb_setcolreg = fb_setcolreg,
};

struct video_mode {
    const char *name;
    u32 bits_per_pixel;
    struct fb_bitfield red;
    struct fb_bitfield green;
    struct fb_bitfield blue;
    struct fb_bitfield transp;
};

static struct video_mode video_modes[] = {
    { "argb4444", 16, {7, 4}, {4, 4}, {0, 4}, {0, 0} },
};

static int fb_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct fb_info *info;
    struct plat_msd7818_fb * pdata = NULL;

    pdata = dev_get_platdata(&pdev->dev);

    if (!pdata) {
        dev_err(&pdev->dev, "No platform data supplied - unable to initialize.\n");
        return -ENODEV;
    }

    info = framebuffer_alloc(sizeof(u32) * 16, &pdev->dev);
    if (!info) {
        dev_err(&pdev->dev, "Failed to allocate framebuffer.\n");
        return -ENOMEM;
    }
    info->pseudo_palette = (void *)(info + 1);

    platform_set_drvdata(pdev, info);

    info->fix = msd7818_fix;
    info->fix.smem_start = pdata->iobase;
    info->fix.smem_len = pdata->size;
    info->fix.line_length = pdata->width * pdata->depth / 8;

    info->var = msd7818_var;
    info->var.xres = pdata->width;
    info->var.yres = pdata->height;
    info->var.xres_virtual = pdata->width;
    info->var.yres_virtual = pdata->height;
    info->var.bits_per_pixel = pdata->depth;
    if (pdata->mode < ARRAY_SIZE(video_modes)) {
        info->var.red = video_modes[pdata->mode].red;
        info->var.green = video_modes[pdata->mode].green;
        info->var.blue = video_modes[pdata->mode].blue;
        info->var.transp = video_modes[pdata->mode].transp;
    } else {
        dev_err(&pdev->dev, "No valid pixel mode provided.\n");
        ret = -EINVAL;
        goto err_unmap;
    }

    info->var.nonstd = 1;

    info->fbops = &msd7818fb_ops;
    info->flags = FBINFO_DEFAULT;
    info->screen_base = devm_ioremap(&pdev->dev, pdata->iobase, pdata->size);
    info->screen_size = pdata->size;
    if (!info->screen_base) {
        dev_err(&pdev->dev, "ioremap failed.\n");
        ret = -ENOMEM;
        goto err_free_fb;
    }

    ret = register_framebuffer(info);
    if (ret < 0) {
        dev_err(&pdev->dev, "Unable to register msd7818fb: %d\n", ret);
        goto err_unmap;
    }

    dev_info(&pdev->dev, "fb%d: Probed! Resolution: %dx%d@%dbit, mode: %s, base: 0x%08lx virt: 0x%p\n",
             info->node, info->var.xres, info->var.yres, info->var.bits_per_pixel,
             video_modes[pdata->mode].name, pdata->iobase, info->screen_base);
    return 0;

err_unmap:
    devm_iounmap(&pdev->dev, info->screen_base);
err_free_fb:
    framebuffer_release(info);

    return ret;
}

static int fb_remove(struct platform_device *pdev)
{
    struct fb_info *info = platform_get_drvdata(pdev);

    unregister_framebuffer(info);
    devm_iounmap(&pdev->dev, info->screen_base);
    framebuffer_release(info);

    dev_info(&pdev->dev, "Removed");
    return 0;
}

static struct platform_driver fb_driver = {
    .probe = fb_probe,
    .remove = fb_remove,
    .driver = {
        .name = "msd7818-fb",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(fb_driver);

MODULE_AUTHOR("Mateusz Kulikowski");
MODULE_DESCRIPTION("MSD7818 u-boot framebuffer driver");
MODULE_LICENSE("GPL v2");
