#ifndef __MSMB_ISP__
#define __MSMB_ISP__

#include <linux/videodev2.h>

#define MAX_PLANES_PER_STREAM 3
#define MAX_NUM_STREAM 7

#define ISP_VERSION_40        40
#define ISP_VERSION_32        32
#define ISP_NATIVE_BUF_BIT    0x10000
#define ISP0_BIT              0x20000
#define ISP1_BIT              0x40000
#define ISP_META_CHANNEL_BIT  0x80000
#define ISP_STATS_STREAM_BIT  0x80000000

#define MSM_VFE_REG_CFG_FRAME_ID_NOT_MATCH_ERROR	0xCACFC

enum ISP_START_PIXEL_PATTERN {
	ISP_BAYER_RGRGRG,
	ISP_BAYER_GRGRGR,
	ISP_BAYER_BGBGBG,
	ISP_BAYER_GBGBGB,
	ISP_YUV_YCbYCr,
	ISP_YUV_YCrYCb,
	ISP_YUV_CbYCrY,
	ISP_YUV_CrYCbY,
	ISP_PIX_PATTERN_MAX
};

enum msm_vfe_plane_fmt {
	Y_PLANE,
	CB_PLANE,
	CR_PLANE,
	CRCB_PLANE,
	CBCR_PLANE,
	VFE_PLANE_FMT_MAX
};

enum msm_vfe_input_src {
	VFE_PIX_0,
	VFE_RAW_0,
	VFE_RAW_1,
	VFE_RAW_2,
	VFE_SRC_MAX,
};

enum msm_vfe_axi_stream_src {
	PIX_ENCODER,
	PIX_VIEWFINDER,
	CAMIF_RAW,
	IDEAL_RAW,
	RDI_INTF_0,
	RDI_INTF_1,
	RDI_INTF_2,
	VFE_AXI_SRC_MAX
};

enum msm_vfe_frame_skip_pattern {
	NO_SKIP,
	EVERY_2FRAME,
	EVERY_3FRAME,
	EVERY_4FRAME,
	EVERY_5FRAME,
	EVERY_6FRAME,
	EVERY_7FRAME,
	EVERY_8FRAME,
#ifdef CONFIG_MSMB_CAMERA_MM
	EVERY_9FRAME,
	EVERY_10FRAME,
	EVERY_11FRAME,
	EVERY_12FRAME,
	EVERY_13FRAME,
	EVERY_14FRAME,
	EVERY_15FRAME,
#endif
	EVERY_16FRAME,
	EVERY_32FRAME,
	SKIP_ALL,
	MAX_SKIP,
};

enum msm_vfe_camif_input {
	CAMIF_DISABLED,
	CAMIF_PAD_REG_INPUT,
	CAMIF_MIDDI_INPUT,
	CAMIF_MIPI_INPUT,
};

struct msm_vfe_camif_cfg {
	uint32_t lines_per_frame;
	uint32_t pixels_per_line;
	uint32_t first_pixel;
	uint32_t last_pixel;
	uint32_t first_line;
	uint32_t last_line;
	uint32_t epoch_line0;
	uint32_t epoch_line1;
	enum msm_vfe_camif_input camif_input;
};

enum msm_vfe_inputmux {
	CAMIF,
	TESTGEN,
	EXTERNAL_READ,
};

struct msm_vfe_pix_cfg {
	struct msm_vfe_camif_cfg camif_cfg;
	enum msm_vfe_inputmux input_mux;
	enum ISP_START_PIXEL_PATTERN pixel_pattern;
};

struct msm_vfe_rdi_cfg {
	uint8_t cid;
	uint8_t frame_based;
};

struct msm_vfe_input_cfg {
	union {
		struct msm_vfe_pix_cfg pix_cfg;
		struct msm_vfe_rdi_cfg rdi_cfg;
	} d;
	enum msm_vfe_input_src input_src;
	uint32_t input_pix_clk;
};

struct msm_vfe_axi_plane_cfg {
	uint32_t output_width; /*Include padding*/
	uint32_t output_height;
	uint32_t output_stride;
	uint32_t output_scan_lines;
	uint32_t output_plane_format; /*Y/Cb/Cr/CbCr*/
	uint32_t plane_addr_offset;
	uint8_t csid_src; /*RDI 0-2*/
	uint8_t rdi_cid;/*CID 1-16*/
};

struct msm_vfe_axi_stream_request_cmd {
	uint32_t session_id;
	uint32_t stream_id;
	uint32_t output_format;/*Planar/RAW/Misc*/
	enum msm_vfe_axi_stream_src stream_src; /*CAMIF/IDEAL/RDIs*/
	struct msm_vfe_axi_plane_cfg plane_cfg[MAX_PLANES_PER_STREAM];

	uint32_t burst_count;
	uint32_t hfr_mode;
	uint8_t frame_base;

	uint32_t init_frame_drop; /*MAX 31 Frames*/
	enum msm_vfe_frame_skip_pattern frame_skip_pattern;
	uint8_t buf_divert; /* if TRUE no vb2 buf done. */
	/*Return values*/
	uint32_t axi_stream_handle;
};

struct msm_vfe_axi_stream_release_cmd {
	uint32_t stream_handle;
};

enum msm_vfe_axi_stream_cmd {
	STOP_STREAM,
	START_STREAM,
};

struct msm_vfe_axi_stream_cfg_cmd {
	uint8_t num_streams;
	uint32_t stream_handle[MAX_NUM_STREAM];
	enum msm_vfe_axi_stream_cmd cmd;
#ifdef CONFIG_MSMB_CAMERA_MM
	int32_t recording_hint;
#endif
};

enum msm_vfe_axi_stream_update_type {
#ifdef CONFIG_MSMB_CAMERA_MM
    AXI_STREAM_UPDATE_INVALID,
#endif
	ENABLE_STREAM_BUF_DIVERT,
	DISABLE_STREAM_BUF_DIVERT,
	UPDATE_STREAM_FRAMEDROP_PATTERN,
	UPDATE_STREAM_AXI_CONFIG,
};

struct msm_vfe_axi_stream_cfg_update_info {
	uint32_t stream_handle;
	uint32_t output_format;
	enum msm_vfe_frame_skip_pattern skip_pattern;
	struct msm_vfe_axi_plane_cfg plane_cfg[MAX_PLANES_PER_STREAM];
};

struct msm_vfe_axi_halt_cmd {
  uint32_t stop_camif; //Boolean whether stop camif is to be done
  uint32_t overflow_detected;
};

struct msm_vfe_axi_reset_cmd {
  uint32_t blocking; //Boolean whether stop camif is to be done
  unsigned long frame_id;
};

struct msm_vfe_axi_restart_cmd {
  uint32_t enable_camif; //Boolean whether stop camif is to be done
};

struct msm_vfe_axi_stream_update_cmd {
	uint32_t num_streams;
	enum msm_vfe_axi_stream_update_type update_type;
	struct msm_vfe_axi_stream_cfg_update_info update_info[MAX_NUM_STREAM];
};

enum msm_vfe_stats_pipeline_policy {
	STATS_COMP_ALL,
	STATS_COMP_NONE,
	MAX_STATS_POLICY,
};

enum msm_isp_stats_type {
	MSM_ISP_STATS_AEC,   /* legacy based AEC */
	MSM_ISP_STATS_AF,    /* legacy based AF */
	MSM_ISP_STATS_AWB,   /* legacy based AWB */
	MSM_ISP_STATS_RS,    /* legacy based RS */
	MSM_ISP_STATS_CS,    /* legacy based CS */
	MSM_ISP_STATS_IHIST, /* legacy based HIST */
	MSM_ISP_STATS_SKIN,  /* legacy based SKIN */
	MSM_ISP_STATS_BG,    /* Bayer Grids */
	MSM_ISP_STATS_BF,    /* Bayer Focus */
	MSM_ISP_STATS_BE,    /* Bayer Exposure*/
	MSM_ISP_STATS_BHIST, /* Bayer Hist */
	MSM_ISP_STATS_MAX    /* MAX */
};

struct msm_vfe_stats_stream_request_cmd {
	uint32_t session_id;
	uint32_t stream_id;
	enum msm_isp_stats_type stats_type;
	uint32_t composite_flag;
	uint32_t framedrop_pattern;
	uint32_t irq_subsample_pattern;
	uint32_t buffer_offset;
	uint32_t stream_handle;
};

struct msm_vfe_stats_stream_release_cmd {
	uint32_t stream_handle;
};
struct msm_vfe_stats_stream_cfg_cmd {
	uint8_t num_streams;
	uint32_t stream_handle[MSM_ISP_STATS_MAX];
	uint8_t enable;
};

struct msm_vfe_stats_comp_policy_cfg {
	enum msm_vfe_stats_pipeline_policy stats_pipeline_policy;
	uint32_t comp_framedrop_pattern;
	uint32_t comp_irq_subsample_pattern;
};

enum msm_vfe_reg_cfg_type {
	VFE_WRITE,
	VFE_WRITE_MB,
	VFE_READ,
	VFE_CFG_MASK,
	VFE_WRITE_DMI_16BIT,
	VFE_WRITE_DMI_32BIT,
	VFE_WRITE_DMI_64BIT,
	VFE_READ_DMI_16BIT,
	VFE_READ_DMI_32BIT,
	VFE_READ_DMI_64BIT,
};

struct msm_vfe_cfg_cmd2 {
	uint16_t num_cfg;
	uint16_t cmd_len;
#if defined(CONFIG_MSMB_CAMERA_MM) || defined(CONFIG_SEC_LT03_PROJECT)
	uint32_t frame_id;
#endif
	void __user *cfg_data;
	void __user *cfg_cmd;
};

struct msm_vfe_reg_rw_info {
	uint32_t reg_offset;
	uint32_t cmd_data_offset;
	uint32_t len;
};

struct msm_vfe_reg_mask_info {
	uint32_t reg_offset;
	uint32_t mask;
	uint32_t val;
};

struct msm_vfe_reg_dmi_info {
	uint32_t hi_tbl_offset; /*Optional*/
	uint32_t lo_tbl_offset; /*Required*/
	uint32_t len;
};

struct msm_vfe_reg_cfg_cmd {
	union {
		struct msm_vfe_reg_rw_info rw_info;
		struct msm_vfe_reg_mask_info mask_info;
		struct msm_vfe_reg_dmi_info dmi_info;
	} u;

	enum msm_vfe_reg_cfg_type cmd_type;
};

enum msm_isp_buf_type {
	ISP_PRIVATE_BUF,
	ISP_SHARE_BUF,
	MAX_ISP_BUF_TYPE,
};

struct msm_isp_buf_request {
	uint32_t session_id;
	uint32_t stream_id;
	uint8_t num_buf;
	uint32_t handle;
	enum msm_isp_buf_type buf_type;
};

struct msm_isp_qbuf_info {
	uint32_t handle;
	int buf_idx;
	/*Only used for prepare buffer*/
	struct v4l2_buffer buffer;
	/*Only used for diverted buffer*/
	uint32_t dirty_buf;
};

struct msm_vfe_axi_src_state {
	enum msm_vfe_input_src input_src;
	uint32_t src_active;
};

enum msm_isp_event_idx {
	ISP_REG_UPDATE      = 0,
	ISP_START_ACK       = 1,
	ISP_STOP_ACK        = 2,
	ISP_IRQ_VIOLATION   = 3,
	ISP_WM_BUS_OVERFLOW = 4,
	ISP_STATS_OVERFLOW  = 5,
	ISP_CAMIF_ERROR     = 6,
	ISP_SOF             = 7,
	ISP_EOF             = 8,
	ISP_ERROR           = 9,
	ISP_EVENT_MAX       = 10
};

#define ISP_EVENT_OFFSET          8
#define ISP_EVENT_BASE            (V4L2_EVENT_PRIVATE_START)
#define ISP_BUF_EVENT_BASE        (ISP_EVENT_BASE + (1 << ISP_EVENT_OFFSET))
#define ISP_STATS_EVENT_BASE      (ISP_EVENT_BASE + (2 << ISP_EVENT_OFFSET))
#define ISP_EVENT_REG_UPDATE      (ISP_EVENT_BASE + ISP_REG_UPDATE)
#define ISP_EVENT_START_ACK       (ISP_EVENT_BASE + ISP_START_ACK)
#define ISP_EVENT_STOP_ACK        (ISP_EVENT_BASE + ISP_STOP_ACK)
#define ISP_EVENT_IRQ_VIOLATION   (ISP_EVENT_BASE + ISP_IRQ_VIOLATION)
#define ISP_EVENT_WM_BUS_OVERFLOW (ISP_EVENT_BASE + ISP_WM_BUS_OVERFLOW)
#define ISP_EVENT_STATS_OVERFLOW  (ISP_EVENT_BASE + ISP_STATS_OVERFLOW)
#define ISP_EVENT_CAMIF_ERROR     (ISP_EVENT_BASE + ISP_CAMIF_ERROR)
#define ISP_EVENT_SOF             (ISP_EVENT_BASE + ISP_SOF)
#define ISP_EVENT_EOF             (ISP_EVENT_BASE + ISP_EOF)
#define ISP_EVENT_ERROR           (ISP_EVENT_BASE + ISP_ERROR)
#define ISP_EVENT_BUF_DIVERT      (ISP_BUF_EVENT_BASE)
#define ISP_EVENT_STATS_NOTIFY    (ISP_STATS_EVENT_BASE)
#define ISP_EVENT_COMP_STATS_NOTIFY (ISP_EVENT_STATS_NOTIFY + MSM_ISP_STATS_MAX)
/* The msm_v4l2_event_data structure should match the
 * v4l2_event.u.data field.
 * should not exceed 64 bytes */

struct msm_isp_buf_event {
	uint32_t session_id;
	uint32_t stream_id;
	uint32_t handle;
	uint32_t output_format;
	int8_t buf_idx;
};
struct msm_isp_stats_event {
	uint32_t stats_mask;                        /* 4 bytes */
	uint8_t stats_buf_idxs[MSM_ISP_STATS_MAX];  /* 11 bytes */
};

struct msm_isp_stream_ack {
	uint32_t session_id;
	uint32_t stream_id;
	uint32_t handle;
};

struct msm_isp_error_info {
  uint32_t error_mask; /* 1 << msm_isp_event_idx */
};

struct msm_isp_event_data {
	/*Wall clock except for buffer divert events
	 *which use monotonic clock
	 */
	struct timeval timestamp;
	/* if pix is a src frame_id is from camif */
	uint32_t frame_id;
#ifdef CONFIG_MSMB_CAMERA_MM
	enum msm_vfe_input_src input_src;
#endif
	union {
		/* START_ACK, STOP_ACK */
		struct msm_isp_stream_ack stream_ack;
		/* REG_UPDATE_TRIGGER, bus over flow */
#ifdef CONFIG_MSMB_CAMERA_LL
		enum msm_vfe_input_src input_src;
#endif
		
		/* stats notify */
		struct msm_isp_stats_event stats;
		/* IRQ_VIOLATION, STATS_OVER_FLOW, WM_OVER_FLOW */
		uint32_t irq_status_mask;
		struct msm_isp_buf_event buf_done;
#ifdef CONFIG_MSMB_CAMERA_MM
    struct msm_isp_error_info error_info;
#endif
	} u; /* union can have max 52 bytes */
};

#define V4L2_PIX_FMT_QBGGR8  v4l2_fourcc('Q', 'B', 'G', '8')
#define V4L2_PIX_FMT_QGBRG8  v4l2_fourcc('Q', 'G', 'B', '8')
#define V4L2_PIX_FMT_QGRBG8  v4l2_fourcc('Q', 'G', 'R', '8')
#define V4L2_PIX_FMT_QRGGB8  v4l2_fourcc('Q', 'R', 'G', '8')
#define V4L2_PIX_FMT_QBGGR10 v4l2_fourcc('Q', 'B', 'G', '0')
#define V4L2_PIX_FMT_QGBRG10 v4l2_fourcc('Q', 'G', 'B', '0')
#define V4L2_PIX_FMT_QGRBG10 v4l2_fourcc('Q', 'G', 'R', '0')
#define V4L2_PIX_FMT_QRGGB10 v4l2_fourcc('Q', 'R', 'G', '0')
#define V4L2_PIX_FMT_QBGGR12 v4l2_fourcc('Q', 'B', 'G', '2')
#define V4L2_PIX_FMT_QGBRG12 v4l2_fourcc('Q', 'G', 'B', '2')
#define V4L2_PIX_FMT_QGRBG12 v4l2_fourcc('Q', 'G', 'R', '2')
#define V4L2_PIX_FMT_QRGGB12 v4l2_fourcc('Q', 'R', 'G', '2')
#define V4L2_PIX_FMT_META v4l2_fourcc('M', 'E', 'T', 'A')
#define V4L2_PIX_FMT_NV14 v4l2_fourcc('N', 'V', '1', '4')
#define V4L2_PIX_FMT_NV41 v4l2_fourcc('N', 'V', '4', '1')
#define V4L2_PIX_FMT_NV46 v4l2_fourcc('N', 'V', '4', '6')
#define V4L2_PIX_FMT_NV64 v4l2_fourcc('N', 'V', '6', '4')

#define VIDIOC_MSM_VFE_REG_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE, struct msm_vfe_cfg_cmd2)

#define VIDIOC_MSM_ISP_REQUEST_BUF \
	_IOWR('V', BASE_VIDIOC_PRIVATE+1, struct msm_isp_buf_request)

#define VIDIOC_MSM_ISP_ENQUEUE_BUF \
	_IOWR('V', BASE_VIDIOC_PRIVATE+2, struct msm_isp_qbuf_info)

#define VIDIOC_MSM_ISP_RELEASE_BUF \
	_IOWR('V', BASE_VIDIOC_PRIVATE+3, struct msm_isp_buf_request)

#define VIDIOC_MSM_ISP_REQUEST_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+4, struct msm_vfe_axi_stream_request_cmd)

#define VIDIOC_MSM_ISP_CFG_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+5, struct msm_vfe_axi_stream_cfg_cmd)

#define VIDIOC_MSM_ISP_RELEASE_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+6, struct msm_vfe_axi_stream_release_cmd)

#define VIDIOC_MSM_ISP_INPUT_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE+7, struct msm_vfe_input_cfg)

#define VIDIOC_MSM_ISP_SET_SRC_STATE \
	_IOWR('V', BASE_VIDIOC_PRIVATE+8, struct msm_vfe_axi_src_state)

#define VIDIOC_MSM_ISP_REQUEST_STATS_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+9, \
	struct msm_vfe_stats_stream_request_cmd)

#define VIDIOC_MSM_ISP_CFG_STATS_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+10, struct msm_vfe_stats_stream_cfg_cmd)

#define VIDIOC_MSM_ISP_RELEASE_STATS_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+11, \
	struct msm_vfe_stats_stream_release_cmd)

#define VIDIOC_MSM_ISP_CFG_STATS_COMP_POLICY \
	_IOWR('V', BASE_VIDIOC_PRIVATE+12,   \
	      struct msm_vfe_stats_comp_policy_cfg)

#define VIDIOC_MSM_ISP_UPDATE_STREAM \
	_IOWR('V', BASE_VIDIOC_PRIVATE+13, struct msm_vfe_axi_stream_update_cmd)

#define VIDIOC_MSM_ISP_AXI_HALT \
	_IOWR('V', BASE_VIDIOC_PRIVATE+14, struct msm_vfe_axi_halt_cmd)

#define VIDIOC_MSM_ISP_AXI_RESET \
	_IOWR('V', BASE_VIDIOC_PRIVATE+15, struct msm_vfe_axi_reset_cmd)

#define VIDIOC_MSM_ISP_AXI_RESTART \
	_IOWR('V', BASE_VIDIOC_PRIVATE+16, struct msm_vfe_axi_restart_cmd)

#endif /* __MSMB_ISP__ */
