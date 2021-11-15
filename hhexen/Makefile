.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT must be set in the environment.")
endif

include $(PSL1GHT)/Makefile.base

TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCE		:=	source
INCLUDE		:=	include
DATA		:=	
LIBS		:=	-lgcm_sys -lreality -lsysutil -lio -lsysmodule -laudioplayer -lmpg123 -logg -lspu_sound -laudio -lm \
            $(PSL1GHT)/modules/spu_soundmodule.bin.a

ICON0       = $(SOURCE)/../ICON0.PNG

TITLE		:=	HHEXEN - PS3
APPID		:=	HEXEN0001
CONTENTID	:=	UP0001-$(APPID)_00-0000000000000000

CFLAGS		+= -g -O2 -Wall --std=gnu99
CXXFLAGS	+= -g -O2 -Wall

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCE),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export BUILDDIR	:=	$(CURDIR)/$(BUILD)
export DEPSDIR	:=	$(BUILDDIR)

CFILES		:= a_action.c am_map.c ct_chat.c d_net.c f_finale.c font.c g_game.c graphics.c h2_main.c h_hubmsg.c i_cdmus.c \
            i_cyber.c i_sound.c in_lude.c info.c m_misc.c mn_menu.c p_acs.c p_anim.c p_ceilng.c p_doors.c p_enemy.c p_floor.c \
            p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c \
            p_things.c p_tick.c p_user.c po_man.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c r_segs.c r_things.c sb_bar.c sc_man.c \
            sn_sonix.c sounds.c st_start.c sv_save.c tables.c v_video.c w_wad.c z_zone.c ps3main.c

CXXFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
VCGFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.vcg)))

export OFILES	:=	$(CFILES:.c=.o) \
					$(CXXFILES:.cpp=.o) \
					$(SFILES:.S=.o) \
					$(BINFILES:.bin=.bin.o) \
					$(VCGFILES:.vcg=.vcg.o)

export BINFILES	:=	$(BINFILES:.bin=.bin.h)
export VCGFILES	:=	$(VCGFILES:.vcg=.vcg.h)

export INCLUDES	:=	$(foreach dir,$(INCLUDE),-I$(CURDIR)/$(dir)) \
					-I$(CURDIR)/$(BUILD) -I$(PSL1GHT)/modules

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo Clean...
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).a $(OUTPUT).pkg

pkg: $(BUILD)
	@echo Creating PKG...
	@mkdir -p $(BUILD)/pkg
	@mkdir -p $(BUILD)/pkg/USRDIR
	@cp $(ICON0) $(BUILD)/pkg/
	@$(FSELF) -n $(BUILD)/$(TARGET).elf $(BUILD)/pkg/USRDIR/EBOOT.BIN
	@$(SFO) --title "$(TITLE)" --appid "$(APPID)" -f $(SFOXML) $(BUILD)/pkg/PARAM.SFO
	@$(PKG) --contentid $(CONTENTID) $(BUILD)/pkg/ $(OUTPUT).pkg

run: $(BUILD)
	@$(PS3LOADAPP) $(OUTPUT).self

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
$(OFILES): $(BINFILES) $(VCGFILES)

-include $(DEPENDS)

endif
