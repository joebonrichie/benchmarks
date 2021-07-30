#!/bin/bash

perf stat hb-shape -o /dev/null /usr/share/fonts/truetype/noto-sans-ttf/NotoKufiArabic-Regular.ttf --text-file fa-thelittleprince.txt --num-iterations=1000 --font-funcs=ot
perf stat hb-shape -o /dev/null /usr/share/fonts/truetype/noto-sans-ttf/NotoKufiArabic-Regular.ttf --text-file fa-thelittleprince.txt --num-iterations=1000 --font-funcs=ft
perf stat hb-shape -o /dev/null /usr/share/fonts/truetype/noto-sans-ttf/NotoSans-Regular.ttf --text-file en-thelittleprince.txt --num-iterations=1000 --font-funcs=ot
perf stat hb-shape -o /dev/null /usr/share/fonts/truetype/noto-sans-ttf/NotoSans-Regular.ttf --text-file en-thelittleprince.txt --num-iterations=1000 --font-funcs=ft
