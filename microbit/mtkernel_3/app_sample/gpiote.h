/*
 *  GPIOTE レジスタ定義
 *
 *  Copyright (C) 2010-2023 Personal Media Corporation
 */

#define GPIOTE(r)       (GPIOTE_BASE + GPIOTE_##r)
#define GPIOTE_BASE     0x40006000
#define GPIOTE_TASKS_OUT(n)    (0x000 + (n) * 4)
#define GPIOTE_TASKS_SET(n)    (0x030 + (n) * 4)
#define GPIOTE_TASKS_CLR(n)    (0x060 + (n) * 4)
#define GPIOTE_EVENTS_IN(n)    (0x100 + (n) * 4)
#define GPIOTE_EVENTS_PORT     0x17c
#define GPIOTE_INTENSET        0x304
#define GPIOTE_INTENCLR        0x308
#define GPIOTE_CONFIG(n)       (0x510 + (n) * 4)
