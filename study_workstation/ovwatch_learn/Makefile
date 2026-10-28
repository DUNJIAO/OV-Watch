# ============================================================
# OV-Watch 学习工程（PC 轨）
#   - 目录结构照抄参考工程：User/ 放应用，BSP/ 放硬件驱动
#   - BSP/ 是"真机驱动"，由 Keil 编译烧进手表，PC 这一份不编译它
#     （PC 版的数据来源是 User/Func/HWDataAccess.c 里的 #else 分支）
#   - LVGL / lv_drivers 直接复用参考工程，不复制、不修改
#     （对应参考工程的 Middlewares/LVGL）
#   - 用法： mingw32-make -j4      然后运行 bin/ovwatch_learn.exe
# ============================================================

# GNU Make 在 Windows 上默认用 cmd.exe 跑命令，这里切到 Git Bash
SHELL := C:/PROGRA~1/Git/usr/bin/bash.exe

CC      := gcc
REF     := ../../lv_sim_vscode_win
SDL2DIR := ../tools/SDL2-2.30.9/x86_64-w64-mingw32
BUILD   := build
BIN     := bin/ovwatch_learn.exe

CFLAGS  := -std=gnu11 -O2 -g -MMD -MP \
           -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
           -DLV_CONF_INCLUDE_SIMPLE \
           -I. -IUser -IUser/GUI_App -IUser/Func/Inc -IBSP \
           -I$(REF) -I$(REF)/lvgl -I$(SDL2DIR)/include
LDFLAGS := -L$(SDL2DIR)/lib -lmingw32 -lSDL2 -lm

# 递归收集某个目录下的 .c 文件
rwildcard = $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2)$(filter $(subst *,%,$2),$d))

# 参考工程里已经生成好的字体和图片资源（纯数据文件，直接复用）
ASSET_SRC := $(call rwildcard,$(REF)/user_test/GUI_App/Fonts,*.c) \
             $(call rwildcard,$(REF)/user_test/GUI_App/IMGs,*.c)

# 注意：这里没有 BSP/。BSP 里是 HAL 代码，PC 上编不过（没有 stm32f4xx_hal.h）。
# 真机 BSP 由 Software/OV_Watch/MDK-ARM 的 Keil 工程负责编译。
SRC := main.c \
       $(call rwildcard,User,*.c) \
       $(ASSET_SRC) \
       $(call rwildcard,$(REF)/lvgl/src,*.c) \
       $(REF)/lv_drivers/sdl/sdl.c \
       $(REF)/mouse_cursor_icon.c

# 把所有目标文件平铺到 build/，避免在 Windows 下创建多级目录
objname = $(BUILD)/$(subst /,_,$(subst ../,,$(patsubst %.c,%,$1))).o

OBJ := $(foreach s,$(SRC),$(call objname,$(s)))

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	cp "$(SDL2DIR)/bin/SDL2.dll" bin/

# 为每个源文件生成一条显式编译规则
define COMPILE_RULE
$(call objname,$(1)): $(1)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $(1) -o $$@
endef
$(foreach s,$(SRC),$(eval $(call COMPILE_RULE,$(s))))

-include $(OBJ:.o=.d)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD) bin

.PHONY: all run clean
