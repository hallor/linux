// Based on simplefb

#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <video/msd7818fb.h>

static struct fb_fix_screeninfo simplefb_fix = {
    .id		= "msd7818",
    .type		= FB_TYPE_PACKED_PIXELS,
    .visual		= FB_VISUAL_TRUECOLOR,
    .accel		= FB_ACCEL_NONE,
};

static struct fb_var_screeninfo simplefb_var = {
    .height		= -1,
    .width		= -1,
    .activate	= FB_ACTIVATE_NOW,
    .vmode		= FB_VMODE_NONINTERLACED,
};

__u16 color2(int r, int g, int b) {
    return ( 0xF000 | ((r << 4) & 0xF00) |
             (g & 0xF0) |
             ((b >> 4) & 0xF) );
}

#define screen_w (1920)
#define screen_h (1080)
#define frame_size (1920 * 1080 * 2)
static void * screen = (void*) 0xa2100000;

inline void memset32(void *s, __u32 __c, size_t __n) {
    __u32 * __s = s;
    while (__n--) {
        *__s++ = __c;
    }
}

void screen_line(int sx, int sy, int w, __u16 col)
{
    __u32 c = col | (col << 16);
    if (w > 1) {
        memset32(screen + (sy * screen_w + sx) * 2, c, (w - (w % 2)) / 2);
        memset32(screen + frame_size  + (sy * screen_w + sx) * 2, c, (w - (w % 2)) / 2);
    }

    if (w % 2) {
        *(__u16*)(screen + (sy * screen_w + sx + w - 1) * 2) = col;
        *(__u16*)(screen + frame_size  + (sy * screen_w + sx + w - 1) * 2) = col;
    }
}

// src must be screen size
void screen_line_blit(void * src, int sx, int sy, int dx, int dy, int w)
{
    memcpy(screen + (dy * screen_w + dx) * 2,
           src + (sy * screen_w + sx) * 2, w * 2);
    memcpy(screen + frame_size + (dy * screen_w + dx) * 2,
           src + (sy * screen_w + sx) * 2, w * 2);
}

inline void draw_pixel_screen(void *dst, int x, int y, __u16 col) {
    ((__u16*)dst)[ y * screen_w + x ] = col;
    ((__u16*)(dst + frame_size))[ y * screen_w + x ] = col;
}

static void fillrect(struct fb_info *info, const struct fb_fillrect *rect) {
    int line = 0;
    pr_err("%s() (%dx%d) [%dx%d] (%x)\n", __FUNCTION__,
           rect->dx, rect->dy, rect->width, rect->height, rect->color);
    for (line=rect->dy; line < rect->dy + rect->height; ++line)
        screen_line(rect->dx, line, rect->width, rect->color);
//    cfb_fillrect(info, rect);
}

static void copyarea(struct fb_info *info, const struct fb_copyarea *area) {
    pr_err("%s()\n", __FUNCTION__);
    cfb_copyarea(info, area);
}

void screen_line_copy(void * dst, void * src, int sx, int sy, int dx, int dy, int w)
{
    memcpy(dst + (dy * screen_w + dx) * 2,
           src + (sy * screen_w + sx) * 2, w * 2);
}

static void imageblit(struct fb_info *info, const struct fb_image *image) {
    int x = image->dx, y = image->dy;
    if (image->depth != 1)
        cfb_imageblit(info, image);
    else {
        for (y = 0; y < image->height; ++y) {
            for (x = 0; x < image->width; ++x) {
                if (image->data[(y * image->width + x) / 8] & (1 << (7 - (x % 8))))
                    draw_pixel_screen(screen, x + image->dx, y + image->dy, 0xFFFF);
                else
                    draw_pixel_screen(screen, x + image->dx, y + image->dy, 0x0);
            }
        }
    }
    // copy to bottom buffer
    for (y = image->dy; y < image->dy + image->height; ++y)
        screen_line_copy(screen + frame_size, screen, image->dx, y, image->dx, y, image->width);
}

ssize_t fb_read(struct fb_info *info, char __user *buf,
           size_t count, loff_t *ppos) {
    if (count > frame_size)
        return -EFBIG;
    copy_to_user(buf, screen, count);
    pr_err("%s()\n", __FUNCTION__);
    return count;
}

ssize_t fb_write(struct fb_info *info, const char __user *buf,
            size_t count, loff_t *ppos) {
    if (count > frame_size)
        return -EFBIG;
    copy_from_user(screen, buf, count);
    copy_from_user(screen+frame_size, buf, count);
    pr_err("%s()\n", __FUNCTION__);
    return count;
}

static struct fb_ops msd7818fb_ops = {
    .owner		= THIS_MODULE,
    .fb_fillrect	= fillrect,
    .fb_copyarea	= copyarea,
    .fb_imageblit	= imageblit,
    .fb_read = fb_read,
    .fb_write = fb_write,
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
    { "Xrgb4444", 16, {7, 4}, {4, 4}, {0, 4}, {0, 0} },
};

struct simplefb_params {
    u32 width;
    u32 height;
    u32 stride;
    struct video_mode *mode;
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

    info = framebuffer_alloc(0, &pdev->dev);
    if (!info) {
        dev_err(&pdev->dev, "Failed to allocate framebuffer.\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, info);

    info->fix = simplefb_fix;
    info->fix.smem_start = pdata->iobase;
    info->fix.smem_len = pdata->size;
    info->fix.line_length = pdata->width * pdata->depth / 8;

    info->var = simplefb_var;
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
