; /* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
;  * This file is part of the "AmberOS" project
;  * Unauthorized copying of this file, via any medium is strictly prohibited
;  * Proprietary and confidential
;  * Written by Michał Pazurek <michal10.05.pk@gmail.com>
; */

[BITS 16]
[ORG 0x7C00]

mov ah, 0x00
mov al, 0x03
int 0x10

times 510 - ($-$$) db 0
dw 0xAA55
