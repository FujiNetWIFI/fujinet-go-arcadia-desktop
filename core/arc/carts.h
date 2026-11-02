/* carts.h -- the cartridge slot and its mappers.
 *
 * Transposed from MAME's Arcadia cartridge bus (src/devices/bus/arcadia/
 * slot.cpp, rom.cpp; BSD-3-Clause, copyright-holders Fabio Priuli -- see
 * COMPLIANCE.md).
 *
 * The slot is a set of function pointers, because the FujiNet cartridge
 * (fujinet_cart.c) sits in the same slot and the machine must not know which
 * it has. Each mapper sees exactly what MAME's device_arcadia_cart_interface
 * sees: read_rom(offset) for the console's $0000-$0FFF and extra_rom(offset)
 * for the range the driver installs over the rest of the cartridge space
 * (machine_start: $2000-$7FFF for a standard cart, $4000-$4FFF for Golf), the
 * offset relative to the start of that range.
 *
 * A read made by the debugger passes peek = 1: it must change nothing (the
 * FujiNet cart's windows have side effects; MAME's side_effects_disabled()).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * (transposed from MAME; see COMPLIANCE.md)
 */

#ifndef ARC_CARTS_H
#define ARC_CARTS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum arc_cart_type {
    ARC_CART_NONE = 0,
    ARC_CART_STD,       /* arcadia_rom_device: up to 8K, the 2nd 4K at $2000 */
    ARC_CART_GOLF,      /* arcadia_golf_device: the 2nd block shown at $4000 */
    ARC_CART_FUJINET,   /* the FujiNet cartridge (fujinet_cart.c) */
};

typedef struct arc_cart arc_cart_t;

typedef struct {
    uint8_t (*read_rom)(arc_cart_t *c, uint16_t offset, int peek);   /* $0000-$0FFF */
    uint8_t (*extra_rom)(arc_cart_t *c, uint16_t offset, int peek);  /* offset from extra_base */
} arc_cart_ops_t;

struct arc_cart {
    const arc_cart_ops_t *ops;
    int type;                       /* enum arc_cart_type */

    /* the CPU addresses extra_rom answers, [extra_lo, extra_hi] */
    uint16_t extra_lo, extra_hi;

    /* the image (not owned) */
    const uint8_t *rom;
    uint32_t rom_size;

    void *priv;                     /* fujinet: its own state */
};

/* What a plain image is: the one image MAME's software list gives the Golf
 * mapper (by its CRC-32), else a standard cart. */
int arc_cart_identify(const uint8_t *img, uint32_t size);

/* Set up c for `type` over an image of `size` bytes (not copied). Returns 0,
 * or -1 and a message in *err. */
int arc_cart_init(arc_cart_t *c, int type, const uint8_t *rom, uint32_t size, const char **err);

const char *arc_cart_type_name(int type);

/* CRC-32 (IEEE: the zip/MAME polynomial). */
uint32_t arc_crc32(const uint8_t *p, uint32_t n);

static inline uint8_t arc_cart_read_rom(arc_cart_t *c, uint16_t off, int peek)
{
    return c ? c->ops->read_rom(c, off, peek) : 0xff;
}
static inline uint8_t arc_cart_extra_rom(arc_cart_t *c, uint16_t off, int peek)
{
    return c ? c->ops->extra_rom(c, off, peek) : 0xff;
}

#ifdef __cplusplus
}
#endif

#endif /* ARC_CARTS_H */
