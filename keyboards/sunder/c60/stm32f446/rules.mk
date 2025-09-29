CUSTOM_MATRIX = yes

VPATH += keyboards/sunder/common \
		 keyboards/sunder/common/stm32f446
SRC += matrix.c he_adc.c he_debug.c he_keys.c he_matrix.c he_sync.c lut.c

OPT = 3