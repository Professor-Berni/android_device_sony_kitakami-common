[AID_VENDOR_QCOM_DIAG]
value:2950

[AID_VENDOR_RFS]
value:2951

[AID_VENDOR_RFS_SHARED]
value:2952

[AID_VENDOR_IDD]
value: 2987

[AID_VENDOR_UPDATEMISCTA]
value: 2991

[AID_VENDOR_TRIMAREA]
value: 2993

[AID_VENDOR_CREDMGR_CLIENT]
value: 2996

[AID_VENDOR_TAD]
value: 2997

[AID_VENDOR_TA_QMI]
value: 2998

[system/vendor/bin/imsdatadaemon]
mode: 0755
user: AID_SYSTEM
group: AID_SYSTEM
caps: NET_BIND_SERVICE

[system/vendor/bin/ims_rtp_daemon]
mode: 0755
user: AID_SYSTEM
group: AID_RADIO
caps: NET_BIND_SERVICE

[system/bin/xtwifi-client]
mode: 0755
user: AID_ROOT
group: AID_SHELL
caps: WAKE_ALARM BLOCK_SUSPEND

[system/bin/xtwifi-inet-agent]
mode: 0755
user: AID_ROOT
group: AID_SHELL
caps: WAKE_ALARM BLOCK_SUSPEND

[system/bin/lowi-server]
mode: 0755
user: AID_ROOT
group: AID_SHELL
caps: WAKE_ALARM BLOCK_SUSPEND