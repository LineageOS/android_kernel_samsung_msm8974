# MSM7x01A
   zreladdr-$(CONFIG_ARCH_MSM7X01A)	:= 0x10008000
params_phys-$(CONFIG_ARCH_MSM7X01A)	:= 0x10000100
initrd_phys-$(CONFIG_ARCH_MSM7X01A)	:= 0x10800000

# MSM7x25
   zreladdr-$(CONFIG_ARCH_MSM7X25)	:= 0x00208000
params_phys-$(CONFIG_ARCH_MSM7X25)	:= 0x00200100
initrd_phys-$(CONFIG_ARCH_MSM7X25)	:= 0x0A000000

# MSM7x27
   zreladdr-$(CONFIG_ARCH_MSM7X27)	:= 0x00208000
params_phys-$(CONFIG_ARCH_MSM7X27)	:= 0x00200100
initrd_phys-$(CONFIG_ARCH_MSM7X27)	:= 0x0A000000

# MSM7x27A
   zreladdr-$(CONFIG_ARCH_MSM7X27A)	:= 0x00208000
params_phys-$(CONFIG_ARCH_MSM7X27A)	:= 0x00200100

# MSM8625
   zreladdr-$(CONFIG_ARCH_MSM8625)	:= 0x00208000
params_phys-$(CONFIG_ARCH_MSM8625)	:= 0x00200100

# MSM7x30
   zreladdr-$(CONFIG_ARCH_MSM7X30)	:= 0x00208000
params_phys-$(CONFIG_ARCH_MSM7X30)	:= 0x00200100
initrd_phys-$(CONFIG_ARCH_MSM7X30)	:= 0x01200000

ifeq ($(CONFIG_MSM_SOC_REV_A),y)
# QSD8x50
   zreladdr-$(CONFIG_ARCH_QSD8X50)	:= 0x20008000
params_phys-$(CONFIG_ARCH_QSD8X50)	:= 0x20000100
initrd_phys-$(CONFIG_ARCH_QSD8X50)	:= 0x24000000
endif

# MSM8x60
   zreladdr-$(CONFIG_ARCH_MSM8X60)	:= 0x40208000

# MSM8960
   zreladdr-$(CONFIG_ARCH_MSM8960)	:= 0x80208000

# MSM8930
   zreladdr-$(CONFIG_ARCH_MSM8930)	:= 0x80208000

# APQ8064
   zreladdr-$(CONFIG_ARCH_APQ8064)	:= 0x80208000

# MSM8974 & 8974PRO
   zreladdr-$(CONFIG_ARCH_MSM8974)	:= 0x00008000
ifeq ($(CONFIG_SEC_MONDRIAN_PROJECT),y)
    ifeq ($(CONFIG_MACH_MONDRIAN_LTE),y)
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= msm8974-sec-mondrianlte-r07.dtb
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= msm8974-sec-mondrianlte-r08.dtb
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= msm8974-sec-mondrianlte-r12.dtb
    else ifeq ($(CONFIG_MACH_MONDRIAN_WIFI),y)
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= apq8074-sec-mondrianwifi-r07.dtb
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= apq8074-sec-mondrianwifi-r08.dtb
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= apq8074-sec-mondrianwifi-r12.dtb
    else ifeq ($(CONFIG_MACH_MONDRIAN_3G),y)
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= msm8974-sec-mondrian3g-r08.dtb
	dtb-$(CONFIG_SEC_MONDRIAN_PROJECT)	+= msm8974-sec-mondrian3g-r12.dtb
    endif
endif
	dtb-$(CONFIG_SEC_VIENNA_PROJECT)	+= msm8974-sec-viennalte-r00.dtb
	dtb-$(CONFIG_SEC_VIENNA_PROJECT)	+= msm8974-sec-viennalte-r10.dtb
	dtb-$(CONFIG_SEC_VIENNA_PROJECT)	+= msm8974-sec-viennalte-r12.dtb
ifeq ($(CONFIG_SEC_K_PROJECT),y)
    ifeq ($(CONFIG_MACH_KLTE_KOR),y)
        # dtbs for KOR
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kkor-r07.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kkor-r08.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kkor-r09.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kkor-r13.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kkor-r14.dtb
    else ifeq ($(CONFIG_MACH_K3GDUOS_CTC),y)
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r03.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r04.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r05.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r06.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r07.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r08.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kctc-r14.dtb
    else ifeq ($(CONFIG_MACH_KLTE_LTNDUOS),y)
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r03.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r04.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r05.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r06.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r07.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r08.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kltnduos-r14.dtb
    else ifeq ($(CONFIG_MACH_KLTE_CHN),y)
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r04.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r05.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kchn-r06.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kchn-r07.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kchn-r08.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kchn-r09.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kchn-r10.dtb
    else ifeq ($(CONFIG_MACH_KLTE_JPN),y)
	# dtbs for JPN
	dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kjpn-r08.dtb
	dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kjpn-r09.dtb
	dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kjpn-r10.dtb
	dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kjpn-r11.dtb
	dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kjpn-r12.dtb
	dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-kjpn-r13.dtb
    else
        # default dtbs
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r03.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r04.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r05.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r06.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r07.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r08.dtb
        dtb-$(CONFIG_SEC_K_PROJECT)	+= msm8974pro-ac-sec-k-r14.dtb
    endif
endif
ifeq ($(CONFIG_SEC_S_PROJECT),y)
	ifeq ($(CONFIG_MACH_SLTE_DCM),y)
	dtb-$(CONFIG_SEC_S_PROJECT)	+= msm8974pro-ac-sec-sjpn-r01.dtb
	else
        # default dtbs
        dtb-$(CONFIG_SEC_S_PROJECT)	+= msm8974pro-ac-sec-s-r00.dtb
        dtb-$(CONFIG_SEC_S_PROJECT)	+= msm8974pro-ac-sec-s-r01.dtb
	endif
endif
ifeq ($(CONFIG_SEC_PATEK_PROJECT),y)
        # default dtbs
        dtb-$(CONFIG_SEC_PATEK_PROJECT)	+= msm8974pro-ac-sec-patek-r00.dtb
        dtb-$(CONFIG_SEC_PATEK_PROJECT)	+= msm8974pro-ac-sec-patek-r02.dtb
        dtb-$(CONFIG_SEC_PATEK_PROJECT)	+= msm8974pro-ac-sec-patek-r04.dtb
endif
	dtb-$(CONFIG_SEC_N2_PROJECT)	+= msm8974-sec-n2-r00.dtb
ifeq ($(CONFIG_SEC_H_PROJECT),y)
ifeq ($(CONFIG_SEC_LOCALE_KOR),y)
	dtb-y += msm8974-sec-hltekor-r04.dtb
	dtb-y += msm8974-sec-hltekor-r05.dtb
	dtb-y += msm8974-sec-hltekor-r06.dtb
	dtb-y += msm8974-sec-hltekor-r07.dtb
else ifeq ($(CONFIG_SEC_LOCALE_JPN),y)
	dtb-y	+= msm8974-sec-hltejpn-r05.dtb
	dtb-y	+= msm8974-sec-hltejpn-r06.dtb
	dtb-y	+= msm8974-sec-hltejpn-r07.dtb
	dtb-y	+= msm8974-sec-hltejpn-r08.dtb
else
	ifeq ($(CONFIG_MACH_H3GDUOS),y)
		ifeq ($(CONFIG_MACH_H3GDUOS_CU),y)
			dtb-y	+= msm8974-sec-h3gchncu-r07.dtb
			dtb-y	+= msm8974-sec-h3gchncu-r09.dtb
		else
			dtb-y	+= msm8974-sec-h3gchnduos-r07.dtb
			dtb-y	+= msm8974-sec-h3gchnduos-r08.dtb
		endif
else
	ifeq ($(CONFIG_MACH_JSGLTE_CHN_CMCC),y)
		dtb-y	+= msm8974-sec-jsglte-r01.dtb
		dtb-y	+= msm8974-sec-jsglte-r02.dtb
		dtb-y	+= msm8974-sec-jsglte-r03.dtb
		dtb-y	+= msm8974-sec-jsglte-r04.dtb
	else
		ifeq ($(CONFIG_MACH_H3G_CHN_CMCC),y)
			dtb-y += msm8974-sec-hlte-chn-r06.dtb
			dtb-y += msm8974-sec-hlte-chn-r07.dtb
			dtb-y += msm8974-sec-hlte-chn-r09.dtb
		else
			ifeq ($(CONFIG_MACH_H3G_CHN_OPEN),y)
				dtb-y += msm8974-sec-hlte-chn-r03.dtb
				dtb-y += msm8974-sec-hlte-chn-r07.dtb
				dtb-y += msm8974-sec-hlte-chn-r09.dtb
			else
				ifeq ($(CONFIG_MACH_HLTE_CHN_CMCC),y)
					dtb-y += msm8974-sec-hlte-chn-r07.dtb
					dtb-y += msm8974-sec-hlte-chn-r09.dtb
				else
					dtb-y += msm8974-sec-hlte-r05.dtb
					dtb-y += msm8974-sec-hlte-r06.dtb
					dtb-y += msm8974-sec-hlte-r07.dtb
					dtb-y += msm8974-sec-hlte-r09.dtb
				endif
			endif
		endif
	endif
  endif
endif
endif

ifeq ($(CONFIG_SEC_FRESCO_PROJECT),y)
ifeq ($(CONFIG_SEC_LOCALE_KOR),y)
	dtb-y += msm8974-sec-frescoltekor-r07.dtb
	dtb-y += msm8974-sec-frescoltekor-r08.dtb
else
	dtb-y += msm8974-sec-frescoltekor-r07.dtb
	dtb-y += msm8974-sec-frescoltekor-r08.dtb
endif
endif
	dtb-$(CONFIG_SEC_LT03_PROJECT)	+= msm8974-sec-lt03-r00.dtb
	dtb-$(CONFIG_SEC_LT03_PROJECT)	+= msm8974-sec-lt03-r05.dtb
	dtb-$(CONFIG_SEC_LT03_PROJECT)	+= msm8974-sec-lt03-r07.dtb
ifeq ($(CONFIG_SEC_LOCALE_KOR),y)
	dtb-$(CONFIG_SEC_LT03_PROJECT)	+= msm8974-sec-lt03kor-r02.dtb
	dtb-$(CONFIG_SEC_LT03_PROJECT)	+= msm8974-sec-lt03kor-r06.dtb
	dtb-$(CONFIG_SEC_LT03_PROJECT)	+= msm8974-sec-lt03kor-r07.dtb
endif
	dtb-$(CONFIG_SEC_PICASSO_PROJECT)	+= msm8974-sec-picasso-r11.dtb
	dtb-$(CONFIG_SEC_PICASSO_PROJECT)	+= msm8974-sec-picasso-r12.dtb
	dtb-$(CONFIG_SEC_PICASSO_PROJECT)	+= msm8974-sec-picasso-r14.dtb
ifeq ($(CONFIG_SEC_CHAGALL_PROJECT),y)
ifeq ($(CONFIG_SEC_LOCALE_JPN),y)
ifeq ($(CONFIG_MACH_CHAGALL_KDI),y)
	# dtbs for JPN KDI (8974Pro)
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974pro-ac-sec-chagalljpn-r00.dtb
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974pro-ac-sec-chagalljpn-r01.dtb
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974pro-ac-sec-chagalljpn-r02.dtb
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974pro-ac-sec-chagalljpn-r03.dtb
else
	# dtbs for JPN
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974-sec-chagalljpn-r00.dtb
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974-sec-chagalljpn-r02.dtb
endif
else
	# default dtbs
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974-sec-chagall-r00.dtb
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974-sec-chagall-r02.dtb
	dtb-$(CONFIG_SEC_CHAGALL_PROJECT)	+= msm8974-sec-chagall-r03.dtb
endif
endif
ifeq ($(CONFIG_SEC_LOCALE_JPN),y)
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimtjpn-r03.dtb
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimtjpn-r04.dtb
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimtjpn-r05.dtb
else
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimt-r00.dtb
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimt-r01.dtb
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimt-r02.dtb
	dtb-$(CONFIG_SEC_KLIMT_PROJECT)	+= msm8974-sec-klimt-r03.dtb
endif
#	dtb-$(CONFIG_SEC_V2_PROJECT)	+= msm8974-sec-v2lte-r00.dtb
	dtb-$(CONFIG_SEC_V2_PROJECT)	+= msm8974-sec-v2lte-r01.dtb
	dtb-$(CONFIG_SEC_V2_PROJECT)	+= msm8974-sec-v2lte-r02.dtb
ifeq ($(CONFIG_SEC_F_PROJECT),y)
# flte kor device tree
	dtb-y	+= msm8974-sec-fltekor-r05.dtb
	dtb-y	+= msm8974-sec-fltekor-r06.dtb
	dtb-y	+= msm8974-sec-fltekor-r07.dtb
	dtb-y	+= msm8974-sec-fltekor-r09.dtb
	dtb-y	+= msm8974-sec-fltekor-r10.dtb
	dtb-y	+= msm8974-sec-fltekor-r11.dtb
	dtb-y	+= msm8974-sec-fltekor-r12.dtb
	dtb-y	+= msm8974-sec-fltekor-r13.dtb
	dtb-y	+= msm8974-sec-fltekor-r14.dtb
	dtb-y	+= msm8974-sec-fltekor-r16.dtb
endif
ifeq ($(CONFIG_SEC_KS01_PROJECT),y)
ifeq ($(CONFIG_SEC_LOCALE_KOR),y)
    # dtbs for KOR
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r00.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r01.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r02.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r03.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r04.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r05.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r06.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r07.dtb
    dtb-$(CONFIG_SEC_KS01_PROJECT)	+= msm8974-sec-ks01lte-r11.dtb
endif
endif
ifeq ($(CONFIG_SEC_JACTIVE_PROJECT),y)
    dtb-$(CONFIG_SEC_JACTIVE_PROJECT)  += msm8974-sec-jactive-r00.dtb
    dtb-$(CONFIG_SEC_JACTIVE_PROJECT)  += msm8974-sec-jactive-r03.dtb
endif
ifeq ($(CONFIG_SEC_JS_PROJECT),y)
ifeq ($(CONFIG_SEC_LOCALE_JPN),y)
     ifeq ($(CONFIG_MACH_JS01LTEZT),y)
	dtb-y	+= msm8974-sec-js01ltetw-r08.dtb
     else
	dtb-$(CONFIG_SEC_JS_PROJECT)	+= msm8974-sec-js01ltejpn-r04.dtb
	dtb-$(CONFIG_SEC_JS_PROJECT)	+= msm8974-sec-js01ltejpn-r05.dtb
	dtb-$(CONFIG_SEC_JS_PROJECT)	+= msm8974-sec-js01ltejpn-r06.dtb
	dtb-$(CONFIG_SEC_JS_PROJECT)	+= msm8974-sec-js01ltejpn-r07.dtb
	dtb-$(CONFIG_SEC_JS_PROJECT)	+= msm8974-sec-js01ltejpn-r08.dtb
     endif
endif
endif
ifeq ($(CONFIG_SEC_KACTIVE_PROJECT),y)
ifeq ($(CONFIG_SEC_LOCALE_KOR),y)
    # dtbs for KOR
    dtb-$(CONFIG_SEC_KACTIVE_PROJECT)	+= msm8974pro-ac-sec-kactivelteskt-r01.dtb
else ifeq ($(CONFIG_SEC_LOCALE_JPN),y)
    # dtbs for JPN
    dtb-$(CONFIG_SEC_KACTIVE_PROJECT)	+= msm8974pro-ac-sec-kactiveltedcm-r02.dtb
else ifeq ($(CONFIG_MACH_KACTIVELTE_KOR),y)
    dtb-y += msm8974pro-ac-sec-kactiveltekor-r02.dtb
    dtb-y += msm8974pro-ac-sec-kactiveltekor-r03.dtb
    dtb-y += msm8974pro-ac-sec-kactiveltekor-r04.dtb
    dtb-y += msm8974pro-ac-sec-kactiveltekor-r05.dtb
    dtb-y += msm8974pro-ac-sec-kactiveltekor-r06.dtb
else
    # default dtbs
    dtb-$(CONFIG_SEC_KACTIVE_PROJECT)	+= msm8974pro-ac-sec-kactivelte-r00.dtb
    dtb-$(CONFIG_SEC_KACTIVE_PROJECT)	+= msm8974pro-ac-sec-kactivelte-r01.dtb
endif
endif
	dtb-$(CONFIG_SEC_KSPORTS_PROJECT)	+= msm8974pro-ac-sec-ksports-r00.dtb
	dtb-$(CONFIG_SEC_KSPORTS_PROJECT)	+= msm8974pro-ac-sec-ksports-r01.dtb
	dtb-$(CONFIG_SEC_KSPORTS_PROJECT)	+= msm8974pro-ac-sec-ksports-r03.dtb

# APQ8084
   zreladdr-$(CONFIG_ARCH_APQ8084)	:= 0x00008000
        dtb-$(CONFIG_ARCH_APQ8084)	+= apq8084-sim.dtb

# MSMKRYPTON
   zreladdr-$(CONFIG_ARCH_MSMKRYPTON)	:= 0x00008000
	dtb-$(CONFIG_ARCH_MSMKRYPTON)	+= msmkrypton-sim.dtb

# MSM9615
   zreladdr-$(CONFIG_ARCH_MSM9615)	:= 0x40808000

# MSM9625
   zreladdr-$(CONFIG_ARCH_MSM9625)	:= 0x00208000
        dtb-$(CONFIG_ARCH_MSM9625)	+= msm9625-v1-cdp.dtb
        dtb-$(CONFIG_ARCH_MSM9625)	+= msm9625-v1-mtp.dtb
        dtb-$(CONFIG_ARCH_MSM9625)	+= msm9625-v1-rumi.dtb
	dtb-$(CONFIG_ARCH_MSM9625)      += msm9625-v2-cdp.dtb
	dtb-$(CONFIG_ARCH_MSM9625)      += msm9625-v2-mtp.dtb
	dtb-$(CONFIG_ARCH_MSM9625)      += msm9625-v2.1-mtp.dtb
	dtb-$(CONFIG_ARCH_MSM9625)      += msm9625-v2.1-cdp.dtb

# MSM8226
   zreladdr-$(CONFIG_ARCH_MSM8226)	:= 0x00008000
#        dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sim.dtb
#        dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-fluid.dtb
#        dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-v1-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-v2-720p-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-v2-1080p-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-720p-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-1080p-mtp.dtb
#        dtb-$(CONFIG_ARCH_MSM8226)	+= apq8026-v1-mtp.dtb
#        dtb-$(CONFIG_ARCH_MSM8226)	+= apq8026-v2-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= apq8026-v2-720p-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= apq8026-v2-1080p-mtp.dtb
#	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-v1-mtp-r01.dtb
ifeq ($(CONFIG_MACH_MILLET3G_CHN_OPEN),y)
         dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-millet3g-chn-open-r00.dtb
else ifeq ($(CONFIG_MACH_MILLET3G_EUR),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-millet3geur-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-millet3geur-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-millet3geur-r04.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-millet3geur-r05.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-millet3geur-r06.dtb
else ifeq ($(CONFIG_MACH_MILLETLTE_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletlte-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletlte-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletlte-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletlte-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletlte-r04.dtb
else ifeq ($(CONFIG_MACH_MILLETLTE_ATT),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletlteatt-r00.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletlteatt-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletlteatt-r02.dtb
else ifeq ($(CONFIG_MACH_MILLETLTE_CAN),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletltecan-r01.dtb
else ifeq ($(CONFIG_MACH_MILLETLTE_TMO),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletlte_tmo-r01.dtb
else ifeq ($(CONFIG_MACH_MILLETLTE_VZW),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletltevzw-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletltevzw-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletltevzw-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletltevzw-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletltevzw-r04.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-milletltevzw-r05.dtb
else ifeq ($(CONFIG_MACH_MILLETLTE_KOR),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletltekor-r00.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletltekor-r01.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletltekor-r02.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletltekor-r03.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-milletltekor-r04.dtb
else ifeq ($(CONFIG_SEC_MILLETWIFI_COMMON),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-milletwifieur-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-milletwifieur-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-milletwifieur-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-milletwifieur-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-milletwifieur-r04.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-milletwifieur-r05.dtb
else ifeq ($(CONFIG_SEC_RUBENSWIFI_COMMON),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-rubenswifieur-r00.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-rubenswifieur-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-rubenswifieur-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-rubenswifieur-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-rubenswifieur-r04.dtb
else ifeq ($(CONFIG_MACH_RUBENSLTE_OPEN),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-rubenslteeur-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-rubenslteeur-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-rubenslteeur-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-rubenslteeur-r03.dtb
else ifeq ($(CONFIG_MACH_DEGASLTE_SPR),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-degasltespr-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-degasltespr-r02.dtb
else ifeq ($(CONFIG_MACH_DEGASLTE_VZW),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-degasltevzw-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-degasltevzw-r05.dtb
else ifeq ($(CONFIG_MACH_MATISSE3G_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-matisse3g-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-matisse3g-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-matisse3g-r02.dtb
else ifeq ($(CONFIG_SEC_MATISSEWIFI_COMMON),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-matissewifi-r00.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-matissewifi-r01.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-matissewifi-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-matissewifi-r03.dtb
else ifeq ($(CONFIG_MACH_MATISSELTE_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-matisselte-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-matisselte-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-matisselte-r02.dtb
else ifeq ($(CONFIG_MACH_MATISSELTE_ATT),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteatt-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteatt-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteatt-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteatt-r03.dtb
else ifeq ($(CONFIG_MACH_MATISSELTE_VZW),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisseltevzw-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisseltevzw-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisseltevzw-r03.dtb
else ifeq ($(CONFIG_MACH_MATISSELTE_USC),y)
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteusc-r00.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteusc-r01.dtb
         dtb-$(CONFIG_ARCH_MSM8226)     += msm8926-sec-matisselteusc-r03.dtb
else ifeq ($(CONFIG_SEC_AFYONLTE_COMMON),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r04.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r05.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-afyonlte-r06.dtb
else ifeq ($(CONFIG_MACH_ATLANTICLTE_ATT),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-atlanticlteatt-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-atlanticlteatt-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-atlanticlteatt-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-atlanticlteatt-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-atlanticlteatt-r05.dtb
else ifeq ($(CONFIG_MACH_ATLANTICLTE_VZW),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticltevzw-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticltevzw-r01.dtb
else ifeq ($(CONFIG_MACH_ATLANTICLTE_USC),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticlteusc-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticlteusc-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticlteusc-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticlteusc-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-atlanticlteusc-r05.dtb
else ifeq ($(CONFIG_SEC_ATLANTIC3G_COMMON),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8228-sec-atlantic3g-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8228-sec-atlantic3g-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8228-sec-atlantic3g-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8228-sec-atlantic3g-r03.dtb
else ifeq ($(CONFIG_MACH_BERLUTI3G_EUR),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-berluti3geur-r00.dtb
else ifeq ($(CONFIG_MACH_BERLUTILTE_EUR),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-berlutilte-r00.dtb
else ifeq ($(CONFIG_MACH_VICTOR3GDSDTV_LTN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-victor3gdsdtv-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-victor3gdsdtv-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-victor3gdsdtv-r02.dtb
else ifeq ($(CONFIG_MACH_S3VE3G_EUR),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8226-sec-s3ve3geur-r04.dtb
else ifeq ($(CONFIG_MACH_MS01_EUR_3G),y)
                dtb-$(CONFIG_ARCH_MSM8226)      += msm8226-sec-ms013geur-r00.dtb
                dtb-$(CONFIG_ARCH_MSM8226)      += msm8226-sec-ms013geur-r01.dtb
                dtb-$(CONFIG_ARCH_MSM8226)      += msm8226-sec-ms013geur-r02.dtb
                dtb-$(CONFIG_ARCH_MSM8226)      += msm8226-sec-ms013geur-r03.dtb
                dtb-$(CONFIG_ARCH_MSM8226)      += msm8226-sec-ms013geur-r04.dtb
                dtb-$(CONFIG_ARCH_MSM8226)      += msm8226-sec-ms013geur-r05.dtb
else ifeq ($(CONFIG_MACH_VICTORLTE_CMCC),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-victorltecmcc-r00.dtb
else ifeq ($(CONFIG_MACH_VICTORLTE_CTC),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-victorltectc-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-victorltectc-r01.dtb
else ifeq ($(CONFIG_MACH_VASTALTE_CTC),y)
	 dtb-$(CONFIG_ARCH_MSM8226) += msm8926-sec-vastaltectc-r00.dtb
else ifeq ($(CONFIG_MACH_VASTALTE_CHN_CMCC_DUOS),y)
	 dtb-$(CONFIG_ARCH_MSM8226) += msm8926-sec-vastalteduos-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226) += msm8926-sec-vastalteduos-r03.dtb
else ifeq ($(CONFIG_MACH_FRESCONEOLTE_CTC),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-fresconeoltectc-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-fresconeoltectc-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-fresconeoltectc-r02.dtb
else ifeq ($(CONFIG_MACH_HESTIALTE_EUR),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-hestia-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-hestia-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-hestia-r02.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-hestia-r03.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-hestia-r04.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-hestia-r06.dtb
else ifeq ($(CONFIG_MACH_HESTIALTE_ATT),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8928-sec-hestialteatt-r07.dtb
else ifeq ($(CONFIG_SEC_MEGA23G_COMMON),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8228-sec-mega23g-r00.dtb
else ifeq ($(CONFIG_SEC_MEGA2LTE_COMMON),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8928-sec-mega2lte-r00.dtb
else ifeq ($(CONFIG_MACH_GNOTELTEDS_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)	+= msm8926-sec-gnotelteds-r00.dtb
else ifeq ($(CONFIG_MACH_T10_3G_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-t10_3g-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-t10_3g-r01.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-t10_3g-r02.dtb
else ifeq ($(CONFIG_MACH_T8_3G_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-t8_3g-r00.dtb
else ifeq ($(CONFIG_MACH_T10_WIFI_OPEN),y)
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-t10_wifi-r00.dtb
	 dtb-$(CONFIG_ARCH_MSM8226)     += msm8226-sec-t10_wifi-r01.dtb
endif

# FSM9XXX
   zreladdr-$(CONFIG_ARCH_FSM9XXX)	:= 0x10008000
params_phys-$(CONFIG_ARCH_FSM9XXX)	:= 0x10000100
initrd_phys-$(CONFIG_ARCH_FSM9XXX)	:= 0x12000000

# FSM9900
   zreladdr-$(CONFIG_ARCH_FSM9900)	:= 0x00008000
        dtb-$(CONFIG_ARCH_FSM9900)	:= fsm9900-rumi.dtb
        dtb-$(CONFIG_ARCH_FSM9900)	:= fsm9900-sim.dtb

# MPQ8092
   zreladdr-$(CONFIG_ARCH_MPQ8092)	:= 0x00008000

# MSM8610
   zreladdr-$(CONFIG_ARCH_MSM8610)	:= 0x00008000
   ifeq ($(CONFIG_SEC_HEAT_PROJECT),y)
	ifeq ($(CONFIG_MACH_HEAT_DYN),y)
		dtb-$(CONFIG_SEC_HEAT_PROJECT)	+= msm8610-sec-heat-dyn-r00.dtb
	else
		ifeq ($(CONFIG_MACH_HEAT_AIO),y)
			dtb-$(CONFIG_SEC_HEAT_PROJECT)	+= msm8610-sec-heat-aio-r00.dtb
		else
			dtb-$(CONFIG_SEC_HEAT_PROJECT)	+= msm8610-sec-heat-tfnvzw-r00.dtb
		endif
	endif
#else
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v1-cdp.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v2-cdp.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v1-mtp.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v2-mtp.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-rumi.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-sim.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v1-qrd-skuaa.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v1-qrd-skuab.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v2-qrd-skuaa.dtb
#        dtb-$(CONFIG_ARCH_MSM8610)	+= msm8610-v2-qrd-skuab.dtb
endif
ifeq ($(CONFIG_MACH_KANAS3G_CMCC),y)
	dtb-$(CONFIG_MACH_KANAS3G_CMCC)	+= msm8610-sec-kanas3g-chn-cmcc-r00.dtb
	dtb-$(CONFIG_MACH_KANAS3G_CMCC)	+= msm8610-sec-kanas3g-chn-cmcc-r01.dtb
	dtb-$(CONFIG_MACH_KANAS3G_CMCC)	+= msm8610-sec-kanas3g-chn-cmcc-r02.dtb
	dtb-$(CONFIG_MACH_KANAS3G_CMCC)	+= msm8610-sec-kanas3g-chn-cmcc-r03.dtb
	dtb-$(CONFIG_MACH_KANAS3G_CMCC)	+= msm8610-sec-kanas3g-chn-cmcc-r04.dtb
	dtb-$(CONFIG_MACH_KANAS3G_CMCC)	+= msm8610-sec-kanas3g-chn-cmcc-r05.dtb
else ifeq ($(CONFIG_MACH_KANAS3G_CU),y)
	dtb-$(CONFIG_SEC_KANAS_PROJECT)	+= msm8610-sec-kanas3g-chn-cu-r00.dtb
	dtb-$(CONFIG_SEC_KANAS_PROJECT)	+= msm8610-sec-kanas3g-chn-cu-r02.dtb
	dtb-$(CONFIG_SEC_KANAS_PROJECT)	+= msm8610-sec-kanas3g-chn-cu-r04.dtb
endif

ifeq ($(CONFIG_MACH_KANAS3G_CTC),y)
    dtb-$(CONFIG_MACH_KANAS3G_CTC) +=  msm8610-sec-kanas3g-chn-ctc-r00.dtb
    dtb-$(CONFIG_MACH_KANAS3G_CTC) +=  msm8610-sec-kanas3g-chn-ctc-r01.dtb
    dtb-$(CONFIG_MACH_KANAS3G_CTC) +=  msm8610-sec-kanas3g-chn-ctc-r03.dtb
    dtb-$(CONFIG_MACH_KANAS3G_CTC) +=  msm8610-sec-kanas3g-chn-ctc-r04.dtb
endif
# MSMSAMARIUM
#   zreladdr-$(CONFIG_ARCH_MSMSAMARIUM)	:= 0x00008000
#	dtb-$(CONFIG_ARCH_MSMSAMARIUM)	+= msmsamarium-sim.dtb
#	dtb-$(CONFIG_ARCH_MSMSAMARIUM)	+= msmsamarium-rumi.dtb
